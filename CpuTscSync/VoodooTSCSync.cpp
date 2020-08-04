#include <Headers/kern_util.hpp>
#include "VoodooTSCSync.h"

OSDefineMetaClassAndStructors(VoodooTSCSync, IOService)

_Atomic(bool) VoodooTSCSync::tsc_synced = false;

//stamp the tsc
extern "C" void stamp_tsc(void *tscp)
{
	wrmsr64(MSR_IA32_TSC, *reinterpret_cast<uint64_t*>(tscp));
}

IOService* VoodooTSCSync::probe(IOService* provider, SInt32* score)
{
    if (!super::probe(provider, score)) return NULL;
    if (!provider) return NULL;

    // only attach to last CPU
    uint16_t threadCount = rdmsr64(MSR_CORE_THREAD_COUNT);
    OSNumber* cpuNumber = OSDynamicCast(OSNumber, provider->getProperty("IOCPUNumber"));
    if (!cpuNumber || cpuNumber->unsigned16BitValue() != threadCount-1)
        return NULL;

    return this;
}

static IOPMPowerState powerStates[2] =
{
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, kIOPMPowerOn, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0 }
};

IOReturn VoodooTSCSync::setPowerState(unsigned long powerStateOrdinal, IOService *whatDevice )
{
	tsc_synced = false;

    if (powerStateOrdinal)
        doTSC();
	
    return IOPMAckImplied;
}

void VoodooTSCSync::stop(IOService *provider)
{
    PMstop();
    super::stop(provider);
}

bool VoodooTSCSync::start(IOService *provider)
{
	if (!super::start(provider)) { return false; }

    // announce version
    extern kmod_info_t kmod_info;
    SYSLOG("cputs", "Version %s starting on OS X Darwin %d.%d.\n", kmod_info.version, version_major, version_minor);

    // place version/build info in ioreg properties RM,Build and RM,Version
    char buf[128];
    snprintf(buf, sizeof(buf), "%s %s", kmod_info.name, kmod_info.version);
    setProperty("RM,Version", buf);
#ifdef DEBUG
    setProperty("RM,Build", "Debug");
#else
    setProperty("RM,Build", "Release");
#endif

    PMinit();
    registerPowerDriver(this, powerStates, 2);
    provider->joinPMtree(this);
	return true;
}

// Update MSR on all processors.
void VoodooTSCSync::doTSC()
{
	uint64_t tsc = rdtsc64();
	SYSLOG("cputs", "current tsc from rdtsc64() is %lld. Rendezvouing..\n", tsc);
	
	// call the kernel function that will call this "action" on all cores/processors
	mp_rendezvous_no_intrs(stamp_tsc, &tsc);
	tsc_synced = true;
}
