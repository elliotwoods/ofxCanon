#pragma once

#include "ofMain.h"
#include <future>

namespace ofxCanon {
	class RemoteDevice {
	public:
		struct AsyncPhotoResult {
			ofBuffer buffer;
		};

		struct DeviceInfo {
			string manufacturer;
			string productName;
			string guid;
			string serialNumber;
			string macAddress;
			string firmwareVersion;
		};

		RemoteDevice();
		bool setup(const string & hostname);
		
		DeviceInfo getDeviceInfo() const;

		void update();
		bool isFrameNew() const;

		void takePhoto(bool autoFocus);
		
		ofImage& getImage();

		string getBaseURL() const;

	protected:
		void poll();
		void getFileFromCamera(const string & address);

		string hostname;

		DeviceInfo deviceInfo;
		ofURLFileLoader urlLoader;

		ofThreadChannel<ofBuffer> incomingImages;
		bool frameIsNew = false;
		ofImage image;
	};
}