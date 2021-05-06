#include "Device.h"

#include "Initializer.h"

#include "ofImage.h"

using namespace std;

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
				ERROR_GOTO_FAIL(EdsGetDeviceInfo(camera, &deviceInfo)
					, "Get device info");
				this->deviceInfo.description = string(deviceInfo.szDeviceDescription);
				this->deviceInfo.port = string(deviceInfo.szPortName);
			}

			ERROR_GOTO_FAIL(EdsSetPropertyEventHandler(camera, kEdsPropertyEvent_All, Handlers::handlePropertyEvent, (EdsVoid *)this)
				, "Set property event handler");
			ERROR_GOTO_FAIL(EdsSetObjectEventHandler(camera, kEdsObjectEvent_All, Handlers::handleObjectEvent, (EdsVoid *)this)
				, "Set object event handler");
			ERROR_GOTO_FAIL(EdsSetCameraStateEventHandler(camera, kEdsStateEvent_All, Handlers::handleStateEvent, (EdsVoid *)this)
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
	EdsCameraRef & Device::getRawCamera() {
		return this->camera;
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

		ERROR_GOTO_FAIL(EdsOpenSession(this->camera)
			, "Open session");

		//Set the save-to location to host
		{
			EdsUInt32 saveTo = kEdsSaveTo_Host;
			ERROR_GOTO_FAIL(EdsSetPropertyData(this->camera, kEdsPropID_SaveTo, 0, sizeof(saveTo), &saveTo)
				, "Set save to host");
		}

		//Temporarily lock the UI
		ERROR_GOTO_FAIL(EdsSendStatusCommand(this->camera, kEdsCameraStatusCommand_UILock, 0)
			, "Lock the camera UI");

		//Tell the camera that there's plenty of capacity available on the host (used to display shots remaining)
		{
			EdsCapacity capacity = { 0x7FFFFFFF, 0x1000, 1 };
			ERROR_GOTO_FAIL(EdsSetCapacity(this->camera, capacity)
				, "Notify the camera of available storage on host device");
		}

		//Unlock the camera UI
		ERROR_GOTO_FAIL(EdsSendStatusCommand(this->camera, kEdsCameraStatusCommand_UIUnLock, 0)
			, "Unlock the camera UI");

		this->isOpen = true;
		return true;

	fail:
		return false;
	}

	//----------
	void Device::close() {
		if (this->isOpen) {
			ERROR_GOTO_FAIL(EdsCloseSession(this->camera)
				, "Close session");
		}
	fail:
		return;
	}

	//----------
	void Device::update() {
		//perform outstanding actions in queue
		{
			Action action;
			if (this->actionQueue.tryReceive(action)) {
				action();
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
	void Device::takePhotoToMemoryCard() const {
		//Set the save-to location to camera
		{
			EdsUInt32 saveTo = kEdsSaveTo_Camera;
			ERROR_GOTO_FAIL(EdsSetPropertyData(this->camera, kEdsPropID_SaveTo, 0, sizeof(saveTo), &saveTo)
				, "Set save to camera");
		}

		EdsSendCommand(this->camera, kEdsCameraCommand_TakePicture, 0);

	fail:
		return;
	}

	//----------
	bool Device::getLiveView(ofPixels & pixels) const {
		try {
			if (!this->liveViewEnabled) {
				ofLogError("ofxCanon") << "Cannot call getLiveView. Please call setLiveViewEnabled(true).";
				throw((EdsError)0);
			}

			EdsStreamRef encodedStream = NULL;
			ERROR_THROW(EdsCreateMemoryStream(0, &encodedStream)
				, "Create memory stream for encoded live view image");

			EdsEvfImageRef evfImage = NULL;
			ERROR_THROW(EdsCreateEvfImageRef(encodedStream, &evfImage)
				, "Create EVF iamge reference");

			{
				auto result = EdsDownloadEvfImage(this->camera, evfImage);
				if (result == EDS_ERR_OBJECT_NOTREADY) {
					return false;
				}
				else {
					ERROR_THROW(result
						, "Download live view image");
				}
			}

			auto buffer = getBuffer(encodedStream);

			ofLoadImage(pixels, *buffer);

			ERROR_THROW(EdsRelease(encodedStream)
				, "Release live view stream");
			ERROR_THROW(EdsRelease(evfImage)
				, "Release live view image");

			return true;
		}
		catch (EdsError) {
			ofLogError("ofxCanon") << "Poll live view failed";
			return false;
		}
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
	bool Device::getLogDeviceCallbacks() const {
		return this->logDeviceCallbacks;
	}

	//----------
	void Device::setLogDeviceCallbacks(bool logDeviceCallbacks) {
		this->logDeviceCallbacks = logDeviceCallbacks;
	}

	//----------
	void Device::performInCameraThread(Action && action) {
		if (std::this_thread::get_id() == this->cameraThreadId) {
			// We're already in the camera thread
			action();
		}
		else {
			// We're calling from another thread, queue the action for later
			this->actionQueue.send(move(action));
		}
	}

	//----------
	void Device::performInCameraThreadBlocking(Action && action) {
		if (this_thread::get_id() == this->cameraThreadId) {
			// We're already in the camera thread
			action();
		}
		else {
			ofThreadChannel<int> returnChannel;

			//wrap the action so that it returns messages
			auto wrappedAction = [this, action, &returnChannel]() {
				action();
				returnChannel.send(0);
			};

			//send the action to be performed
			this->actionQueue.send(move(wrappedAction));

			// Wait for the return
			int returnedValue;
			returnChannel.receive(returnedValue);
		}
	}

	//----------
	void Device::setDownloadEnabled(bool downloadEnabled) {
		this->downloadEnabled = downloadEnabled;
	}

	//----------
	bool Device::getDownloadEnabled() const {
		return this->downloadEnabled;
	}

	//----------
	void Device::download(EdsDirectoryItemRef directoryItem) {
		if (!this->downloadEnabled) {
			// e.g. just save to memory card
			return;
		}

		PhotoCaptureResult photoCaptureAsyncResult;
		try {
			EdsDirectoryItemInfo directoryItemInfo;
			EdsStreamRef encodedStream = NULL;

			ERROR_THROW(EdsGetDirectoryItemInfo(directoryItem, &directoryItemInfo)
				, "Get directory item info");

			//download image
			{
				ERROR_THROW(EdsCreateMemoryStream(0, &encodedStream)
					, "Create memory stream for encoded image");
				ERROR_THROW(EdsDownload(directoryItem, directoryItemInfo.size, encodedStream)
					, "Download directory item");
				ERROR_THROW(EdsDownloadComplete(directoryItem)
					, "Download complete");
				
				// NOTE: Using EDSDK 13.12.1 Using EdsDeleteDirectoryItem on an in memory 
				// item first hang the camera for several seconds and then crashed the live view
				// following the reference document/pdf a DeleteDirectory item is not needed (at least)
				// it is not mentioned in the document (also not as change from previous API versions)
				//WARNING(EdsDeleteDirectoryItem(directoryItem)
				//	, "Delete directory item");
			}

			// NOTE : The Canon SDK does not provide decoding of RAW images for all its cameras, especially in 64bit
			// Therefore we use freeimage to perform this function (i.e. openFrameworks' built in functions),
			//	which do not offer functions such as getting the lens properties.

			auto buffer = getBuffer(encodedStream);

			//attempt to read metadata from the image reference
			EdsImageRef imageRef = NULL;
			shared_ptr<PhotoMetadata> metaData;
			{
				try {
					ERROR_THROW(EdsCreateImageRef(encodedStream, &imageRef)
						, "Create image reference from incoming stream for metadata purposes (Unsupported for CR2 on some camera models especially in x64)");
					vector<EdsRational> value(3);
					ERROR_THROW(EdsGetPropertyData(imageRef, kEdsPropID_FocalLength, 0, sizeof(EdsRational) * 3, value.data())
						, "Get focal length data from image");
					metaData = make_shared<PhotoMetadata>();
					metaData->focalLength.minimumFocalLength = rationalToFloat(value[1]);
					metaData->focalLength.currentFocalLength = rationalToFloat(value[0]);
					metaData->focalLength.maximumFocalLength = rationalToFloat(value[2]);

					ERROR_THROW(EdsRelease(imageRef)
						, "Release downloaded image");
				}
				catch (EdsError) {
					//this generally happens when the image is RAW and the SDK can't process it
				}
			}

			//release the objects
			ERROR_THROW(EdsRelease(encodedStream)
				, "Release encoded stream");

			photoCaptureAsyncResult.errorReturned = EDS_ERR_OK;
			photoCaptureAsyncResult.encodedBuffer = buffer;
			photoCaptureAsyncResult.metaData = metaData;

			//if we've got an async listener waiting for a photo
			if (this->capturePromise) {
				this->hasDownloadedFirstPhoto = true;
				this->captureStatus = CaptureStatus::CaptureSucceeded;

				this->capturePromise->set_value(photoCaptureAsyncResult);
				this->capturePromise.reset();
			}
			else {
				this->onUnrequestedPhotoReceived.notify(this, photoCaptureAsyncResult);
			}
		}
		catch (EdsError error) {
			this->captureStatus = CaptureStatus::CaptureFailed;

			//if we've got an async listener waiting for a photo
			if (this->capturePromise) {
				photoCaptureAsyncResult.errorReturned = error;

				this->capturePromise->set_value(photoCaptureAsyncResult);
				this->capturePromise.reset();
			}
		}
	}

	//----------
	void Device::lensChanged() {
		auto lensStatus = this->getProperty<EdsUInt32>(kEdsPropID_LensStatus);

		if (lensStatus == 0) {
			//no lens present
			if (this->lensInfo.lensAttached) {
				//lens removed
				this->lensInfo = LensInfo();
				this->onLensChange.notify(this, this->lensInfo);
			}
		}
		else {
			//lens present
			this->lensInfo.lensAttached = true;
			this->onLensChange.notify(this, this->lensInfo);
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
		case kEdsPropID_Evf_OutputDevice:
		{
			auto encoded = this->getProperty<EdsUInt32>(kEdsPropID_Evf_OutputDevice);
			if (encoded & kEdsEvfOutputDevice_PC) {
				this->liveViewEnabled = true;
			}
			else {
				this->liveViewEnabled = false;
			}
		}
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

		ofNotifyEvent(this->onParameterOptionsChange, propertyID, this);
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

		ERROR_GOTO_FAIL(EdsGetPropertyDesc(this->camera, kEdsPropID_ISOSpeed, &propertyDescription)
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

		ERROR_GOTO_FAIL(EdsGetPropertyDesc(this->camera, kEdsPropID_Av, &propertyDescription)
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

		ERROR_GOTO_FAIL(EdsGetPropertyDesc(this->camera, kEdsPropID_Tv, &propertyDescription)
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
	template<typename T>
	T findClosestOption(const T & value, const vector<T> & options) {
		T bestDistance = std::numeric_limits<T>::max();
		T bestOption = value;

		for (const auto & option : options) {
			auto distance = abs(option - value);
			if (distance < bestDistance) {
				bestDistance = distance;
				bestOption = option;
			}
		}

		return bestOption;
	}

	//----------
	void Device::setISO(int ISO, bool findClosest) {
		if (findClosest) {
			ISO = findClosestOption(ISO, this->getISOOptions());
		}
		this->parameters.ISO = ISO;
	}

	//----------
	float Device::getAperture() const {
		return this->parameters.aperture;
	}

	//----------
	void Device::setAperture(float aperture, bool findClosest) {
		if (findClosest) {
			aperture = findClosestOption(aperture, this->getApertureOptions());
		}
		this->parameters.aperture = aperture;
	}

	//----------
	float Device::getShutterSpeed() const {
		return this->parameters.shutterSpeed;
	}

	//----------
	void Device::setShutterSpeed(float shutterSpeed, bool findClosest) {
		if (findClosest) {
			shutterSpeed = findClosestOption(shutterSpeed, this->getShutterSpeedOptions());
		}
		this->parameters.shutterSpeed = shutterSpeed;
	}

	//----------
	bool Device::getLiveViewEnabled() const {
		return this->liveViewEnabled;
	}

	//----------
	void Device::setLiveViewEnabled(bool liveViewEnabled, bool enableCameraScreen) {
		{
			EdsUInt32 evfMode = liveViewEnabled ? 1 : 0;
			this->setProperty(kEdsPropID_Evf_Mode, evfMode);
		}

		this->liveViewEnabled = liveViewEnabled;

		if (liveViewEnabled) {
			EdsUInt32 outputDevice = liveViewEnabled
				? kEdsEvfOutputDevice_PC | (enableCameraScreen ? kEdsEvfOutputDevice_TFT : 0) // live view enabled
				: 0; // live view disabled
			this->setProperty(kEdsPropID_Evf_OutputDevice, outputDevice);
		}
	}

	//----------
	//Referenced from CameraControl.cpp in sample of v0304W of EDSDK
	vector<shared_ptr<Device>> listDevices() {
		vector<shared_ptr<Device>> devices;

		//ensure that EDSDK is initialized
		ofxCanon::Initializer::X();

		EdsCameraListRef cameraList;
		ERROR_GOTO_FAIL(EdsGetCameraList(&cameraList)
			, "Get camera list");

		EdsUInt32 cameraCount;
		ERROR_GOTO_FAIL(EdsGetChildCount(cameraList, &cameraCount)
			, "Get camera count");


		for (EdsUInt32 i = 0; i < cameraCount; i++) {
			EdsCameraRef camera;
			ERROR_GOTO_FAIL(EdsGetChildAtIndex(cameraList, 0, &camera)
				, "Get the camera device");

			if (camera != NULL) {
				devices.emplace_back(new Device(camera));
			}
		}

	fail:
		return devices;
	}

	//----------
	std::string Device::DeviceInfo::toString() const {
		stringstream status;

		status << "Camera : " << this->description << endl;
		status << "Port : " << this->port << endl;
		status << endl;
		status << "Manufacturer : " << this->owner.manufacturer << endl;
		status << "Owner : " << this->owner.owner << endl;
		status << "Artist : " << this->owner.artist << endl;
		status << "Copyright : " << this->owner.copyright << endl;
		status << endl;
		status << "Battery level : " << this->battery.batteryLevel << endl;
		status << "Battery quality : " << this->battery.batteryQuality << endl;
		status << "PSU present : " << this->battery.psuPresent << endl;
		status << endl;

		return status.str();
	}

	//----------
	std::string Device::LensInfo::toString() const {
		stringstream status;

		if (this->lensAttached) {
			status << "Lens : " << this->lensName << endl;
		}
		else {
			status << "No lens attached" << endl;
		}

		return status.str();
	}

}
