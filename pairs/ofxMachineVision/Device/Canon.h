#pragma once

#include "ofxCanon.h"
#include "ofxMachineVision.h"

namespace ofxMachineVision {
	namespace Device {
		class Canon : public Updating {
		public:
			Canon();
			string getTypeName() const override;
			shared_ptr<Base::InitialisationSettings> getDefaultSettings() const override {
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
			chrono::system_clock::time_point openTime;
			shared_ptr<ofxCanon::Simple> camera;

			struct {
				shared_ptr<ofxMachineVision::Parameter<int>> iso;
				shared_ptr<ofxMachineVision::Parameter<float>> aperture;
				shared_ptr<ofxMachineVision::Parameter<float>> shutterSpeed;
				shared_ptr<ofxMachineVision::Parameter<bool>> directRawEnabled;
				shared_ptr<ofxMachineVision::Parameter<bool>> monoDebayerEnabled;
				shared_ptr<ofxMachineVision::Parameter<int>> monoDebayerDilateIterations;
				shared_ptr<ofxMachineVision::Parameter<bool>> normalize;
				shared_ptr<ofxMachineVision::Parameter<float>> normalizePercentile;
			} customParameters;
		};
	}
}