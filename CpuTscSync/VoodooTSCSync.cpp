#include "VoodooTSCSync.h"
#include "CpuTscSync.hpp"

#include <i386/proc_reg.h>

OSDefineMetaClassAndStructors(VoodooTSCSync, IOService)

IOService* VoodooTSCSync::probe(IOService* provider, SInt32* score)
{
    if (!provider) return NULL;
    if (!super::probe(provider, score)) return NULL;

    OSNumber* cpuNumber = OSDynamicCast(OSNumber, provider->getProperty("IOCPUNumber"));
    if (!cpuNumber) return NULL;

    if (getKernelVersion() >= KernelVersion::Monterey) {
        // only attach to the first CPU
        if (cpuNumber->unsigned16BitValue() != 0) return NULL;
        CpuTscSyncPlugin::tsc_adjust_or_reset();
    } else {
        // only attach to the last CPU
        uint16_t threadCount = rdmsr64(MSR_CORE_THREAD_COUNT);
        if (cpuNumber->unsigned16BitValue() != threadCount-1) return NULL;
        CpuTscSyncPlugin::tsc_adjust_or_reset();
    }

    return this;
}

bool VoodooTSCSync::start(IOService *provider) {
    if (!IOService::start(provider)) {
        SYSLOG("cputs", "failed to start the parent");
        return false;
    }

    PMinit();
    provider->joinPMtree(this);
    registerPowerDriver(this, powerStates, arrsize(powerStates));

    return true;
}

IOReturn VoodooTSCSync::setPowerState(unsigned long state, IOService *whatDevice){
    if (!CpuTscSyncPlugin::is_non_legacy_method_used_to_sync()) {
        DBGLOG("cputs", "changing power state to %lu", state);
        if (state == PowerStateOff)
            CpuTscSyncPlugin::reset_sync_flag();
        if (state == PowerStateOn)
            CpuTscSyncPlugin::tsc_adjust_or_reset();
    }

    return kIOPMAckImplied;
}
