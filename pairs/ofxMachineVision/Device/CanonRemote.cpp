#include "CanonRemote.h"

namespace ofxMachineVision {
	namespace Device {
		//----------
		CanonRemote::InitialisationSettings::InitialisationSettings()
		{
			this->clear();
			this->add(this->hostname);
		}

		//----------
		CanonRemote::CanonRemote()
		{
			// Make the parameters
			this->customParameters.shootingMode= make_shared<ofxMachineVision::Parameter<string>>(ofParameter<string>("Shooting mode", "m"));
			this->customParameters.iso = make_shared<ofxMachineVision::Parameter<int>>(ofParameter<int>("ISO", 400));
			this->customParameters.aperture = make_shared<ofxMachineVision::Parameter<float>>(ofParameter<float>("Aperture", 9, 0, 22));
			this->customParameters.shutterSpeed = make_shared<ofxMachineVision::Parameter<float>>(ofParameter<float>("Shutter Speed", 1. / 30., 0, 60));
			this->customParameters.keepPhotosOnCamera = make_shared<ofxMachineVision::Parameter<bool>>(ofParameter<bool>("Keep photos on camera", false));
			this->customParameters.downloadJPEG = make_shared<ofxMachineVision::Parameter<bool>>(ofParameter<bool>("Download JPEG", true));
			this->customParameters.downloadRAW = make_shared<ofxMachineVision::Parameter<bool>>(ofParameter<bool>("Download RAW", true));

			// Add to this->parameters 
			this->parameters.insert(this->parameters.end()
				, {
					this->customParameters.shootingMode
					, this->customParameters.iso
					, this->customParameters.aperture
					, this->customParameters.shutterSpeed
					, this->customParameters.keepPhotosOnCamera
					, this->customParameters.downloadJPEG
					, this->customParameters.downloadRAW
				});

			// Attach actions to the parameters
			{
				//shooting mode
				{
					this->customParameters.shootingMode->getDeviceValueFunction = [this]() {
						string value;
						if (this->device.getShootingMode(value)) {
							return value;
						}
						else {
							throw(ofxMachineVision::Exception("Failed to pull shootingMode"));
						}
					};
					this->customParameters.shootingMode->setDeviceValueFunction = [this](const string& value) {
						if (!this->device.setShootingMode(value)) {
							throw(ofxMachineVision::Exception("Failed to push shootingMode"));
						}
					};
				}

				//iso 
				{
					this->customParameters.iso->getDeviceValueFunction = [this]() {
						int value;
						if (this->device.getISO(value)) {
							return value;
						}
						else {
							throw(ofxMachineVision::Exception("Failed to pull ISO"));
						}
					};
					this->customParameters.iso->setDeviceValueFunction = [this](const int& value) {
						if (!this->device.setISO(value)) {
							throw(ofxMachineVision::Exception("Failed to push ISO"));
						}
					};
				}

				//aperture
				{
					this->customParameters.aperture->getDeviceValueFunction = [this]() {
						float value;
						if (this->device.getAperture(value)) {
							return value;
						}
						else {
							throw(ofxMachineVision::Exception("Failed to pull aperture"));
						}
					};
					this->customParameters.aperture->setDeviceValueFunction = [this](const float& value) {
						if (!this->device.setAperture(value)) {
							throw(ofxMachineVision::Exception("Failed to push aperture"));
						}
					};
				}

				//shutterSpeed
				{
					this->customParameters.shutterSpeed->getDeviceValueFunction = [this]() {
						float value;
						if (this->device.getShutterSpeed(value)) {
							return value;
						}
						else {
							throw(ofxMachineVision::Exception("Failed to pull shutter speed"));
						}
					};
					this->customParameters.shutterSpeed->setDeviceValueFunction = [this](const float& value) {
						if (!this->device.setShutterSpeed(value)) {
							throw(ofxMachineVision::Exception("Failed to push shutter speed"));
						}
					};
				}

				//keep photos on camera
				{
					this->customParameters.keepPhotosOnCamera->getDeviceValueFunction = [this]() {
						return this->device.getKeepFilesOnDevice();
					};
					this->customParameters.keepPhotosOnCamera->setDeviceValueFunction = [this](const bool& value) {
						return this->device.setKeepFilesOnDevice(value);
					};
				}

				//download JPEG
				{
					this->customParameters.downloadJPEG->getDeviceValueFunction = [this]() {
						return this->device.getDownloadJPEG();
					};
					this->customParameters.downloadJPEG->setDeviceValueFunction = [this](const bool& value) {
						return this->device.setDownloadJPEG(value);
					};
				}

				//download RAW
				{
					this->customParameters.downloadRAW->getDeviceValueFunction = [this]() {
						return this->device.getDownloadRAW();
					};
					this->customParameters.downloadRAW->setDeviceValueFunction = [this](const bool& value) {
						return this->device.setDownloadRAW(value);
					};
				}
			}

			// Add listeners to events coming from device
			{
				ofAddListener(this->device.deviceEvents.onShootingModeChange, this, &CanonRemote::callbackDeviceShootingModeChange);
				ofAddListener(this->device.deviceEvents.onISOChange, this, &CanonRemote::callbackDeviceISOChange);
				ofAddListener(this->device.deviceEvents.onApertureChange, this, &CanonRemote::callbackDeviceApertureChange);
				ofAddListener(this->device.deviceEvents.onShutterSpeedChange, this, &CanonRemote::callbackDeviceShutterSpeedChange);
			}
		}

