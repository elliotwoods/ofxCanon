// First include the headers for any classes that you want to include
#include "ofxPlugin.h"
#include "ofxCanon.h"

#include "../pairs/ofxMachineVision/Device/Canon.h"

OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxMachineVision::Device::Base)
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxMachineVision::Device::Canon);
//	OFXPLUGIN_REGISTER(ofxMachineVision::Device::CanonLiveView);
OFXPLUGIN_PLUGIN_MODULES_END