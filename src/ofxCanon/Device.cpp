#include "Device.h"

#include "Initializer.h"

namespace ofxCanon {
	//----------
	Device::Device(EdsCameraRef camera) {
		this->camera = camera;
		if (camera == NULL) {
			goto fail;
		}
		else {
			{
				EdsDeviceInfo deviceInfo;
				CHECK_ERROR(EdsGetDeviceInfo(camera, &deviceInfo)
					, "Get device info");
				this->deviceInfo.description = string(deviceInfo.szDeviceDescription);
				this->deviceInfo.port = string(deviceInfo.szPortName);
			}

			CHECK_ERROR(EdsSetPropertyEventHandler(camera, kEdsPropertyEvent_All, Handlers::handlePropertyEvent, (EdsVoid *) this)
				, "Set property event handler");
			CHECK_ERROR(EdsSetObjectEventHandler(camera, kEdsObjectEvent_All, Handlers::handleObjectEvent, (EdsVoid *) this)
				, "Set object event handler");
			CHECK_ERROR(EdsSetCameraStateEventHandler(camera, kEdsStateEvent_All, Handlers::handleStateEvent, (EdsVoid *) this)
				, "Set state event handler");
		}

		this->parameters.ISO.addListener(this, &Device::callbackISOParameterChanged);
		this->parameters.aperture.addListener(this, &Device::callbackApertureParameterChanged);
		this->parameters.shutterSpeed.addListener(this, &Device::callbackShutterSpeedParameterChanged);
		
	fail:
		return;
	}

	//----------
	Device::~Device() {
		this->close();

		if (this->camera != NULL) {
			EdsRelease(this->camera);
		}
	}

	//----------
	const Device::DeviceInfo & Device::getDeviceInfo() const {
		return this->deviceInfo;
	}

	//----------
	const Device::LensInfo & Device::getLensInfo() const {
		return this->lensInfo;
	}

	//----------
	bool Device::open() {
		this->close();

		this->cameraThreadId = std::this_thread::get_id();

		CHECK_ERROR(EdsOpenSession(this->camera)
			, "Open session");

		//Set the save-to location to host
		{
			EdsUInt32 saveTo = kEdsSaveTo_Host;
			CHECK_ERROR(EdsSetPropertyData(this->camera, kEdsPropID_SaveTo, 0, sizeof(saveTo), &saveTo)
				, "Set save to host");
		}

		//Temporarily lock the UI
		CHECK_ERROR(EdsSendStatusCommand(this->camera, kEdsCameraStatusCommand_UILock, 0)
			, "Lock the camera UI");

		//Tell the camera that there's plenty of capacity available on the host (used to display shots remaining)
		{
			EdsCapacity capacity = { 0x7FFFFFFF, 0x1000, 1 };
			CHECK_ERROR(EdsSetCapacity(this->camera, capacity)
				, "Notify the camera of available storage on host device");
		}

		//Unlock the camera UI
		CHECK_ERROR(EdsSendStatusCommand(this->camera, kEdsCameraStatusCommand_UIUnLock, 0)
			, "Unlock the camera UI");

		this->isOpen = true;
		return true;

	fail:
		return false;
	}

	//----------
	void Device::close() {
		if (this->isOpen) {
			CHECK_ERROR(EdsCloseSession(this->camera)
				, "Close session");
		}
	fail:
		return;
	}

	//----------
	void Device::idleFunction() {
		//perform any immediate blocking action
		{
			unique_lock<mutex> lock(this->blockingActionMutex);
			if (this->blockingAction) {
				this->blockingAction();
				this->blockingAction = Action(); // clear action
			}
		}

		//perform outstanding actions in queue
		{
			unique_lock<mutex> lock(this->actionQueueMutex);
			while (!actionQueue.empty()) {
				actionQueue.front()();
				actionQueue.pop();
			}
		}
	 
	}

	//----------
	future<Device::PhotoCaptureResult> Device::takePhotoAsync() {
		EdsError error = EDS_ERR_OK;

		if (this->captureStatus == CaptureStatus::WaitingForPhotoDownload) {
			//we're already capturing, return a failed capture future
			error = EDS_ERR_DEVICE_BUSY;
		}
		else {
			this->captureStatus = CaptureStatus::WaitingForPhotoDownload;

			//press shutter
			{
				error = EdsSendCommand(this->camera, kEdsCameraCommand_TakePicture, 0);
				if (error != EDS_ERR_OK) {
					goto failNewCapture;
				}
			}

			this->capturePromise = make_unique<promise<PhotoCaptureResult>>();
			return this->capturePromise->get_future();

		failNewCapture:
			this->captureStatus = CaptureStatus::CaptureFailed;
			goto fail;
		}

	fail:
		//return failed future
		promise<PhotoCaptureResult> promiseWeCantKeep;
		auto future = promiseWeCantKeep.get_future();
		PhotoCaptureResult result = {
			nullptr,
			nullptr,
			error
		};
		promiseWeCantKeep.set_value(result);
		return future;
	}

