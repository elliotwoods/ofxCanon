#include "Initializer.h"

#include "ofLog.h"

#include <memory>
#include "EDSDK.h"

namespace ofxCanon {
	//----------
	Initializer & Initializer::X() {
		static auto instance = std::make_unique<Initializer>();
		return *instance;
	}

	//----------
	bool Initializer::isInitialized() const {
		return this->initialized;
	}

	//----------
	Initializer::Initializer() {
		auto error = EdsInitializeSDK();
		if (error != EDS_ERR_OK) {
			ofLogError("ofxCanon") << "Failed to initialize EDSDK";
			this->initialized = false;
		}
		else {
			this->initialized = true;
		}
	}

	//----------
	Initializer::~Initializer() {
		auto error = EdsTerminateSDK();
		if (error != EDS_ERR_OK) {
			ofLogError("ofxCanon") << "Failed to terminate EDSDK";
		}
		this->initialized = false;
	}
}