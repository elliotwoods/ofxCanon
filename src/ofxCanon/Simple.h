#pragma once

#include "Device.h"

#include <thread>
#include <mutex>
#include <condition_variable>

namespace ofxCanon {
	//The Simple class conceals thread complexity away from the user of ofxCanon (mirrors ofxEdsdk interface).
	class Simple {
	public:
		//We directly mirror the interface of ofxEdsdk as of August 31st 2016, changes:
		// * setup returns a bool
		// * close returns a void
		
		//Not supported:
		// * getBandwidth
		// * movie recording
		// * taking photos using the camera button trigger

		virtual ~Simple();

		void setDeviceId(int deviceId);
		void setOrientationMode(int orientationMode);
		void setLiveView(bool useLiveView);
		bool setup();
		void close();

		void update();
		bool isFrameNew();
		unsigned int getWidth() const;
		unsigned int getHeight() const;
		bool isLiveDataReady() const;
		void draw(float x, float y);
		void draw(float x, float y, float width, float height);
		ofPixels& getLivePixels();
		ofTexture& getLiveTexture();
		float getFrameRate();
		float getBandwidth();

		void takePhoto(bool blocking = false);
		bool isPhotoNew();
		void drawPhoto(float x, float y);
		void drawPhoto(float x, float y, float width, float height);
		bool savePhoto(string filename); // .jpg only
		ofPixels& getPhotoPixels();
		ofTexture& getPhotoTexture();

		void beginMovieRecording();
		void endMovieRecording();
		bool isMovieNew();

		bool isConnected();

	protected:
		struct CameraThread {
			shared_ptr<Device> device;
			std::thread thread;


			ofPixels photoLoad;
			ofPixels photo;
			bool photoIsNew = false;
			mutex photoMutex;

			ofPixels liveViewLoad;
			ofPixels liveView;
			bool liveViewIsNew = false;
			mutex liveViewMutex;

			future<Device::PhotoCaptureResult> futurePhoto;

			bool closeThread = false;
		};

		int deviceId = 0;
		int orientationMode = 0;
		bool useLiveView = true;

		shared_ptr<CameraThread> cameraThread;

		ofPixels photoPixels;
		ofTexture photoTexture;
		bool photoIsNew = false;

		ofPixels liveViewPixels;
		ofTexture liveViewTexture;
		bool liveViewIsNew = false;
		FramerateCounter liveViewFramerateCounter;
	};
}