	//----------
	Device::CaptureStatus Device::getCaptureStatus() const {
		return this->captureStatus;
	}

	//----------
	ofParameterGroup & Device::getParameters() {
		return this->parameters;
	}

	//----------
	vector<string> Device::getOptions(const ofAbstractParameter & parameter) const {
		vector<string> optionsStrings;

		if (&parameter == &this->parameters.ISO) {
			const auto & options = this->getISOOptions();
			for (const auto & option : options) {
				optionsStrings.push_back(ofToString(option));
			}
		}
		else if (&parameter == &this->parameters.aperture) {
			const auto & options = this->getApertureOptions();
			for (const auto & option : options) {
				optionsStrings.push_back(ofToString(option));
			}
		}
		else if (&parameter == &this->parameters.shutterSpeed) {
			const auto & options = this->getShutterSpeedOptions();
			for (const auto & option : options) {
				optionsStrings.push_back(ofToString(option));
			}
		}

		return optionsStrings;
	}

	//----------
	void Device::performInCameraThread(Action && action) {
		if (std::this_thread::get_id() == this->cameraThreadId) {
			// We're already in the camera thread
			action();
		}
		else {
			// We're calling from another thread, queue the action for later
			unique_lock<mutex> lock(this->actionQueueMutex);
			this->actionQueue.emplace(action);
		}
	}

	//----------
	void Device::performInCameraThreadBlocking(Action && action) {
		if (this_thread::get_id() == this->cameraThreadId) {
			action();
		}
		else {
			while (true) {
				{
					unique_lock<mutex> lock(this->blockingActionMutex);
					if (!this->blockingAction) {
						//no blocking action waiting
						this->blockingAction = action;
						break;
					}
				}

				//already have a blocking action waiting before this one
				this_thread::sleep_for(chrono::milliseconds(10));
			}

			while (true) {
				unique_lock<mutex> lock(this->blockingActionMutex);
				if (!this->blockingAction) {
					break;
				}

				//waiting for our blocking action to complete
				this_thread::sleep_for(chrono::milliseconds(10));
			}
		}
	}

	//----------
	void Device::download(EdsDirectoryItemRef directoryItem) {
		PhotoCaptureResult photoCaptureAsyncResult;
		try {
			EdsDirectoryItemInfo directoryItemInfo;
			EdsStreamRef encodedStream = NULL;

			THROW_ERROR(EdsGetDirectoryItemInfo(directoryItem, &directoryItemInfo)
				, "Get directory item info");

			//download image
			{
				THROW_ERROR(EdsCreateMemoryStream(0, &encodedStream)
					, "Create memory stream for encoded image");
				THROW_ERROR(EdsDownload(directoryItem, directoryItemInfo.size, encodedStream)
					, "Download directory item");
				THROW_ERROR(EdsDownloadComplete(directoryItem)
					, "Download complete");
				THROW_ERROR(EdsDeleteDirectoryItem(directoryItem)
					, "Delete directory item");
			}

			// NOTE : The Canon SDK does not provide decoding of RAW images for all its cameras, especially in 64bit
			// Therefore we use freeimage to perform this function (i.e. openFrameworks' built in functions),
			//	which do not offer functions such as getting the lens properties.

			//decode stream
			shared_ptr<ofBuffer> buffer;
			{
				EdsUInt64 streamLength;
				THROW_ERROR(EdsGetLength(encodedStream, &streamLength)
					, "Get encoded stream length (i.e. file size)");
				char * encodedData = NULL;
				THROW_ERROR(EdsGetPointer(encodedStream, (EdsVoid**)& encodedData)
					, "Get pointer to encoded data");
				buffer = make_shared<ofBuffer>(encodedData, streamLength);
			}

			//attempt to read metadata from the image reference
			EdsImageRef imageRef = NULL;
			shared_ptr<PhotoMetadata> metaData;
			{
				try {
					THROW_ERROR(EdsCreateImageRef(encodedStream, &imageRef)
						, "Create image reference from incoming stream for metadata purposes (Unsupported for CR2 on some camera models especially in x64)");
					vector<EdsRational> value(3);
					THROW_ERROR(EdsGetPropertyData(imageRef, kEdsPropID_FocalLength, 0, sizeof(EdsRational) * 3, value.data())
						, "Get focal length data from image");
					metaData = make_shared<PhotoMetadata>();
					metaData->focalLength.minimumFocalLength = rationalToFloat(value[2]);
					metaData->focalLength.currentFocalLength = rationalToFloat(value[0]);
					metaData->focalLength.maximumFocalLength = rationalToFloat(value[1]);
				}
				catch (EdsError errorCode) {
					//this generally happens when the image is RAW and the SDK can't process it
				}
			}

			//release the objects
			THROW_ERROR(EdsRelease(encodedStream)
				, "Release encoded stream");

			this->hasDownloadedFirstPhoto = true;
			this->captureStatus = CaptureStatus::CaptureSucceeded;

			//if we've got an async listener waiting for a photo
			if (this->capturePromise) {
				photoCaptureAsyncResult.errorReturned = EDS_ERR_OK;
				photoCaptureAsyncResult.encodedBuffer = buffer;
				photoCaptureAsyncResult.metaData = metaData;
			}
		}
		catch (EdsError error) {
			this->captureStatus = CaptureStatus::CaptureFailed;

			//if we've got an async listener waiting for a photo
			if (this->capturePromise) {
				photoCaptureAsyncResult.errorReturned = error;
			}
		}

		this->capturePromise->set_value(photoCaptureAsyncResult);
		this->capturePromise.reset();
	}

