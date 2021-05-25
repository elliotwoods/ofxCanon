#pragma once

#include "ofxCanon.h"
#include "ofxMachineVision.h"
#include <opencv2/opencv.hpp>

namespace ofxMachineVision {
	namespace Device {
		cv::Mat adaptiveNormalize(cv::Mat input, float windowSize);

		class Canon : public Updating {
		public:
			struct InitialisationSettings : public Base::InitialisationSettings {
				InitialisationSettings() {
					this->add(this->monoDebayer);
				}

				ofParameter<bool> monoDebayer{ "Mono debayer", false };
			};
			Canon();
			string getTypeName() const override;
			shared_ptr<Base::InitialisationSettings> getDefaultSettings() const override {
				return make_shared<InitialisationSettings>();
			}
			Specification open(shared_ptr<Base::InitialisationSettings> = nullptr) override;
			void close() override;
			void singleShot() override;

			void updateIsFrameNew() override;
			bool isFrameNew() override;
			shared_ptr<Frame> getFrame() override;

			shared_ptr<ofxCanon::Simple> getCamera();

			//--
			// Mono debayer functions
			//
			static void processRawMono(const cv::Mat & image
				, int dilateIterations);

			static cv::Mat adaptiveNormalize(cv::Mat input, float windowSize);

			template<typename PixelsType>
			static void normalize(ofPixels_<PixelsType>& pixels, float percentile, float ignoreTop, float normalizeTo);
			//
			//--

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
				shared_ptr<ofxMachineVision::Parameter<float>> normalizeIgnoreTop;
				shared_ptr<ofxMachineVision::Parameter<float>> normalizeTo;
				shared_ptr<ofxMachineVision::Parameter<bool>> adaptiveNormalize;
				shared_ptr<ofxMachineVision::Parameter<float>> adaptiveNormalizeWindowSize;
			} customParameters;
		};
	}
}

#include "debayerRoutines.h"