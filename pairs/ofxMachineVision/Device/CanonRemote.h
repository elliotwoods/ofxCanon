#pragma once
#include "ofxCanon.h"
#include "ofxMachineVision.h"

namespace ofxMachineVision {
	namespace Device {
		class CanonRemote : public Updating {
		public:
			struct InitialisationSettings : public Base::InitialisationSettings {
				InitialisationSettings();
				ofParameter<string> hostname{ "Hostname", "172.30.1.100" };
			};

			CanonRemote();
			string getTypeName() const override;
			shared_ptr<Base::InitialisationSettings> getDefaultSettings() const override;
			Specification open(shared_ptr<Base::InitialisationSettings> = nullptr) override;
			void close() override;
			void singleShot() override;

			void updateIsFrameNew() override;
			bool isFrameNew() override;
			shared_ptr<Frame> getFrame() override;
		protected:
			void callbackDeviceShootingModeChange(string&);
			void callbackDeviceISOChange(int&);
			void callbackDeviceApertureChange(float&);
			void callbackDeviceShutterSpeedChange(float&);

			ofxCanon::RemoteDevice device;
			shared_ptr<Frame> frame;
			uint64_t frameIndex = 0;
			uint64_t timeOpen = 0;

			bool firstFrame = true;
			bool frameIsNew = false;

			struct {
				shared_ptr<ofxMachineVision::Parameter<string>> shootingMode;
				shared_ptr<ofxMachineVision::Parameter<int>> iso;
				shared_ptr<ofxMachineVision::Parameter<float>> aperture;
				shared_ptr<ofxMachineVision::Parameter<float>> shutterSpeed;
				shared_ptr<ofxMachineVision::Parameter<bool>> keepPhotosOnCamera;
				shared_ptr<ofxMachineVision::Parameter<bool>> downloadJPEG;
				shared_ptr<ofxMachineVision::Parameter<bool>> downloadRAW;
			} customParameters;
		};
	}
}