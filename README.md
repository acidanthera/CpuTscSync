# CpuTscSync
[![Build Status](https://github.com/acidanthera/CpuTscSync/workflows/CI/badge.svg?branch=master)](https://github.com/acidanthera/CpuTscSync/actions) [![Scan Status](https://scan.coverity.com/projects/22194/badge.svg?flat=1)](https://scan.coverity.com/projects/22194)

It is a Lilu plugin, combining functionality of VoodooTSCSync and disabling xcpm_urgency if TSC is not in sync. It should solve some kernel panics after wake.

**WARNING**: CPUs without `MSR_IA32_TSC_ADJUST` (03Bh) are currently unsupported on macOS 12 and newer.
**WARNING**: if you still get kernel panic like "Non-monotonic time: invoke at 0xxxxxxxxxxx, runnable....", you can try to add `TSC_sync_margin=0` into your boot-args.
See [CpuTscSync Monterey kernel panic on wake up #1900" for more details](https://github.com/acidanthera/bugtracker/issues/1900)

#### Boot-args
- `-cputsdbg` turns on debugging output
- `-cputsbeta` enables loading on unsupported osx
- `-cputsoff` disables kext loading
- `-cputsclock` forces using of method clock_get_calendar_microtime to sync TSC (the same method is used when boot-arg `TSC_sync_margin` is specified)

#### Credits
- [Apple](https://www.apple.com) for macOS  
- [vit9696](https://github.com/vit9696) for [Lilu.kext](https://github.com/vit9696/Lilu)
- [Voodoo Projects Team](http://forge.voodooprojects.org/p/voodootscsync/) for initial idea and implementation
- [RehabMan](https://github.com/RehabMan/VoodooTSCSync) for improved implementation
- [lvs1974](https://applelife.ru/members/lvs1974.53809/) for writing the software and maintaining it
