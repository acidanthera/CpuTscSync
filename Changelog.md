CpuTscSync Changelog
===================
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
