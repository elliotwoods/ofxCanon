#pragma once

#include "ofxCanon.h"
#include "ofxMachineVision.h"

namespace ofxMachineVision {
	namespace Device {
		class Canon : public Updating {
		public:
			Canon();
			string getTypeName() const override;
			shared_ptr<Base::InitialisationSettings> getDefaultSettings() override {
				return make_shared<Base::InitialisationSettings>();
			}
			Specification open(shared_ptr<Base::InitialisationSettings> = nullptr) override;
			void close() override;
			void singleShot() override;

			void updateIsFrameNew() override;
			bool isFrameNew() override;
			shared_ptr<Frame> getFrame() override;

			shared_ptr<ofxCanon::Simple> getCamera();
		protected:
			int frameIndex;
			bool markFrameNew;
			ofxMachineVision::Microseconds openTime;
			shared_ptr<ofxCanon::Simple> camera;
		};
	}
}