#pragma once

#include "Handlers.h"
#include "ofMain.h"
#include "EDSDK.h"

#ifndef PARAM_DECLARE
	// Syntactic sugar which enables struct-ofParameterGroup
	#define PARAM_DECLARE(NAME, ...) bool paramDeclareConstructor \
	{ [this] { this->setName(NAME), this->add(__VA_ARGS__); return true; }() };
#endif

namespace ofxCanon {
	
	/*
		A blocking implementation of an EDSDK Camera Device.
		
		You can get a list of connected devices using the listDevices() method.

		Often you will want to wrap this class in something which handles threading
		and presenting the image to the screen, in which case all operations on the
		Device class should come from the same thread.
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
				float currentFocalLength = 0;
				float maximumFocalLength = 0;
				float minimumFocalLength = 0;
			};

			Device(EdsCameraRef);
			~Device();

			const DeviceInfo & getDeviceInfo() const;
			const LensInfo & getLensInfo() const; // Note this is only updated when photos are captured

			bool open();
			void close();
			void idleFunction();

			void takePicture();

			ofPixels getPhoto(); //<-- temporary, should go into takePhoto
			
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
			DataType getProperty(EdsPropertyID propertyID);
			template<typename DataType>
			vector<DataType> getPropertyArray(EdsPropertyID propertyID, size_t size);

			template<typename DataType>
			bool setProperty(EdsPropertyID propertyID, DataType value);

			ofEvent<EdsPropertyID> onParameterOptionsChange;
			ofEvent<LensInfo> onLensChange;
		protected:
			friend Handlers;

			typedef function<void()> Action;
			void performInCameraThread(Action &&);
			std::thread::id cameraThreadId;

			void download(EdsDirectoryItemRef);
			void lensChanged();

			void pollProperty(EdsPropertyID);
			
			void callbackISOParameterChanged(int &);
			void callbackApertureParameterChanged(float &);
			void callbackShutterSpeedParameterChanged(float &);

			EdsCameraRef camera = NULL;
			bool isOpen = false;

			DeviceInfo deviceInfo;
			LensInfo lensInfo;

			mutex photoMutex;
			ofPixels photo;

			mutex actionQueueMutex;
			queue<Action> actionQueue;

			struct Parameters : ofParameterGroup {
				ofParameter<int> ISO{ "ISO", 400 };
				ofParameter<float> aperture{ "Aperture", 5.6 };
				ofParameter<float> shutterSpeed{ "Shutter Speed", 1.0 / 30.0 };

				PARAM_DECLARE("Canon Camera Parameters", ISO, aperture, shutterSpeed);
			} parameters;
	};
	vector<shared_ptr<Device>> listDevices();
}