#pragma once

#include "ofxCanon.h"
#include "ofxMachineVision.h"

namespace ofxMachineVision {
	namespace Device {
		class Canon : public Blocking {
		public:
			Canon();
			string getTypeName() const override;

			void initOnMainThread() override;

			shared_ptr<Base::InitialisationSettings> getDefaultSettings() override {
				return make_shared<Base::InitialisationSettings>();
			}
			Specification open(shared_ptr<Base::InitialisationSettings> = nullptr) override;
			void close() override;
			void singleShot() override;

			void getFrame(shared_ptr<Frame>) override;

			void setExposure(Microseconds exposure) override;
			void setGain(float percent) override;
			void setFocus(float percent) override;

			shared_ptr<ofxCanon::Device> getDevice();

		protected:
			shared_ptr<Frame> frame;

			int frameIndex = 0;
			bool markFrameNew = false;
			chrono::high_resolution_clock::time_point openTime;

			vector<shared_ptr<ofxCanon::Device>> devices;
			shared_ptr<ofxCanon::Device> device;
		};
	}
}