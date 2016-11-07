#include "Canon.h"
#include "ofAppGLFWWindow.h"

#ifdef TARGET_WIN32
	#include "combaseapi.h"
#endif

namespace ofxMachineVision {
	namespace Device {
		//----------
		Canon::Canon() {

		}

		//----------
		string Canon::getTypeName() const {
			return "Canon";
		}

		//----------
		void Canon::initOnMainThread() {
			ofxCanon::Initializer::X();
			this->devices = ofxCanon::listDevices();
		}

		//----------
		Specification Canon::open(shared_ptr<Base::InitialisationSettings> initialisationSettings) {
#if defined(TARGET_WIN32)
			CoInitializeEx(NULL, 0x0); // COINIT_APARTMENTTHREADED in SDK docs
#endif
			try {
				this->device = this->devices.at(initialisationSettings->deviceID);
			}
			catch (const std::out_of_range &) {
				throw(ofxMachineVision::Exception("DeviceID #" + ofToString(initialisationSettings->deviceID) + " not available. " + ofToString(this->devices.size()) + " devices found."));
			}

			if (!device->open()) {
				throw(ofxMachineVision::Exception("Cannot open device"));
			}
			
			this->openTime = chrono::high_resolution_clock::now();
			this->frameIndex = 0;

			//take photo to get initial specs
			ofPixels pixels;
			this->device->takePhoto(pixels);

			Specification specification(pixels.getWidth(), pixels.getHeight(), device->getDeviceInfo().owner.manufacturer, device->getDeviceInfo().description);
			specification.addFeature(ofxMachineVision::Feature::Feature_OneShot);

			return specification;
		}

		//----------
		void Canon::close() {
			if (this->device) {
				this->device->close();
				this->device.reset();
			}
#if defined(TARGET_WIN32)
			CoUninitialize();
#endif
		}

		//----------
		void Canon::singleShot() {
			if (!this->frame) {
				this->frame = make_shared<Frame>();
			}

			this->frame->lockForWriting();
			{
				if (this->device->takePhoto(this->frame->getPixels())) {
					auto timeSinceOpen = chrono::high_resolution_clock::now() - this->openTime;
					frame->setTimestamp(chrono::duration_cast<chrono::microseconds>(timeSinceOpen).count());
					frame->setFrameIndex(this->frameIndex++);
					frame->setEmpty(false);
					this->markFrameNew = true;
				}
			}
			this->frame->unlock();
		}

		//----------
		void Canon::getFrame(shared_ptr<Frame> frame) {
			if (this->markFrameNew) {
				swap(* frame, * this->frame);
				this->markFrameNew = false;
			}
		}

		//----------
		void Canon::setExposure(Microseconds exposure) {
			throw std::logic_error("The method or operation is not implemented.");
		}

		//----------
		void Canon::setGain(float percent) {
			throw std::logic_error("The method or operation is not implemented.");
		}

		//----------
		void Canon::setFocus(float percent) {
			throw std::logic_error("The method or operation is not implemented.");
		}

		//----------
		shared_ptr<ofxCanon::Device> Canon::getDevice() {
			return this->device;
		}
	}
}