		//----------
		string
		CanonRemote::getTypeName() const
		{
			return "CanonRemote";
		}

		//----------
		shared_ptr<Base::InitialisationSettings>
			CanonRemote::getDefaultSettings() const
		{
			return make_shared<InitialisationSettings>();
		}

		//----------
		Specification
			CanonRemote::open(shared_ptr<Base::InitialisationSettings> initialisationSettings)
		{
			auto typedInitialisationSettings = dynamic_pointer_cast<InitialisationSettings>(initialisationSettings);

			if (!typedInitialisationSettings) {
				throw(ofxMachineVision::Exception("Invalid InitialisationSettings"));
			}

			this->device.open(typedInitialisationSettings->hostname);
			this->device.getImage().setUseTexture(false);
			this->timeOpen = ofGetSystemTimeMicros();

			this->device.takePhoto(false);

			auto timeStart = ofGetElapsedTimeMillis();
			while (ofGetElapsedTimeMillis() < timeStart + 30000) {
				this->device.update();
				if (this->device.isFrameNew()) {
					break;
				}
			}

			if (!this->device.isFrameNew()) {
				throw(ofxMachineVision::Exception("Failed to open CanonRemote using CCAPI : timeout"));
			}

			Specification specification(CaptureSequenceType::OneShot
				, this->device.getImage().getWidth()
				, this->device.getImage().getHeight()
				, "Canon"
				, "Photo");

			return specification;
		}

		//----------
		void
			CanonRemote::close()
		{

		}

		//----------
		void
			CanonRemote::singleShot()
		{
			this->device.takePhoto(true);
		}

		//----------
		bool
			CanonRemote::isFrameNew()
		{
			return this->frameIsNew;
		}

		//----------
		void
			CanonRemote::updateIsFrameNew()
		{
			if (this->firstFrame) {
				this->firstFrame = false;
				this->frameIsNew = true;
			}
			else {
				this->device.update();
				this->frameIsNew = this->device.isFrameNew();
			}

			if (this->frameIsNew) {
				auto frame = FramePool::X().getAvailableFrameFilledWith(this->device.getImage());
				frame->setFrameIndex(this->frameIndex++);
				frame->setTimestamp(chrono::nanoseconds(1000 * (ofGetElapsedTimeMicros() - this->timeOpen)));
				this->frame = frame;
			}
		}

		//----------
		shared_ptr<Frame>
			CanonRemote::getFrame()
		{
			return this->frame;
		}

		//----------
		void
			CanonRemote::callbackDeviceShootingModeChange(string& value)
		{
			this->customParameters.shootingMode->getParameterTypedAuto()->set(value);
		}

		//----------
		void
			CanonRemote::callbackDeviceISOChange(int& value)
		{
			this->customParameters.iso->getParameterTypedAuto()->set(value);
		}

		//----------
		void
			CanonRemote::callbackDeviceApertureChange(float& value)
		{
			this->customParameters.aperture->getParameterTypedAuto()->set(value);
		}

		//----------
		void
			CanonRemote::callbackDeviceShutterSpeedChange(float& value)
		{
			this->customParameters.shutterSpeed->getParameterTypedAuto()->set(value);
		}
	}
}