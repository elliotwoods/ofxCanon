#include "Simple.h"
#include "Initializer.h"

#include "ofImage.h"

#ifdef TARGET_WIN32
	#include "combaseapi.h"
#endif

using namespace std;

namespace ofxCanon {
#pragma mark CameraThread
	//----------
	void Simple::CameraThread::lensChangeCallback(Device::LensInfo & lensInfo) {
		this->lensIsNew = true;
	}

	//----------
	void Simple::CameraThread::parameterChangeCallback(EdsPropertyID& propertyID) {
		switch (propertyID) {
		case kEdsPropID_ISOSpeed:
			this->ISOIsNew = true;
			break;
		case kEdsPropID_Av:
			this->apertureIsNew = true;
			break;
		case kEdsPropID_Tv:
			this->shutterSpeedIsNew = true;
			break;
		default:
			break;
		}
	}

#pragma mark Simple
	//----------
	Simple::~Simple() {
		this->close();
	}

	//----------
	void Simple::setDeviceId(int deviceId) {
		this->deviceId = deviceId;
	}

	//----------
	void Simple::setOrientationMode(int orientationMode) {
		this->orientationMode = orientationMode;
	}

	//----------
	void Simple::setLiveView(bool useLiveView) {
		this->useLiveView = useLiveView;
	}

	//----------
	bool Simple::setup() {
		if (this->deviceId < 0) {
			this->deviceId = 0;
		}

		//Initializer must be called from main thread so that callbacks come through glfw poll function
		ofxCanon::Initializer::X();

		ofThreadChannel<bool> reportSuccess;
		
		this->cameraThread = make_shared<CameraThread>();
		this->cameraThread->thread = thread([this, &reportSuccess]() {

#if defined(TARGET_WIN32)
			CoInitializeEx(NULL, 0x0); // COINIT_APARTMENTTHREADED in SDK docs
#endif

			auto devices = ofxCanon::listDevices();
			if (this->deviceId >= devices.size()) {
				ofLogError("ofxCanon") << "Device index " << this->deviceId << " does not exist";
				reportSuccess.send(false);
				return false;
			}
			this->cameraThread->device = devices[deviceId];

			bool success = this->cameraThread->device->open();

			//report back to main thread
			reportSuccess.send(success);

			//in this thread, start operating the camera
			if (success) {
				if (this->useLiveView) {
					this->cameraThread->device->setLiveViewEnabled(true);
				}

				ofAddListener(this->cameraThread->device->onLensChange, this->cameraThread.get(), &CameraThread::lensChangeCallback);
				ofAddListener(this->cameraThread->device->onParameterOptionsChange, this->cameraThread.get(), &CameraThread::parameterChangeCallback);
				ofAddListener(this->cameraThread->device->onUnrequestedPhotoReceived, this, &Simple::callbackUnrequestedPhotoReceived);

				while (!this->cameraThread->closeThread) {
					//receive incoming photo
					if (this->cameraThread->futurePhoto.valid()) {
						if (this->cameraThread->futurePhoto.wait_for(chrono::milliseconds(1)) == future_status::ready) {
							//photo has been received so load it
							auto result = this->cameraThread->futurePhoto.get();
							this->processCaptureResult(result);
						}
					}

					//receive unrequested photos
					{
						Device::PhotoCaptureResult unrequestedPhoto;
						while (this->unrequestedPhotosIncoming.tryReceive(unrequestedPhoto)) {
							this->processCaptureResult(unrequestedPhoto);
						}
					}

					//perform outstanding actions
					this->cameraThread->device->update();

					//grab live view frame
					if (this->useLiveView) {
						if (this->cameraThread->device->getLiveView(this->cameraThread->liveViewLoad)) {
							if (this->orientationMode != 0) {
								this->cameraThread->liveView.rotate90(this->orientationMode);
							}

							unique_lock<mutex> lock(this->cameraThread->liveViewMutex);
							swap(this->cameraThread->liveViewLoad, this->cameraThread->liveView);
							this->liveViewFramerateCounter.addFrame();
							this->cameraThread->liveViewIsNew = true;
						}
					}

					//from ofxEdsdk
					ofSleepMillis(5);
				}

				ofRemoveListener(this->cameraThread->device->onLensChange, this->cameraThread.get(), &CameraThread::lensChangeCallback);

				this->cameraThread->device->close();
			}

#if defined(TARGET_WIN32)
			CoUninitialize();
#endif
		});
		
		bool success;
		reportSuccess.receive(success);
		if (!success) {
			this->close();
		}
		return success;
	}
	
	//----------
	void Simple::close() {
		if (this->cameraThread) {
			this->cameraThread->closeThread = true;
			if (this->cameraThread->thread.joinable()) {
				this->cameraThread->thread.join();
			}
		}
	}

	//----------
	void Simple::update() {
		if (this->cameraThread) {
			this->liveViewFramerateCounter.update();

			this->photoIsNew = false;
			this->liveViewIsNew = false;

			{
				unique_lock<mutex> lock(this->cameraThread->photoMutex);
				if (this->cameraThread->photoIsNew) {
					swap(this->photoPixels, cameraThread->photo);
					this->photoTexture.loadData(this->photoPixels);
					this->photoIsNew = true;
					this->cameraThread->photoIsNew = false;
				}
			}

			if(this->useLiveView) {
				unique_lock<mutex> lock(this->cameraThread->liveViewMutex);
				if (this->cameraThread->liveViewIsNew) {
					swap(this->liveViewPixels, cameraThread->liveView);
					this->liveViewTexture.loadData(this->liveViewPixels);
					this->liveViewIsNew = true;
					this->cameraThread->liveViewIsNew = false;
				}
			}

			{
				this->lensIsNew = this->cameraThread->lensIsNew;
				this->apertureIsNew = this->cameraThread->apertureIsNew;
				this->ISOIsNew = this->cameraThread->ISOIsNew;
				this->shutterSpeedIsNew = this->cameraThread->shutterSpeedIsNew;

				this->cameraThread->lensIsNew = false;
				this->cameraThread->apertureIsNew = false;
				this->cameraThread->ISOIsNew = false;
				this->cameraThread->shutterSpeedIsNew = false;
			}
		}
	}

