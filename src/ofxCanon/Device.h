#pragma once

#include "Constants.h"
#include "Handlers.h"
#include "ofMain.h"
#include "EDSDK.h"

#include <future>

#ifndef PARAM_DECLARE
	// Syntactic sugar which enables struct-ofParameterGroup
	#define PARAM_DECLARE(NAME, ...) bool paramDeclareConstructor \
	{ [this] { this->setName(NAME), this->add(__VA_ARGS__); return true; }() };
#endif

namespace ofxCanon {
	
	/*
		A blocking implementation of an EDSDK Camera Device.
		
		You can get a list of connected devices using the listDevices() method.

		All operations which happen on this class should come from the 'Camera thread'
		which is a thread that you own externally. That could be your main thread, or 
		any thread that you've created. You should regularly call Device::idleFunction()
		from that thread (e.g. in your update loop if you're using your main thread).

		Often you will want to wrap this class in something which handles threading
		and presents images to the screen in textures.
	*/
	class Device {
		public:
			struct DeviceInfo {
				string description;
				string port;

				struct Owner {
					string manufacturer;
					string owner;
					string artist;
					string copyright;
				} owner;

				struct Battery {
					float batteryLevel = 0.0f;
					float batteryQuality = 1.0f;
					bool psuPresent = false;
				} battery;
			};

			struct LensInfo {
				bool lensAttached = false;
				string lensName;
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
				shared_ptr<ofBuffer> encodedBuffer;
				shared_ptr<PhotoMetadata> metaData;
				EdsError errorReturned;

				operator bool() const {
					return this->errorReturned == EDS_ERR_OK;
				}
			};

			Device(EdsCameraRef);
			~Device();

			const DeviceInfo & getDeviceInfo() const;
			const LensInfo & getLensInfo() const; // Note this is only updated when photos are captured

			bool open();
			void close();
			void idleFunction();

			future<PhotoCaptureResult> takePhotoAsync();

			// take photo (e.g. with 8bit, 16bit pixels types)
			template<typename PixelsType>
			PhotoCaptureResult takePhoto(ofPixels_<PixelsType> & pixelsOut) {
				auto future = this->takePhotoAsync();
				while (future.wait_for(chrono::milliseconds(10)) != future_status::ready) {
					glfwPollEvents();
					this->idleFunction();
				}

				auto result = future.get();
				if (result.errorReturned == EDS_ERR_OK) {
					ofLoadImage(pixelsOut, *result.encodedBuffer);
				}
				return result;
			}

			CaptureStatus getCaptureStatus() const;

			ofParameterGroup & getParameters();

			vector<string> getOptions(const ofAbstractParameter &) const;
			vector<int> getISOOptions() const;
			vector<float> getApertureOptions() const;
			vector<float> getShutterSpeedOptions() const;

			int getISO() const;
			void setISO(int ISO);

			float getAperture() const;
			void setAperture(float aperture);

			float getShutterSpeed() const;
			void setShutterSpeed(float shutterSpeed);

			template<typename DataType>
			DataType getProperty(EdsPropertyID propertyID) {
				DataType value;
				CHECK_ERROR(EdsGetPropertyData(this->camera, propertyID, 0, sizeof(DataType), &value)
					, "Get property : " + propertyToString(propertyID));
			fail:
				return value;
			}

			template<typename DataType>
			vector<DataType> getPropertyArray(EdsPropertyID propertyID, size_t size) {
				vector<DataType> value(size);
				CHECK_ERROR(EdsGetPropertyData(this->camera, propertyID, 0, (EdsUInt32)(sizeof(DataType) * size), value.data())
					, "Get property array : " + propertyToString(propertyID));
				return value;
			fail:
				value.clear();
				return value;
			}

			template<typename DataType>
			bool setProperty(EdsPropertyID propertyID, DataType value) {
				CHECK_ERROR(EdsSetPropertyData(this->camera, propertyID, 0, sizeof(DataType), &value)
					, "Set property : " + propertyToString(propertyID));
				return true;
			fail:
				return false;
			}

			bool getLogDeviceCallbacks() const;
			void setLogDeviceCallbacks(bool);

			//these events will always fire in the camera thread
			ofEvent<EdsPropertyID> onParameterOptionsChange;
			ofEvent<LensInfo> onLensChange;
		protected:
			friend Handlers;

			typedef function<void()> Action;
			void performInCameraThread(Action &&);
			void performInCameraThreadBlocking(Action &&);
			std::thread::id cameraThreadId;

			void download(EdsDirectoryItemRef);
			void lensChanged();

			void pollProperty(EdsPropertyID);
			
			void callbackISOParameterChanged(int &);
			void callbackApertureParameterChanged(float &);
			void callbackShutterSpeedParameterChanged(float &);

			EdsCameraRef camera = NULL;
			bool isOpen = false;
			bool logDeviceCallbacks = false;
			bool hasDownloadedFirstPhoto = false;
			CaptureStatus captureStatus = CaptureStatus::NoCaptureTriggered;
			unique_ptr<promise<PhotoCaptureResult>> capturePromise;

			DeviceInfo deviceInfo;
			LensInfo lensInfo;

			mutex actionQueueMutex;
			queue<Action> actionQueue;

			mutex blockingActionMutex;
			Action blockingAction;

			struct Parameters : ofParameterGroup {
				ofParameter<int> ISO{ "ISO", 400 };
				ofParameter<float> aperture{ "Aperture", 5.6 };
				ofParameter<float> shutterSpeed{ "Shutter Speed", 1.0 / 30.0 };

				PARAM_DECLARE("Canon Camera Parameters", ISO, aperture, shutterSpeed);
			} parameters;
	};
	vector<shared_ptr<Device>> listDevices();
}