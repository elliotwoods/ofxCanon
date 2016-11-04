#pragma once

#include "EDSDK_include.h"

namespace ofxCanon {
	// Handlers are callbacks fired through glfwPollEvents (i.e. the EOS SDK fires these events via the OS handle callback system)
	// All these callbacks will happen in the main thread, whilst any resulting actions must be called in the camera thread
	class Handlers {
	public:
		static EdsError EDSCALLBACK handleObjectEvent(EdsObjectEvent eventType, EdsBaseRef object, EdsVoid* context);
		static EdsError EDSCALLBACK handlePropertyEvent(EdsPropertyEvent eventType, EdsPropertyID propertyId, EdsUInt32 param, EdsVoid* context);
		static EdsError EDSCALLBACK handleStateEvent(EdsStateEvent eventType, EdsUInt32 param, EdsVoid* context);
	};
}