	//----------
	bool Simple::isFrameNew() {
		return this->liveViewIsNew;
	}

	//----------
	size_t Simple::getWidth() const {
		return this->liveViewPixels.getWidth();
	}

	//----------
	size_t Simple::getHeight() const {
		return this->liveViewPixels.getHeight();
	}

	//----------
	bool Simple::isLiveDataReady() const {
		return this->liveViewPixels.isAllocated();
	}

	//----------
	void Simple::draw(float x, float y) {
		if (this->liveViewTexture.isAllocated()) {
			this->liveViewTexture.draw(x, y);
		}
	}

	//----------
	void Simple::draw(float x, float y, float width, float height) {
		if (this->liveViewTexture.isAllocated()) {
			this->liveViewTexture.draw(x, y, width, height);
		}
	}

	//----------
	ofPixels& Simple::getLivePixels() {
		return this->liveViewPixels;
	}

	//----------
	ofTexture& Simple::getLiveTexture() {
		return liveViewTexture;
	}

	//----------
	float Simple::getFrameRate() {
		if (this->cameraThread) {
			return this->liveViewFramerateCounter.getFrameRate();
		}
		else {
			return 0;
		}
	}

	//----------
	float Simple::getBandwidth() {
		return 0;
	}

	//----------
	void Simple::takePhoto(bool blocking) {
		if (this->cameraThread) {
			if (blocking) {
				//perform sync take photo call
				this->cameraThread->device->performInCameraThread([this]() {
					if (this->cameraThread->device->takePhoto(this->cameraThread->photo).errorReturned == EDS_ERR_OK) {
						this->cameraThread->photoIsNew = true;
					}
				});
			}
			else {
				//perform async take photo call
				this->cameraThread->device->performInCameraThread([this]() {
					this->cameraThread->futurePhoto = this->cameraThread->device->takePhotoAsync();
				});
			}
		}
	}

	//----------
	bool Simple::isPhotoNew() {
		return this->photoIsNew;
	}

	//----------
	void Simple::drawPhoto(float x, float y) {
		if (this->photoTexture.isAllocated()) {
			this->photoTexture.draw(x, y);
		}
	}

	//----------
	void Simple::drawPhoto(float x, float y, float width, float height) {
		if (this->photoTexture.isAllocated()) {
			this->photoTexture.draw(x, y, width, height);
		}
	}

	//----------
	bool Simple::savePhoto(string filename) {
		if (!this->photoPixels.isAllocated()) {
			return false;
		}
		ofSaveImage(this->photoPixels, filename);
		return true;
	}

	//----------
	ofPixels& Simple::getPhotoPixels() {
		return this->photoPixels;
	}

	//----------
	ofTexture& Simple::getPhotoTexture() {
		return this->photoTexture;
	}

	//----------
	const Device::PhotoCaptureResult& Simple::getPhotoCaptureResult() const {
		return this->photoCaptureResult;
	}

	//----------
	void Simple::beginMovieRecording() {
		// Currently unsupported sorry
	}

	//----------
	void Simple::endMovieRecording() {
		// Currently unsupported sorry
	}

	//----------
	bool Simple::isMovieNew() {
		// Currently unsupported sorry
		return false;
	}

	//----------
	bool Simple::isConnected() {
		if (this->cameraThread) {
			return true;
		}
		else {
			return false;
		}
	}

	//----------
	bool Simple::isLensNew() const {
		return this->lensIsNew;
	}

	//----------
	bool Simple::isApertureNew() const {
		return this->apertureIsNew;
	}

	//----------
	bool Simple::isISONew() const {
		return this->ISOIsNew;
	}

	//----------
	bool Simple::isShutterSpeedNew() const {
		return this->shutterSpeedIsNew;
	}

	//----------
	shared_ptr<Simple::CameraThread> Simple::getCameraThread() {
		return this->cameraThread;
	}

	//----------
	void Simple::callbackUnrequestedPhotoReceived(Device::PhotoCaptureResult& photoCaptureResult) {
		this->unrequestedPhotosIncoming.send(photoCaptureResult);
	}

	//----------
	void Simple::processCaptureResult(const Device::PhotoCaptureResult& photoCaptureResult) {
		if (photoCaptureResult.errorReturned == EDS_ERR_OK) {
			ofLoadImage(this->cameraThread->photoLoad
				, *photoCaptureResult.encodedBuffer);

			//(rotate) and swap it into the chain
			{
				if (this->orientationMode != 0) {
					this->cameraThread->photoLoad.rotate90(this->orientationMode);
				}

				unique_lock<mutex> lock(this->cameraThread->photoMutex);
				swap(this->cameraThread->photoLoad, this->cameraThread->photo);
				this->cameraThread->photoIsNew = true;
			}

			this->photoCaptureResult = photoCaptureResult;
		}
		else {
			ofLogError("ofxCanon") << "Photo capture failed : " << errorToString(photoCaptureResult.errorReturned);
		}
	}
}