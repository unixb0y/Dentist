//
//  kern_start.cpp
//  Dentist
//
//  Copyright © 2020 unixb0y. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/plugin_start.hpp>

#include "dentist.hpp"

static Connector connector;

static const char *bootargOff[] {
	"-dentistoff"
};

static const char *bootargDebug[] {
	"-dentistdbg"
};

static const char *bootargBeta[] {
	"-dentistbeta"
};

PluginConfiguration ADDPR(config) {
	xStringify(PRODUCT_NAME),
	parseModuleVersion(xStringify(MODULE_VERSION)),
	LiluAPI::AllowNormal,
	bootargOff,
	arrsize(bootargOff),
	bootargDebug,
	arrsize(bootargDebug),
	bootargBeta,
	arrsize(bootargBeta),
	KernelVersion::MountainLion,
	KernelVersion::Catalina,
	[]() {
		connector.init();
	}
};
