CpuTscSync Changelog
===================
#### v1.1.1
- Added constants for macOS 15 support

#### v1.1.0
- Added constants for macOS 14 support

#### v1.0.9
- Added constants for macOS 13 support

#### v1.0.8
- Fix old sync logic used when boot-arg -cputsclock is specified

#### v1.0.7
- Find a better place to sync TSC in the kernel (supported since 10.7)
- boot-args `-cputsclock` or `TSC_sync_margin` can be used to select older method to sync TSC

#### v1.0.6
- Override one more kernel method `IOPlatformActionsPostResume` to perform sync as early as possible
- README extended with an additional hint related to `TSC_sync_margin=0`

#### v1.0.5
- Fix issue with wakeup time: tsc sync is performed too early, so wakeup time can be incorrect, some app can crash with assertion failure: "currentTime >= wakeUpTime"
- Revert back tsc sync in VoodooTSCSync::setPowerState as a fallback for older systems

#### v1.0.4
- Added constants for macOS 12 support
- Added macOS 12 compatibility for CPUs with `MSR_IA32_TSC_ADJUST` (03Bh)

#### v1.0.3
- Added MacKernelSDK with Xcode 12 compatibility

#### v1.0.2
- Compatibility for macOS 11
- Use atomic variable tsc_synced 

#### v1.0.1
- Minor improvements (logging, original xcpm_urgency is called after syncing)

#### v1.0.0
- Initial release
