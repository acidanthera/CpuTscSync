//
//  CpuTscSync.cpp
//  CpuTscSync
//
//  Copyright © 2021 lvs1974. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/kern_cpu.hpp>
#include <i386/proc_reg.h>
#include <IOKit/IOTimerEventSource.h>

#include "CpuTscSync.hpp"

static CpuTscSyncPlugin *callbackCpuf = nullptr;
_Atomic(bool) CpuTscSyncPlugin::tsc_synced = false;
_Atomic(uint16_t) CpuTscSyncPlugin::cores_ready = 0;
_Atomic(uint64_t) CpuTscSyncPlugin::tsc_frequency = 0;



//stamp the tsc
void CpuTscSyncPlugin::stamp_tsc(void *tscp)
{
    wrmsr64(MSR_IA32_TSC, *reinterpret_cast<uint64_t*>(tscp));
}

void CpuTscSyncPlugin::stamp_tsc_new(void *)
{
    atomic_fetch_add_explicit(&cores_ready, 1, memory_order_relaxed);

    uint32_t cpu = cpu_number();
    if (cpu != 0) {
        uint64_t tsc = 0;
        while ((tsc = atomic_load_explicit(&tsc_frequency, memory_order_acquire)) == 0) {
        }
        wrmsr64(MSR_IA32_TSC, tsc);
    } else {
        uint16_t threadCount = rdmsr64(MSR_CORE_THREAD_COUNT);
        while (atomic_load_explicit(&cores_ready, memory_order_acquire) != threadCount) {
        }
        atomic_store_explicit(&tsc_frequency, rdtsc64(), memory_order_relaxed);
    }
}

void CpuTscSyncPlugin::reset_tsc_adjust(void *)
{
    wrmsr64(MSR_IA32_TSC_ADJUST, 0);
}

void CpuTscSyncPlugin::tsc_adjust_or_reset()
{
    if (getKernelVersion() >= KernelVersion::Monterey) {
        DBGLOG("cputs", "reset tsc adjust");
        mp_rendezvous_no_intrs(reset_tsc_adjust, NULL);
    } else {
        uint64_t tsc = rdtsc64();
        DBGLOG("cputs", "current tsc from rdtsc64() is %lld. Rendezvouing..", tsc);
        // call the kernel function that will call this "action" on all cores/processors
        mp_rendezvous_no_intrs(stamp_tsc, &tsc);
    }
    tsc_synced = true;
}

void CpuTscSyncPlugin::init()
{
    callbackCpuf = this;

    lilu.onPatcherLoadForce(
        [](void *user, KernelPatcher &patcher) {
            static_cast<CpuTscSyncPlugin *>(user)->processKernel(patcher);
    }, this);
}

void CpuTscSyncPlugin::xcpm_urgency(int urgency, uint64_t rt_period, uint64_t rt_deadline)
{
	if (!tsc_synced)
	{
		DBGLOG("cputs", "xcpm_urgency is called when TSC presumably is not in sync, skip this call");
        return;
	}
	
	FunctionCast(xcpm_urgency, callbackCpuf->org_xcpm_urgency)(urgency, rt_period, rt_deadline);
}

IOReturn CpuTscSyncPlugin::IOHibernateSystemHasSlept()
{
    tsc_synced = false;
    return FunctionCast(IOHibernateSystemHasSlept, callbackCpuf->orgIOHibernateSystemHasSlept)();
}

IOReturn CpuTscSyncPlugin::IOHibernateSystemWake()
{
    tsc_adjust_or_reset();
    return FunctionCast(IOHibernateSystemWake, callbackCpuf->orgIOHibernateSystemWake)();
}

void CpuTscSyncPlugin::processKernel(KernelPatcher &patcher)
{
    if (!kernel_routed)
    {
        KernelPatcher::RouteRequest requests_for_long_jump[] {
            {"_IOHibernateSystemWake", IOHibernateSystemWake, orgIOHibernateSystemWake},
            {"_IOHibernateSystemHasSlept", IOHibernateSystemHasSlept, orgIOHibernateSystemHasSlept}
        };
            
        if (!patcher.routeMultipleLong(KernelPatcher::KernelID, requests_for_long_jump))
            SYSLOG("cputs", "patcher.routeMultiple for %s is failed with error %d", requests_for_long_jump[0].symbol, patcher.getError());
        
        patcher.clearError();
        
        KernelPatcher::RouteRequest requests[] {
            {"_xcpm_urgency", xcpm_urgency, org_xcpm_urgency}
        };
        
        if (!patcher.routeMultiple(KernelPatcher::KernelID, requests))
            SYSLOG("cputs", "patcher.routeMultiple for %s is failed with error %d", requests[0].symbol, patcher.getError());
        kernel_routed = true;
    }

    // Ignore all the errors for other processors
    patcher.clearError();
}
