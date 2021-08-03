/*  Do what you want with this. 
 This work originates from the ideas of Turbo and the 
 frustrations of cosmo1t the dell owner.
 *
 */

#include <IOKit/IOService.h>

class VoodooTSCSync : public IOService
{
    typedef IOService super;
    OSDeclareDefaultStructors(VoodooTSCSync)

public:
    virtual IOService* probe(IOService* provider, SInt32* score) override;
};
