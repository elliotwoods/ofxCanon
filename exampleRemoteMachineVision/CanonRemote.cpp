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
			return this->device.isFrameNew();
		}

		//----------
		void
			CanonRemote::updateIsFrameNew()
		{
			this->device.update();
			if (this->device.isFrameNew()) {
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