#pragma once
#include "ofxCanon.h"
#include "ofxMachineVision.h"

namespace ofxMachineVision {
	namespace Device {
		class CanonRemote : public Updating {
			struct InitialisationSettings : public Base::InitialisationSettings {
				InitialisationSettings();
				ofParameter<string> hostname;
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
			ofxCanon::RemoteDevice device;
			shared_ptr<Frame> frame;
			uint64_t frameIndex = 0;
			uint64_t timeOpen = 0;
		};
	}
}