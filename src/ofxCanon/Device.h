#pragma once

#include "Utils.h"
#include "Handlers.h"

#include "ofPixels.h"
#include "ofParameter.h"
#include "ofThreadChannel.h"

#include <GLFW/glfw3.h>

#include <future>
#include <string>
#include <vector>
#include <memory>

namespace ofxCanon {

	/*
		A blocking implementation of an EDSDK Camera Device.

		You can get a list of connected devices using the listDevices() method.

		All operations which happen on this class should come from the 'Camera thread'
		which is a thread that you own externally. That could be your main thread, or
		any thread that you've created. You should regularly call Device::update()
		from that thread (e.g. in your update loop if you're using your main thread).

		Often you will want to wrap this class in something which handles threading
		and presents images to the screen in textures.
	*/
	class Device {
	public:
		struct DeviceInfo {
			std::string description;
			std::string port;

			struct Owner {
				std::string manufacturer;
				std::string owner;
				std::string artist;
				std::string copyright;
			} owner;

			struct Battery {
				float batteryLevel = 0.0f;
				float batteryQuality = 1.0f;
				bool psuPresent = false;
			} battery;

			std::string toString() const;
		};

		struct LensInfo {
			bool lensAttached = false;
			std::string lensName;

			std::string toString() const;
		};

		struct PhotoMetadata {
			struct FocalLength {
				float currentFocalLength = 0;
				float maximumFocalLength = 0;
				float minimumFocalLength = 0;
			} focalLength;
		};

		enum CaptureStatus {
			NoCaptureTriggered,
			WaitingForPhotoDownload,
			CaptureFailed,
			CaptureSucceeded
		};

		struct PhotoCaptureResult {
			std::shared_ptr<ofBuffer> encodedBuffer;
			std::shared_ptr<PhotoMetadata> metaData;
			EdsError errorReturned;

			operator bool() const {
				return this->errorReturned == EDS_ERR_OK;
			}
		};

		Device(EdsCameraRef);
		~Device();

		EdsCameraRef & getRawCamera();

		const DeviceInfo & getDeviceInfo() const;
		const LensInfo & getLensInfo() const; // Note this is only updated when photos are captured

		bool open();
		void close();
		void update();

		std::future<PhotoCaptureResult> takePhotoAsync();
		void takePhotoToMemoryCard() const;

		// take photo and wait until complete
		// pass in your own ofPixels or ofShortPixels (i.e. for 8bit and 16bit images)
		// NOTE : this function is only compatible with the main thread (since it uses glfwPollEvents), you must use takePhotoAsync otherwise
		template<typename PixelsType>
		PhotoCaptureResult takePhoto(ofPixels_<PixelsType> & pixelsOut) {
			auto future = this->takePhotoAsync();
			while (future.wait_for(chrono::milliseconds(10)) != future_status::ready) {
				glfwPollEvents();
				this->update();
			}

			auto result = future.get();
			if (result.errorReturned == EDS_ERR_OK) {
				ofLoadImage(pixelsOut, *result.encodedBuffer);
			}
			return result;
		}

		bool getLiveView(ofPixels &) const;

		CaptureStatus getCaptureStatus() const;

		ofParameterGroup & getParameters();

		std::vector<std::string> getOptions(const ofAbstractParameter &) const;
		std::vector<int> getISOOptions() const;
		std::vector<float> getApertureOptions() const;
		std::vector<float> getShutterSpeedOptions() const;

		int getISO() const;
		void setISO(int ISO, bool findClosest = true);

		float getAperture() const;
		void setAperture(float aperture, bool findClosest = true);

		float getShutterSpeed() const;
		void setShutterSpeed(float shutterSpeed, bool findClosest = true);

		bool getLiveViewEnabled() const;
		void setLiveViewEnabled(bool liveViewEnabled, bool enableCameraScreen = true);

		template<typename DataType>
		DataType getProperty(EdsPropertyID propertyID) const {
			DataType value;
			ERROR_GOTO_FAIL(EdsGetPropertyData(this->camera, propertyID, 0, sizeof(DataType), &value)
				, "Get property : " + propertyToString(propertyID));
		fail:
			return value;
		}

		template<typename DataType>
		std::vector<DataType> getPropertyArray(EdsPropertyID propertyID, size_t size) const {
			vector<DataType> value(size);
			ERROR_GOTO_FAIL(EdsGetPropertyData(this->camera, propertyID, 0, (EdsUInt32)(sizeof(DataType) * size), value.data())
				, "Get property array : " + propertyToString(propertyID));
			return value;
		fail:
			value.clear();
			return value;
		}

		template<typename DataType>
		bool setProperty(EdsPropertyID propertyID, DataType value) {
			ERROR_GOTO_FAIL(EdsSetPropertyData(this->camera, propertyID, 0, sizeof(DataType), &value)
				, "Set property : " + propertyToString(propertyID));
			return true;
		fail:
			return false;
		}

		bool getLogDeviceCallbacks() const;
		void setLogDeviceCallbacks(bool);

		typedef std::function<void()> Action;
		void performInCameraThread(Action &&);
		void performInCameraThreadBlocking(Action &&);

		void setDownloadEnabled(bool);
		bool getDownloadEnabled() const;

		//these events will always fire in the camera thread
		ofEvent<EdsPropertyID> onParameterOptionsChange;
		ofEvent<LensInfo> onLensChange;
		ofEvent<PhotoCaptureResult> onUnrequestedPhotoReceived;
	protected:
		friend Handlers;

		std::thread::id cameraThreadId;

		void download(EdsDirectoryItemRef);

		void lensChanged();

		void pollProperty(EdsPropertyID);

		void callbackISOParameterChanged(int &);
		void callbackApertureParameterChanged(float &);
		void callbackShutterSpeedParameterChanged(float &);

		EdsCameraRef camera = NULL;
		bool downloadEnabled = true;
		bool isOpen = false;
		bool logDeviceCallbacks = false;
		bool hasDownloadedFirstPhoto = false;
		CaptureStatus captureStatus = CaptureStatus::NoCaptureTriggered;
		std::unique_ptr<std::promise<PhotoCaptureResult>> capturePromise;

		DeviceInfo deviceInfo;
		LensInfo lensInfo;

		ofThreadChannel<Action> actionQueue;

		bool liveViewEnabled = false;

#ifndef PARAM_DECLARE
		// Syntactic sugar which enables struct-ofParameterGroup
#define PARAM_DECLARE(NAME, ...) bool paramDeclareConstructor \
	{ [this] { this->setName(NAME), this->add(__VA_ARGS__); return true; }() };
#endif

		struct Parameters : ofParameterGroup {
			ofParameter<int> ISO{ "ISO", 400 };
			ofParameter<float> aperture{ "Aperture", 5.6 };
			ofParameter<float> shutterSpeed{ "Shutter Speed", 1.0 / 30.0 };

			PARAM_DECLARE("Canon Camera Parameters", ISO, aperture, shutterSpeed);
		} parameters;
	};

	std::vector<std::shared_ptr<Device>> listDevices();
}
