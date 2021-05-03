#include "Canon.h"
#include "ofAppGLFWWindow.h"
#include "ofxCvGui.h"
#include "ofxCvMin.h"
#include "FreeImage.h"

template<typename PixelsType>
void normalize(ofPixels_<PixelsType>& pixels, float percentile) {
	if (percentile > 1.0f || percentile < 0.0f) {
		throw(ofxMachineVision::Exception("Percentile parameter is out of range"));
	}

	if (percentile == 1.0f) {
		// use opencv in this situation
		auto image = ofxCv::toCv(pixels);
		cv::normalize(image
			, image
			, std::numeric_limits<PixelsType>::max()
			, 0.0
			, cv::NormTypes::NORM_INF);
	}
	else {
		// do it ourselves otherwise

		auto data = pixels.getData();

		std::vector<PixelsType> values;

		// skip to every 16th pixel to speed up
		for (size_t i = 0; i < pixels.size(); i += 16) {
			values.push_back(data[i]);
		}

		std::sort(values.begin(), values.end());

		auto maxValue = values.at(((float)values.size() - 1) * percentile);

		float normFactor = (float)std::numeric_limits<PixelsType>::max() / (float)maxValue;
		for (size_t i = 0; i < pixels.size(); i++) {
			data[i] = (PixelsType)((float)data[i] * normFactor);
		}
	}
	
}

enum class BayerColor {
	Green = 0
	, Red = 1
	, Blue = 2
};

BayerColor getBayerColor(int x, int y) {
	if (x % 2 == y % 2) {
		return BayerColor::Green;
	}
	else if (x % 2 == 1) {
		return BayerColor::Blue;
	}
	else {
		return BayerColor::Red;
	}
}

namespace ofxMachineVision {
	namespace Device {
		//----------
		Canon::Canon() {
			this->frameIndex = 0;
			this->markFrameNew = false;

			// Make the parameters
			this->customParameters.iso = make_shared<ofxMachineVision::Parameter<int>>(ofParameter<int>("ISO", 400));
			this->customParameters.aperture = make_shared<ofxMachineVision::Parameter<float>>(ofParameter<float>("Aperture", 9, 0, 22));
			this->customParameters.shutterSpeed = make_shared<ofxMachineVision::Parameter<float>>(ofParameter<float>("Shutter Speed", 1. / 30., 0, 60));
			this->customParameters.directRawEnabled = make_shared<ofxMachineVision::Parameter<bool>>(ofParameter<bool>("Direct raw enabled", false));
			this->customParameters.monoDebayerEnabled = make_shared<ofxMachineVision::Parameter<bool>>(ofParameter<bool>("Mono debayer enabled", false));
			this->customParameters.monoDebayerDilateIterations = make_shared<ofxMachineVision::Parameter<int>>(ofParameter<int>("Mono debayer dilations", 0));
			this->customParameters.normalize = make_shared<ofxMachineVision::Parameter<bool>>(ofParameter<bool>("Normalize", false));
			this->customParameters.normalizePercentile = make_shared<ofxMachineVision::Parameter<float>>(ofParameter<float>("Normalize %", 1, 0, 1));

			// Add to this->parameters 
			this->parameters.insert(this->parameters.end()
				, {
					this->customParameters.iso
					, this->customParameters.aperture
					, this->customParameters.shutterSpeed
					, this->customParameters.directRawEnabled
					, this->customParameters.monoDebayerEnabled
					, this->customParameters.monoDebayerDilateIterations
					, this->customParameters.normalize
					, this->customParameters.normalizePercentile
				});

			// Attach actions to the parameters
			{
				auto performInCameraDeviceThread = [this](std::function<void(shared_ptr<ofxCanon::Device>)> action) {
					auto camera = this->getCamera();
					if (camera) {
						auto cameraThread = this->camera->getCameraThread();
						if (cameraThread) {
							cameraThread->device->performInCameraThreadBlocking([this, action]() {
								action(this->getCamera()->getCameraThread()->device);
								});
						}
					}
				};

				//iso 
				{
					this->customParameters.iso->getDeviceValueFunction = [this, performInCameraDeviceThread]() {
						int value;
						performInCameraDeviceThread([this, &value](shared_ptr<ofxCanon::Device> device) {
							value = device->getISO();
							});
						return value;
					};
					this->customParameters.iso->setDeviceValueFunction = [this, performInCameraDeviceThread](const int& value) {
						performInCameraDeviceThread([this, &value](shared_ptr<ofxCanon::Device> device) {
							device->setISO(value);
							});
					};
				}

				//aperture
				{
					this->customParameters.aperture->getDeviceValueFunction = [this, performInCameraDeviceThread]() {
						float value;
						performInCameraDeviceThread([this, &value](shared_ptr<ofxCanon::Device> device) {
							value = device->getAperture();
							});
						return value;
					};
					this->customParameters.aperture->setDeviceValueFunction = [this, performInCameraDeviceThread](const float& value) {
						performInCameraDeviceThread([this, &value](shared_ptr<ofxCanon::Device> device) {
							device->setAperture(value);
							});
					};
				}

				//shutterSpeed
				{
					this->customParameters.shutterSpeed->getDeviceValueFunction = [this, performInCameraDeviceThread]() {
						float value;
						performInCameraDeviceThread([this, &value](shared_ptr<ofxCanon::Device> device) {
							value = device->getShutterSpeed();
							});
						return value;
					};
					this->customParameters.shutterSpeed->setDeviceValueFunction = [this, performInCameraDeviceThread](const float& value) {
						performInCameraDeviceThread([this, &value](shared_ptr<ofxCanon::Device> device) {
							device->setShutterSpeed(value);
							});
					};
				}
			}
		}

