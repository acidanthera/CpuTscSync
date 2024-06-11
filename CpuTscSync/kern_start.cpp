//
//  kern_start.cpp
//  CPUFriend
//
//  Copyright © 2020. All rights reserved.
//

#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>

#include "CpuTscSync.hpp"

static CpuTscSyncPlugin cpuf;

static const char *bootargOff[] {
	"-cputsoff"
};

static const char *bootargDebug[] {
	"-cputsdbg"
};

static const char *bootargBeta[] {
	"-cputsbeta"
};

PluginConfiguration ADDPR(config) {
	xStringify(PRODUCT_NAME),
	parseModuleVersion(xStringify(MODULE_VERSION)),
	LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
	bootargOff,
	arrsize(bootargOff),
	bootargDebug,
	arrsize(bootargDebug),
	bootargBeta,
	arrsize(bootargBeta),
	KernelVersion::MountainLion,
	KernelVersion::Sequoia,
	[]() {
		cpuf.init();
	}
};
