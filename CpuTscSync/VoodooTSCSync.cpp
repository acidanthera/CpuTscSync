#include "VoodooTSCSync.h"
#include "CpuTscSync.hpp"

#include <i386/proc_reg.h>

OSDefineMetaClassAndStructors(VoodooTSCSync, IOService)

IOService* VoodooTSCSync::probe(IOService* provider, SInt32* score)
{
    if (!super::probe(provider, score)) return NULL;
    if (!provider) return NULL;

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