		//----------
		string Canon::getTypeName() const {
			return "Canon";
		}

		//----------
		Specification Canon::open(shared_ptr<Base::InitialisationSettings> initialisationSettings) {
			this->camera = make_shared<ofxCanon::Simple>();
			this->camera->setLiveView(false);
			this->camera->setDeviceId(initialisationSettings->deviceID);
			if (!this->camera->setup()) {
				return Specification();
			}

			this->openTime = chrono::system_clock::now();
			this->frameIndex = 0;

			this->singleShot();

			const auto& pixels = this->camera->getPhotoPixels();
			Specification specification(CaptureSequenceType::OneShot
				, pixels.getWidth()
				, pixels.getHeight()
				, "Canon"
				, "Photo");

			for (auto parameter : this->parameters) {
				parameter->syncFromDevice();
			}

			return specification;
		}

		//----------
		void Canon::close() {
			this->camera.reset();
		}

		//----------
		void Canon::singleShot() {
			this->camera->takePhoto();
			auto startCapture = chrono::system_clock::now();
			auto maxDuration = chrono::minutes(1);
			while (!this->camera->isPhotoNew()) {
				this->camera->update();
				glfwPollEvents();
				ofSleepMillis(1);
				if (chrono::system_clock::now() - startCapture > maxDuration) {
					throw(ofxMachineVision::Exception("Timeout during capture. Check you have a memory card in your camera and that the exposure length is not too long."));
				}
			}
			this->markFrameNew = true;
		}

		//----------
		void Canon::updateIsFrameNew() {
			this->camera->update();
		}

		//----------
		bool Canon::isFrameNew() {
			if (this->markFrameNew) {
				this->markFrameNew = false;
				return true;
			}
			else {
				return this->camera->isPhotoNew();
			}
		}

