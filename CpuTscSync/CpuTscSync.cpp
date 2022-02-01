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
_Atomic(bool) CpuTscSyncPlugin::clock_get_calendar_called_after_wake = false;
_Atomic(uint16_t) CpuTscSyncPlugin::cores_ready = 0;
_Atomic(uint64_t) CpuTscSyncPlugin::tsc_frequency = 0;
_Atomic(uint64_t) CpuTscSyncPlugin::xnu_thread_tid = -1UL;



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

void CpuTscSyncPlugin::reset_sync_flag()
{
    tsc_synced = false;
    xnu_thread_tid = -1UL;
}

bool CpuTscSyncPlugin::is_clock_get_calendar_called_after_wake()
{
    return clock_get_calendar_called_after_wake;
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
    xnu_thread_tid = -1UL;
    return FunctionCast(IOHibernateSystemHasSlept, callbackCpuf->orgIOHibernateSystemHasSlept)();
}

IOReturn CpuTscSyncPlugin::IOHibernateSystemWake()
{
    // post pone tsc sync in IOPMrootDomain::powerChangeDone,
    // it will be executed later, after calculating wakeup time
    tsc_synced = false;
    xnu_thread_tid = thread_tid(current_thread());
    DBGLOG("cputs", "IOHibernateSystemWake is called");
    return FunctionCast(IOHibernateSystemWake, callbackCpuf->orgIOHibernateSystemWake)();
}

void CpuTscSyncPlugin::clock_get_calendar_microtime(clock_sec_t *secs, clock_usec_t *microsecs)
{
    FunctionCast(clock_get_calendar_microtime, callbackCpuf->org_clock_get_calendar_microtime)(secs, microsecs);
  
    if (xnu_thread_tid == thread_tid(current_thread())) {
        xnu_thread_tid = -1UL;
        DBGLOG("cputs", "clock_get_calendar_microtime is called after wake");
        tsc_adjust_or_reset();
    }
}

void CpuTscSyncPlugin::IOPlatformActionsPostResume(void)
{
    FunctionCast(IOPlatformActionsPostResume, callbackCpuf->orgIOPlatformActionsPostResume)();
    DBGLOG("cputs", "IOPlatformActionsPostResume is called");
    tsc_adjust_or_reset();
}

void CpuTscSyncPlugin::processKernel(KernelPatcher &patcher)
{
    if (!kernel_routed)
    {
        clock_get_calendar_called_after_wake = (getKernelVersion() >= KernelVersion::ElCapitan);
        if (clock_get_calendar_called_after_wake) {
            DBGLOG("cputs", "_clock_get_calendar_microtime will be used to sync tsc after wake");
        }
        
        KernelPatcher::RouteRequest requests_for_long_jump[] {
            {"_IOHibernateSystemHasSlept", IOHibernateSystemHasSlept, orgIOHibernateSystemHasSlept},
            {"_IOHibernateSystemWake", IOHibernateSystemWake, orgIOHibernateSystemWake}
        };
        
        size_t size = arrsize(requests_for_long_jump) - (clock_get_calendar_called_after_wake ? 0 : 1);
        if (!patcher.routeMultipleLong(KernelPatcher::KernelID, requests_for_long_jump, size))
            SYSLOG("cputs", "patcher.routeMultiple for %s is failed with error %d", requests_for_long_jump[0].symbol, patcher.getError());
        
        patcher.clearError();
        
        KernelPatcher::RouteRequest requests[] {
            {"_xcpm_urgency", xcpm_urgency, org_xcpm_urgency},
            {"_clock_get_calendar_microtime", clock_get_calendar_microtime, org_clock_get_calendar_microtime }
        };

        size = arrsize(requests) - (clock_get_calendar_called_after_wake ? 0 : 1);
        if (!patcher.routeMultiple(KernelPatcher::KernelID, requests, size))
            SYSLOG("cputs", "patcher.routeMultiple for %s is failed with error %d", requests[0].symbol, patcher.getError());

        patcher.clearError();

        if (patcher.solveSymbol(KernelPatcher::KernelID, "__Z27IOPlatformActionsPostResumev"))
        {
            KernelPatcher::RouteRequest requests[] {
                {"__Z27IOPlatformActionsPostResumev", IOPlatformActionsPostResume, orgIOPlatformActionsPostResume}
            };

            if (!patcher.routeMultiple(KernelPatcher::KernelID, requests))
                SYSLOG("cputs", "patcher.routeMultiple for %s is failed with error %d", requests[0].symbol, patcher.getError());
        }

        patcher.clearError();

        kernel_routed = true;
    }

    // Ignore all the errors for other processors
    patcher.clearError();
}
