#include "Canon.h"
#include "ofAppGLFWWindow.h"

namespace ofxMachineVision {
	namespace Device {

		//----------

		Canon::Canon() {

			this->frameIndex = 0;

			this->markFrameNew = false;

		}



		//----------

		string Canon::getTypeName() const {

			return "Canon";

		}



		//----------

		Specification Canon::open(shared_ptr<Base::InitialisationSettings> initialisationSettings) {

			this->camera = make_shared<ofxCanon::Simple>();

			this->camera->setLiveView(false);

			this->camera->setDeviceId(initialisationSettings->deviceID);

			if (!this->camera->setup()) {

				return Specification();

			}



			this->openTime = chrono::system_clock::now();

			this->frameIndex = 0;



			//--

			//single shot with timeout

			this->camera->takePhoto();

			float startTime = ofGetElapsedTimef();

			while (!this->camera->isPhotoNew()) {

				this->camera->update();

				glfwPollEvents();

				ofSleepMillis(1);

				if (ofGetElapsedTimef() - startTime > 30.0f) {

					throw(ofxMachineVision::Exception("Timeout opening device Canon. Check you have a memory card in your camera and that the exposure length is not too long."));

				}

			}

			this->markFrameNew = true;

			//

			//--



			const auto & pixels = this->camera->getPhotoPixels();

			Specification specification(CaptureSequenceType::OneShot

				, pixels.getWidth()

				, pixels.getHeight()

				, "Canon"

				, "Photo");



			return specification;

		}



		//----------

		void Canon::close() {

			this->camera.reset();

		}



		//----------

		void Canon::singleShot() {

			this->camera->takePhoto();

			auto startCapture = chrono::system_clock::now();

			auto maxDuration = chrono::minutes(1);

			while (!this->camera->isPhotoNew()) {

				this->camera->update();

				glfwPollEvents();

				ofSleepMillis(1);

				if (chrono::system_clock::now() - startCapture > maxDuration) {

					throw(ofxMachineVision::Exception("Timeout during capture"));

				}

			}

			this->markFrameNew = true;

		}



		//----------

		void Canon::updateIsFrameNew() {

			this->camera->update();

		}



		//----------

		bool Canon::isFrameNew() {

			if (this->markFrameNew) {

				this->markFrameNew = false;

				return true;

			}

			else {

				return this->camera->isPhotoNew();

			}

		}



		//----------

		shared_ptr<Frame> Canon::getFrame() {

			auto frame = FramePool::X().getAvailableFrameFilledWith(this->camera->getPhotoPixels());



			//timestamp

			{

				auto timeSinceOpen = chrono::system_clock::now() - this->openTime;

				frame->setTimestamp(timeSinceOpen);

			}

			frame->setFrameIndex(this->frameIndex++);

			return frame;

		}



		//----------

		shared_ptr<ofxCanon::Simple> Canon::getCamera() {

			return this->camera;

		}

	}

}