		//----------
		shared_ptr<Frame> Canon::getFrame() {
			std::shared_ptr<ofxMachineVision::Frame> frame;

			if (!this->camera) {
				throw(ofxMachineVision::Exception("No camera available"));
			}

			// Standard capture
			if (!this->customParameters.directRawEnabled->getParameterTyped<bool>()->get()) {
				frame = FramePool::X().getAvailableFrameFilledWith(this->camera->getPhotoPixels());

				//normalize
				if (this->customParameters.normalize->getParameterTyped<bool>()->get()) {
					normalize(frame->getPixels(), this->customParameters.normalizePercentile->getParameterTyped<float>()->get());
				}
			}

			// Mono debayer capture
			else {
				ofShortPixels rawPixels;

				// Load the raw pixels from the image 
				ofImageLoadSettings imageLoadSettings;
				{
					imageLoadSettings.freeImageFlags = RAW_UNPROCESSED;
				}
				auto buffer = this->camera->getPhotoCaptureResult().encodedBuffer;
				if (!buffer) {
					throw(ofxMachineVision::Exception("No buffer available from camera"));
				}
				ofLoadImage(rawPixels
					, *buffer
					, imageLoadSettings);

				//normalize before processing
				if (this->customParameters.normalize->getParameterTyped<bool>()->get()) {
					normalize(rawPixels, this->customParameters.normalizePercentile->getParameterTyped<float>()->get());
				}

				//perform the process of mono debayering based on neighborhood white balance
				if (this->customParameters.monoDebayerEnabled->getParameterTyped<bool>()->get()) {
					cv::Mat image = ofxCv::toCv(rawPixels);

					auto width = rawPixels.getWidth();
					auto height = rawPixels.getHeight();

					//create masks for red, green, blue planes
					cv::Mat redMask(height, width, CV_8U);
					cv::Mat greenMask(height, width, CV_8U);
					cv::Mat blueMask(height, width, CV_8U);
					{
						redMask.setTo(cv::Scalar(0));
						greenMask.setTo(cv::Scalar(0));
						blueMask.setTo(cv::Scalar(0));

						auto redOut = redMask.data;
						auto greenOut = greenMask.data;
						auto blueOut = blueMask.data;

						for (size_t y = 0; y < height; ++y) {
							for (size_t x = 0; x < width; ++x) {
								auto color = getBayerColor(x, y);
								switch (color) {
								case BayerColor::Red:
									*redOut = 255;
									break;
								case BayerColor::Green:
									*greenOut = 255;
									break;
								case BayerColor::Blue:
									*blueOut = 255;
									break;
								}

								redOut++;
								greenOut++;
								blueOut++;
							}
						}
					}

					//extract red, green, blue planes
					cv::Mat redPlane(height, width, CV_16U);
					cv::Mat greenPlane(height, width, CV_16U);
					cv::Mat bluePlane(height, width, CV_16U);
					{
						redPlane.setTo(cv::Scalar(0));
						greenPlane.setTo(cv::Scalar(0));
						bluePlane.setTo(cv::Scalar(0));

						cv::copyTo(image, redPlane, redMask);
						cv::copyTo(image, greenPlane, greenMask);
						cv::copyTo(image, bluePlane, blueMask);
					}

					//dilate the planes
					{
						auto crossKernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));
						auto boxKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
						cv::dilate(redPlane, redPlane, boxKernel);
						cv::dilate(greenPlane, greenPlane, crossKernel);
						cv::dilate(bluePlane, bluePlane, boxKernel);
					}

					//take local maxima for the color planes
					{
						auto dilateIterations = this->customParameters.monoDebayerDilateIterations->getParameterTyped<int>()->get();
						auto kernelSize = cv::Size(3, 3);
						auto kernel = cv::getStructuringElement(cv::MORPH_RECT, kernelSize);
						for (int i = 0; i < dilateIterations; i++) {
							//cv::dilate(redPlane, redPlane, kernel);
							//cv::dilate(greenPlane, greenPlane, kernel);
							//cv::dilate(bluePlane, bluePlane, kernel);
							cv::blur(redPlane, redPlane, kernelSize);
							cv::blur(greenPlane, greenPlane, kernelSize);
							cv::blur(bluePlane, bluePlane, kernelSize);
						}
					}

					//promote resolution to float for each plane
					cv::Mat redPlaneFloat, greenPlaneFloat, bluePlaneFloat;
					{
						redPlane.convertTo(redPlaneFloat, CV_32F);
						greenPlane.convertTo(greenPlaneFloat, CV_32F);
						bluePlane.convertTo(bluePlaneFloat, CV_32F);
					}

					//create factors as planes
					cv::Mat redFactor, blueFactor;
					{
						redFactor = greenPlaneFloat / redPlaneFloat;
						blueFactor = greenPlaneFloat / bluePlaneFloat;
					}

					//mask copy the factors back into a combined factor plane
					cv::Mat factor(height, width, CV_32F);
					{
						factor.setTo(cv::Scalar(1.0f));
						cv::copyTo(redFactor, factor, redMask);
						cv::copyTo(blueFactor, factor, blueMask);
					}

					//apply the factor plane and copy back into the original image
					{
						cv::Mat resultFloat;
						image.convertTo(resultFloat, CV_32F);
						resultFloat = resultFloat.mul(factor);
						resultFloat.convertTo(image, CV_16U);
					}
				}

				// Note the raw pixels have been converted by this point:
				frame = FramePool::X().getAvailableFrameFilledWith(rawPixels);
			}

			//timestamp
			{
				auto timeSinceOpen = chrono::system_clock::now() - this->openTime;
				frame->setTimestamp(timeSinceOpen);
			}
			frame->setFrameIndex(this->frameIndex++);

			return frame;
		}

		//----------
		shared_ptr<ofxCanon::Simple> Canon::getCamera() {
			return this->camera;
		}
	}
}