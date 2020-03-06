//
//  CpuTscSync.hpp
//  CpuTscSync
//
//  Copyright © 2020 lvs1974. All rights reserved.
//

#ifndef kern_cputs_hpp
#define kern_cputs_hpp

#include <Headers/kern_patcher.hpp>
#include <Library/LegacyIOService.h>

class CpuTscSyncPlugin {
public:
	void init();
	
	bool kernel_routed = false;

private:
	/**
	 *  Trampolines for original resource load callback
	 */
	mach_vm_address_t org_xcpm_urgency             {0};
	
	/**
	 *  Hooked functions
	 */
	static void xcpm_urgency(int urgency, uint64_t rt_period, uint64_t rt_deadline);
	
	/**
	 *  Patch kext if needed and prepare other patches
	 *
	 *  @param patcher KernelPatcher instance
	 *  @param index   kinfo handle
	 *  @param address kinfo load address
	 *  @param size    kinfo memory size
	 */
	void processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);
	
	/**
	 *  Patch kernel
	 *
	 *  @param patcher KernelPatcher instance
	 */
	void processKernel(KernelPatcher &patcher);
};

#endif /* kern_cputs_hpp */