	//----------
	void Device::lensChanged() {
		auto lensStatus = this->getProperty<EdsUInt32>(kEdsPropID_LensStatus);
		
		if (lensStatus == 0) {
			//no lens present
			if (this->lensInfo.lensAttached) {
				//lens removed
				this->lensInfo = LensInfo();
				this->onLensChange.notify(this->lensInfo);
			}
		}
		else {
			//lens present
			this->lensInfo.lensAttached = true;
			this->onLensChange.notify(this->lensInfo);
		}
	}

	//----------
	template<typename Type>
	void setWithoutEvents(ofParameter<Type> & parameter, const Type & value) {
		parameter.disableEvents();
		parameter.set(value);
		parameter.enableEvents();
	}

	//----------
	void Device::pollProperty(EdsPropertyID propertyID) {
		switch (propertyID) {
		case kEdsPropID_ISOSpeed:
		{
			auto encoded = this->getProperty<EdsUInt32>(kEdsPropID_ISOSpeed);
			setWithoutEvents(this->parameters.ISO, decodeISO(encoded));
			break;
		}
		case kEdsPropID_Av:
		{
			auto encoded = this->getProperty<EdsUInt32>(kEdsPropID_Av);
			setWithoutEvents(this->parameters.aperture, decodeAperture(encoded));
			break;
		}
		case kEdsPropID_Tv:
		{
			auto encoded = this->getProperty<EdsUInt32>(kEdsPropID_Tv);
			setWithoutEvents(this->parameters.shutterSpeed, decodeShutterSpeed(encoded));
			break;
		}

		case kEdsPropID_LensStatus:
		{
			this->lensChanged();
			break;
		}
		case kEdsPropID_LensName:
		{
			auto nameString = this->getPropertyArray<EdsChar>(kEdsPropID_LensName, 100);
			this->lensInfo.lensName = string(nameString.begin(), nameString.end());
			break;
		}

		case kEdsPropID_BatteryLevel:
		{
			auto encoded = this->getProperty<EdsUInt32>(kEdsPropID_BatteryLevel);
			if (encoded == 0xffffffff) {
				//AC Power connected
				this->deviceInfo.battery.batteryLevel = 1.0f;
				this->deviceInfo.battery.batteryQuality = 1.0f;
				this->deviceInfo.battery.psuPresent = true;
			}
			else {
				//Battery operated
				this->deviceInfo.battery.batteryLevel = (float)encoded / 100.0f;
				this->deviceInfo.battery.psuPresent = false;
			}
			break;
		}
		case kEdsPropID_BatteryQuality:
		{
			auto encoded = this->getProperty<EdsUInt32>(kEdsPropID_BatteryQuality);
			this->deviceInfo.battery.batteryQuality = (float)(encoded + 1) / 4.0f;
			break;
		}

		case kEdsPropID_MakerName:
		{
			auto nameString = this->getPropertyArray<EdsChar>(kEdsPropID_MakerName, 100);
			this->deviceInfo.owner.manufacturer = string(nameString.begin(), nameString.end());
			break;
		}
		case kEdsPropID_OwnerName:
		{
			auto nameString = this->getPropertyArray<EdsChar>(kEdsPropID_OwnerName, 100);
			this->deviceInfo.owner.owner = string(nameString.begin(), nameString.end());
			break;
		}
		case kEdsPropID_Artist:
		{
			auto nameString = this->getPropertyArray<EdsChar>(kEdsPropID_Artist, 100);
			this->deviceInfo.owner.artist = string(nameString.begin(), nameString.end());
			break;
		}
		case kEdsPropID_Copyright:
		{
			auto nameString = this->getPropertyArray<EdsChar>(kEdsPropID_Copyright, 100);
			this->deviceInfo.owner.copyright = string(nameString.begin(), nameString.end());
			break;
		}

		default:
			break;
		}
	}

