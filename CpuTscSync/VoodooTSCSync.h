/*  Do what you want with this. 
 This work originates from the ideas of Turbo and the 
 frustrations of cosmo1t the dell owner.
 *
 */

#include <IOKit/IOService.h>
#include <IOKit/pwr_mgt/IOPMPowerSource.h>

class VoodooTSCSync : public IOService
{
    typedef IOService super;
    OSDeclareDefaultStructors(VoodooTSCSync)

    /**
     *  Power state name indexes
     */
    enum PowerState {
        PowerStateOff,
        PowerStateOn,
        PowerStateMax
    };

    /**
     *  Power states we monitor
     */
    IOPMPowerState powerStates[PowerStateMax]  {
        {kIOPMPowerStateVersion1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {kIOPMPowerStateVersion1, kIOPMPowerOn | kIOPMDeviceUsable, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
    };

public:
    /**
     *  Decide on whether to load or not by checking the processor compatibility.
     *
     *  @param provider  parent IOService object
     *  @param score     probing score
     *
     *  @return self if we could load anyhow
     */
    virtual IOService* probe(IOService* provider, SInt32* score) override;
    
    /**
     *  Add VirtualSMC listening notification.
     *
     *  @param provider  parent IOService object
     *
     *  @return true on success
     */
    bool start(IOService *provider) override;
    
    /**
     *  Update power state with the new one, here we catch sleep/wake/boot/shutdown calls
     *  New power state could be the reason for keystore to be saved to NVRAM, for example
     *
     *  @param state      power state index (must be below PowerStateMax)
     *  @param whatDevice power state device
     */
    IOReturn setPowerState(unsigned long state, IOService *whatDevice) override;
};
