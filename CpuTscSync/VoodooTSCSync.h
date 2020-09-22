/*  Do what you want with this. 
 This work originates from the ideas of Turbo and the 
 frustrations of cosmo1t the dell owner.
 *
 */

#include <stdatomic.h>

#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>
#include <i386/proc_reg.h>

//reg define
#define MSR_IA32_TSC                    0x00000010

//extern function defined in mp.c from xnu
extern "C" void  mp_rendezvous_no_intrs(void (*action_func)(void*), void *arg);

class VoodooTSCSync : public IOService
{
    typedef IOService super;
    OSDeclareDefaultStructors(VoodooTSCSync)

private:
	static void doTSC(void);

public:
    static bool isTscSynced() { return tsc_synced; };

    virtual IOService* probe(IOService* provider, SInt32* score) override;
    virtual bool start(IOService* provider) override;
    virtual void stop(IOService* provider) override;
    virtual IOReturn setPowerState(unsigned long powerStateOrdinal, IOService* whatDevice) override;
    
protected:
    static _Atomic(bool) tsc_synced;
};
