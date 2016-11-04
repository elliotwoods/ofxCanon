#include "Simple.h"
#include "Initializer.h"

namespace ofxCanon {

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

				while (!this->cameraThread->closeThread) {

					//receive incoming photo
					if (this->cameraThread->futurePhoto.valid()) {
						if (this->cameraThread->futurePhoto.wait_for(chrono::milliseconds(1)) == future_status::ready) {
							//photo has been received so load it
							auto result = this->cameraThread->futurePhoto.get();
							if (result.errorReturned == EDS_ERR_OK) {
								ofLoadImage(this->cameraThread->photoLoad, *result.encodedBuffer);

								//(rotate) and swap it into the chain
								{
									if (this->orientationMode != 0) {
										this->cameraThread->photoLoad.rotate90(this->orientationMode);
									}

									unique_lock<mutex> lock(this->cameraThread->photoMutex);
									swap(this->cameraThread->photoLoad, this->cameraThread->photo);
									this->cameraThread->photoIsNew = true;
								}
							}
							else {
								ofLogError("ofxCanon") << "Photo capture failed : " << errorToString(result.errorReturned);
							}
						}
					}

					//perform outstanding actions
					this->cameraThread->device->idleFunction();

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
		}
	}

	//----------
	bool Simple::isFrameNew() {
		return this->liveViewIsNew;
	}

	//----------
	unsigned int Simple::getWidth() const {
		return this->liveViewPixels.getWidth();
	}

	//----------
	unsigned int Simple::getHeight() const {
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
	void Simple::beginMovieRecording() {

	}

	//----------
	void Simple::endMovieRecording() {

	}

	//----------
	bool Simple::isMovieNew() {
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

}