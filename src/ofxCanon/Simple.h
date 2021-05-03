#pragma once

#include "Device.h"

#include "ofTexture.h"

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace ofxCanon {
	//The Simple class conceals thread complexity away from the user of ofxCanon (mirrors ofxEdsdk interface).
	class Simple {
	public:
		struct CameraThread {
			void lensChangeCallback(Device::LensInfo & lensInfo);

			std::shared_ptr<Device> device;
			std::thread thread;


			ofPixels photoLoad;
			ofPixels photo;
			bool photoIsNew = false;
			std::mutex photoMutex;

			ofPixels liveViewLoad;
			ofPixels liveView;
			bool liveViewIsNew = false;
			std::mutex liveViewMutex;

			std::future<Device::PhotoCaptureResult> futurePhoto;

			bool lensIsNew = false;
			bool closeThread = false;

			std::queue<std::function<void()>> actionQueue;
		};

		//We directly mirror the interface of ofxEdsdk as of August 31st 2016, changes:
		// * setup returns a bool
		// * close returns a void
		
		//Not supported:
		// * getBandwidth
		// * movie recording

		virtual ~Simple();

		void setDeviceId(int deviceId);
		void setOrientationMode(int orientationMode);
		void setLiveView(bool useLiveView);
		bool setup();
		void close();

		void update();
		bool isFrameNew();
		size_t getWidth() const;
		size_t getHeight() const;
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
		bool savePhoto(std::string filename); // .jpg only
		ofPixels & getPhotoPixels();
		ofTexture & getPhotoTexture();

		const Device::PhotoCaptureResult& getPhotoCaptureResult() const;

		void beginMovieRecording();
		void endMovieRecording();
		bool isMovieNew();

		bool isConnected();

		bool isLensNew() const;

		std::shared_ptr<CameraThread> getCameraThread();
	protected:
		void callbackUnrequestedPhotoReceived(Device::PhotoCaptureResult&);
		void processCaptureResult(const Device::PhotoCaptureResult&);
		int deviceId = 0;
		int orientationMode = 0;
		bool useLiveView = true;

		std::shared_ptr<CameraThread> cameraThread;
		ofThreadChannel<Device::PhotoCaptureResult> unrequestedPhotosIncoming;

		ofPixels photoPixels;
		ofTexture photoTexture;
		bool photoIsNew = false;

		ofPixels liveViewPixels;
		ofTexture liveViewTexture;
		bool liveViewIsNew = false;
		FramerateCounter liveViewFramerateCounter;

		bool lensIsNew = false;

		Device::PhotoCaptureResult photoCaptureResult;
	};
}