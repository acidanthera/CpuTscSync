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
_Atomic(bool) CpuTscSyncPlugin::use_trace_point_method_to_sync = false;
_Atomic(bool) CpuTscSyncPlugin::use_clock_get_calendar_to_sync = false;
_Atomic(bool) CpuTscSyncPlugin::kernel_is_awake = false;
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

void CpuTscSyncPlugin::reset_sync_flag()
{
    tsc_synced = false;
}

bool CpuTscSyncPlugin::is_non_legacy_method_used_to_sync()
{
    return use_trace_point_method_to_sync || use_clock_get_calendar_to_sync;
}

void CpuTscSyncPlugin::init()
{
    callbackCpuf = this;
    use_trace_point_method_to_sync = (getKernelVersion() >= KernelVersion::Lion);
    use_clock_get_calendar_to_sync = (getKernelVersion() >= KernelVersion::ElCapitan) && (checkKernelArgument("TSC_sync_margin") || checkKernelArgument("-cputsclock"));
    if (use_clock_get_calendar_to_sync)
        use_trace_point_method_to_sync = false;

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
    DBGLOG("cputs", "IOHibernateSystemHasSlept is called");
    tsc_synced = false;
    kernel_is_awake = false;
    return FunctionCast(IOHibernateSystemHasSlept, callbackCpuf->orgIOHibernateSystemHasSlept)();
}

IOReturn CpuTscSyncPlugin::IOHibernateSystemWake()
{
    IOReturn result = FunctionCast(IOHibernateSystemWake, callbackCpuf->orgIOHibernateSystemWake)();
    kernel_is_awake = true;
    return result;
}

void CpuTscSyncPlugin::IOPMrootDomain_tracePoint( void *that, uint8_t point )
{
    if (callbackCpuf->use_trace_point_method_to_sync && point == kIOPMTracePointWakeCPUs) {
        DBGLOG("cputs", "IOPMrootDomain::tracePoint with point = kIOPMTracePointWakeCPUs is called");
        tsc_synced = false;
        tsc_adjust_or_reset();
    }
    FunctionCast(IOPMrootDomain_tracePoint, callbackCpuf->orgIOPMrootDomain_tracePoint)(that, point);
    
    if (callbackCpuf->use_trace_point_method_to_sync && point == kIOPMTracePointWakeCPUs)
        kernel_is_awake = true;
}

void CpuTscSyncPlugin::clock_get_calendar_microtime(clock_sec_t *secs, clock_usec_t *microsecs)
{
    FunctionCast(clock_get_calendar_microtime, callbackCpuf->org_clock_get_calendar_microtime)(secs, microsecs);
  
    if (callbackCpuf->use_clock_get_calendar_to_sync && !tsc_synced && kernel_is_awake) {
        DBGLOG("cputs", "clock_get_calendar_microtime is called after wake");
        tsc_adjust_or_reset();
    }
}

void CpuTscSyncPlugin::processKernel(KernelPatcher &patcher)
{
    if (!kernel_routed)
    {
        const auto* io_hib_system_has_slept = (getKernelVersion() >= KernelVersion::Sequoia) ? "__Z25IOHibernateSystemHasSleptv" : "_IOHibernateSystemHasSlept";
        const auto* io_hib_system_wake      = (getKernelVersion() >= KernelVersion::Sequoia) ? "__Z21IOHibernateSystemWakev"  : "_IOHibernateSystemWake";
        
        KernelPatcher::RouteRequest requests_for_long_jump[] {
            {io_hib_system_has_slept, IOHibernateSystemHasSlept, orgIOHibernateSystemHasSlept},
            {io_hib_system_wake, IOHibernateSystemWake, orgIOHibernateSystemWake}
        };
        
        size_t size = arrsize(requests_for_long_jump) - (use_clock_get_calendar_to_sync ? 0 : 1);
        if (!patcher.routeMultipleLong(KernelPatcher::KernelID, requests_for_long_jump, size))
            SYSLOG("cputs", "patcher.routeMultiple for %s is failed with error %d", requests_for_long_jump[0].symbol, patcher.getError());
        
        patcher.clearError();
        
        KernelPatcher::RouteRequest requests[] {
            {"_xcpm_urgency", xcpm_urgency, org_xcpm_urgency},
            {"__ZN14IOPMrootDomain10tracePointEh", IOPMrootDomain_tracePoint, orgIOPMrootDomain_tracePoint},
            {"_clock_get_calendar_microtime", clock_get_calendar_microtime, org_clock_get_calendar_microtime }
        };

        size = arrsize(requests) - (use_clock_get_calendar_to_sync ? 0 : 1);
        if (!patcher.routeMultiple(KernelPatcher::KernelID, requests, size))
            SYSLOG("cputs", "patcher.routeMultiple for %s is failed with error %d", requests[0].symbol, patcher.getError());

        patcher.clearError();
        
        if (use_trace_point_method_to_sync) {
            DBGLOG("cputs", "__ZN14IOPMrootDomain10tracePointEh method will be used to sync TSC after wake");
        }
        else if (use_clock_get_calendar_to_sync) {
            DBGLOG("cputs", "_clock_get_calendar_microtime method will be used to sync TSC after wake");
        }
        else {
            DBGLOG("cputs", "Legacy setPowerState method will be used to sync TSC after wake");
        }

        kernel_routed = true;
    }

    // Ignore all the errors for other processors
    patcher.clearError();
}
