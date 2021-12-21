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

		bool takePhoto(bool autoFocus);
		
		ofImage& getImage();

		string getBaseURL() const;

		bool getShootingMode(string &) const;
		bool setShootingMode(const string&);

		bool getISO(int&) const;
		bool setISO(int);

		bool getAperture(float &) const;
		bool setAperture(float);

		bool getShutterSpeed(float &) const;
		bool setShutterSpeed(float);

		int convertISOFromDevice(string) const;
		string convertISOToDevice(int) const;

		float convertApertureFromDevice(string) const;
		string convertApertureToDevice(float) const;

		float convertShutterSpeedFromDevice(string) const;
		string convertShutterSpeedToDevice(float) const;

		struct {
			ofEvent<string> onShootingModeChange;
			ofEvent<float> onShutterSpeedChange;
			ofEvent<float> onApertureChange;
			ofEvent<int> onISOChange;
		} deviceEvents;
	protected:
		void poll();
		void getFileFromCamera(const string & address);

		nlohmann::json get(const string& address) const;
		nlohmann::json put(const string& address, const nlohmann::json&);
		vector<string> getOptions(const string& address) const;

		string hostname;

		DeviceInfo deviceInfo;
		ofURLFileLoader urlLoader;

		ofThreadChannel<ofBuffer> incomingImages;
		bool frameIsNew = false;
		ofImage image;

		bool waitingForPhoto = false;
	};
}