	//----------
	void Device::callbackISOParameterChanged(int & ISO) {
		auto encoded = encodeISO(ISO);
		if (encoded != 0xFFFFFFFF) {
			this->setProperty(kEdsPropID_ISOSpeed, encoded);
		}
	}

	//----------
	void Device::callbackApertureParameterChanged(float & aperture) {
		auto encoded = encodeAperture(aperture);
		if (encoded != 0xFFFFFFFF) {
			this->setProperty(kEdsPropID_Av, encoded);
		}
	}

	//----------
	void Device::callbackShutterSpeedParameterChanged(float & shutterSpeed) {
		auto encoded = encodeShutterSpeed(shutterSpeed);
		if (encoded != 0xFFFFFFFF) {
			this->setProperty(kEdsPropID_Tv, encoded);
		}
	}

	//----------
	vector<int> Device::getISOOptions() const {
		vector<int> options;

		EdsPropertyDesc propertyDescription;

		CHECK_ERROR(EdsGetPropertyDesc(this->camera, kEdsPropID_ISOSpeed, &propertyDescription)
			, "Get property description");

		for (int i = 0; i < propertyDescription.numElements; i++) {
			options.push_back(decodeISO(propertyDescription.propDesc[i]));
		}

	fail:
		return options;
	}

	//----------
	vector<float> Device::getApertureOptions() const {
		vector<float> options;

		EdsPropertyDesc propertyDescription;

		CHECK_ERROR(EdsGetPropertyDesc(this->camera, kEdsPropID_Av, &propertyDescription)
			, "Get property description");

		for (int i = 0; i < propertyDescription.numElements; i++) {
			options.push_back(decodeAperture(propertyDescription.propDesc[i]));
		}

	fail:
		return options;
	}

	//----------
	vector<float> Device::getShutterSpeedOptions() const {
		vector<float> options;

		EdsPropertyDesc propertyDescription;

		CHECK_ERROR(EdsGetPropertyDesc(this->camera, kEdsPropID_Tv, &propertyDescription)
			, "Get property description");

		for (int i = 0; i < propertyDescription.numElements; i++) {
			options.push_back(decodeShutterSpeed(propertyDescription.propDesc[i]));
		}

	fail:
		return options;
	}

	//----------
	int Device::getISO() const {
		return this->parameters.ISO;
	}

	//----------
	void Device::setISO(int ISO) {
		this->parameters.ISO = ISO;
	}

	//----------
	float Device::getAperture() const {
		return this->parameters.aperture;
	}

	//----------
	void Device::setAperture(float aperture) {
		this->parameters.aperture = aperture;
	}

	//----------
	float Device::getShutterSpeed() const {
		return this->parameters.shutterSpeed;
	}

	//----------
	void Device::setShutterSpeed(float shutterSpeed) {
		this->parameters.shutterSpeed = shutterSpeed;
	}

	//----------
	bool Device::getLogDeviceCallbacks() const {
		return this->logDeviceCallbacks;
	}

	//----------
	void Device::setLogDeviceCallbacks(bool logDeviceCallbacks) {
		this->logDeviceCallbacks = logDeviceCallbacks;
	}

	//----------
	//Referenced from CameraControl.cpp in sample of v0304W of EDSDK
	vector<shared_ptr<Device>> listDevices() {
		vector<shared_ptr<Device>> devices;

		//ensure that EDSDK is initialized
		ofxCanon::Initializer::X();

		EdsCameraListRef cameraList;
		CHECK_ERROR(EdsGetCameraList(&cameraList)
			, "Get camera list");

		EdsUInt32 cameraCount;
		CHECK_ERROR(EdsGetChildCount(cameraList, &cameraCount)
			, "Get camera count");

		
		for (int i = 0; i < cameraCount; i++) {
			EdsCameraRef camera;
			CHECK_ERROR(EdsGetChildAtIndex(cameraList, 0, &camera)
				, "Get the camera device");

			if (camera != NULL) {
				devices.emplace_back(new Device(camera));
			}
		}

	fail:
		return devices;
	}
}