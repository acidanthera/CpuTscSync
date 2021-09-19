//
//  CpuTscSync.hpp
//  CpuTscSync
//
//  Copyright © 2020 lvs1974. All rights reserved.
//

#ifndef kern_cputs_hpp
#define kern_cputs_hpp

#include <Headers/kern_patcher.hpp>
#include <stdatomic.h>


//reg define
#define MSR_IA32_TSC                    0x00000010
#define MSR_IA32_TSC_ADJUST             0x0000003b


class CpuTscSyncPlugin {
public:
	void init();
	
private:
    _Atomic(bool) kernel_routed = false;
    static _Atomic(bool) tsc_synced;
    static _Atomic(bool) clock_get_calendar_called_after_wake;
    static _Atomic(uint16_t) cores_ready;
    static _Atomic(uint64_t) tsc_frequency;
    static _Atomic(uint64_t) xnu_thread_tid;
    
private:
	/**
	 *  Trampolines for original resource load callback
	 */
	mach_vm_address_t org_xcpm_urgency {0};
    mach_vm_address_t orgIOHibernateSystemHasSlept {0};
    mach_vm_address_t orgIOHibernateSystemWake {0};
    mach_vm_address_t org_clock_get_calendar_microtime {0};
     
	/**
	 *  Hooked functions
	 */
	static void     xcpm_urgency(int urgency, uint64_t rt_period, uint64_t rt_deadline);
    static IOReturn IOHibernateSystemHasSlept(void);
    static IOReturn IOHibernateSystemWake();
    static void     clock_get_calendar_microtime(clock_sec_t *secs, clock_usec_t *microsecs);
 	
	/**
	 *  Patch kernel
	 *
	 *  @param patcher KernelPatcher instance
	 */
	void processKernel(KernelPatcher &patcher);
    
    
    static void stamp_tsc(void *tscp);
    static void stamp_tsc_new(void *);
    static void reset_tsc_adjust(void *);

public:
    static void tsc_adjust_or_reset();
    static void reset_sync_flag();
    static bool is_clock_get_calendar_called_after_wake();
};

#endif /* kern_cputs_hpp */
