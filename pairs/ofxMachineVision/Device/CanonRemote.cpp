#include "CanonRemote.h"

namespace ofxMachineVision {
	namespace Device {
		//----------
		CanonRemote::InitialisationSettings::InitialisationSettings()
		{
			this->clear();
			this->add(this->hostname);
		}

		//----------
		CanonRemote::CanonRemote()
		{

		}

		//----------
		string
		CanonRemote::getTypeName() const
		{
			return "CanonRemote";
		}

		//----------
		shared_ptr<Base::InitialisationSettings>
			CanonRemote::getDefaultSettings() const
		{
			return make_shared<InitialisationSettings>();
		}

		//----------
		Specification
			CanonRemote::open(shared_ptr<Base::InitialisationSettings> initialisationSettings)
		{
			auto typedInitialisationSettings = dynamic_pointer_cast<InitialisationSettings>(initialisationSettings);

			if (!typedInitialisationSettings) {
				throw(ofxMachineVision::Exception("Invalid InitialisationSettings"));
			}

			this->device.setup(typedInitialisationSettings->hostname);
			this->device.getImage().setUseTexture(false);
			this->timeOpen = ofGetSystemTimeMicros();

			this->device.takePhoto(false);

			auto timeStart = ofGetElapsedTimeMillis();
			while (ofGetElapsedTimeMillis() < timeStart + 30000) {
				this->device.update();
				if (this->device.isFrameNew()) {
					break;
				}
			}

			if (!this->device.isFrameNew()) {
				throw(ofxMachineVision::Exception("Failed to open CanonRemote using CCAPI : timeout"));
			}

			Specification specification(CaptureSequenceType::OneShot
				, this->device.getImage().getWidth()
				, this->device.getImage().getHeight()
				, "Canon"
				, "Photo");

			return specification;
		}

		//----------
		void
			CanonRemote::close()
		{

		}

		//----------
		void
			CanonRemote::singleShot()
		{
			this->device.takePhoto(true);
		}

		//----------
		bool
			CanonRemote::isFrameNew()
		{
			return this->frameIsNew;
		}

		//----------
		void
			CanonRemote::updateIsFrameNew()
		{
			if (this->firstFrame) {
				this->firstFrame = false;
				this->frameIsNew = true;
			}
			else {
				this->device.update();
				this->frameIsNew = this->device.isFrameNew();
			}

			if (this->frameIsNew) {
				auto frame = FramePool::X().getAvailableFrameFilledWith(this->device.getImage());
				frame->setFrameIndex(this->frameIndex++);
				frame->setTimestamp(chrono::nanoseconds(1000 * (ofGetElapsedTimeMicros() - this->timeOpen)));
				this->frame = frame;
			}
		}

		//----------
		shared_ptr<Frame>
			CanonRemote::getFrame()
		{
			return this->frame;
		}
	}
}