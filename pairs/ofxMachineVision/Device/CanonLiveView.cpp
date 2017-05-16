#include "CanonLiveView.h"
#include "ofAppGLFWWindow.h"

namespace ofxMachineVision {
	namespace Device {
		//----------
		CanonLiveView::CanonLiveView() {
			this->frameIndex = 0;
			this->markFrameNew = false;
		}

		//----------
		string CanonLiveView::getTypeName() const {
			return "CanonLiveView";
		}

		//----------
		Specification CanonLiveView::open(shared_ptr<Base::InitialisationSettings> initialisationSettings) {
			this->camera = make_shared<ofxCanon::Simple>();
			this->camera->setLiveView(true);
			this->camera->setDeviceId(initialisationSettings->deviceID);
			if (!this->camera->setup()) {
				return Specification();
			}

			this->frameIndex = 0;

			auto startTime = chrono::system_clock::now();
			while (!this->camera->isLiveDataReady()) {
				auto now = chrono::system_clock::now();
				if (now - startTime > chrono::seconds(10)) {
					throw(ofxMachineVision::Exception("Timeout opening Canon live view"));
				}

				ofSleepMillis(10);
				this->camera->update();
			}

			const auto & pixels = this->camera->getLivePixels();
			Specification specification(CaptureSequenceType::Continuous
				, pixels.getWidth()
				, pixels.getHeight()
				, "CanonLiveView"
				, "Photo");

			this->openTime = chrono::high_resolution_clock::now();

			return specification;
		}

		//----------
		void CanonLiveView::close() {
			this->camera.reset();
		}

		//----------
		void CanonLiveView::updateIsFrameNew() {
			this->camera->update();
		}

		//----------
		bool CanonLiveView::isFrameNew() {
			return this->camera->isFrameNew();
		}

		//----------
		shared_ptr<Frame> CanonLiveView::getFrame() {
			auto & pixels = this->camera->getLivePixels();

			auto frame = FramePool::X().getAvailableFrameFilledWith(this->camera->getLivePixels());
			frame->setTimestamp(chrono::high_resolution_clock::now() - this->openTime);
			frame->setFrameIndex(this->frameIndex++);
			return frame;
		}

		//----------
		shared_ptr<ofxCanon::Simple> CanonLiveView::getCamera() {
			return this->camera;
		}
	}
}