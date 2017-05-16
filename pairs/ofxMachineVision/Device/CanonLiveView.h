#pragma once

#include "ofxCanon.h"
#include "ofxMachineVision.h"

namespace ofxMachineVision {
	namespace Device {
		class CanonLiveView: public Updating {
		public:
			CanonLiveView();
			string getTypeName() const override;
			shared_ptr<Base::InitialisationSettings> getDefaultSettings() const override {
				return make_shared<Base::InitialisationSettings>();
			}
			Specification open(shared_ptr<Base::InitialisationSettings> = nullptr) override;
			void close() override;

			void updateIsFrameNew() override;
			bool isFrameNew() override;
			shared_ptr<Frame> getFrame() override;

			shared_ptr<ofxCanon::Simple> getCamera();
		protected:
			int frameIndex;
			bool markFrameNew;
			chrono::high_resolution_clock::time_point openTime;
			shared_ptr<ofxCanon::Simple> camera;
		};
	}
}