CpuTscSync Changelog
===================
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
