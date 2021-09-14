#include "RemoteDevice.h"

#define LOG_ERROR ofLogError("ofxCanon::RemoteDevice")

namespace ofxCanon {
	//----------
	RemoteDevice::RemoteDevice()
	{

	}

	//----------
	bool
		RemoteDevice::setup(const string & hostname)
	{
		this->hostname = hostname;

		ofHttpRequest request;
		{
			request.method = ofHttpRequest::GET;
			request.url = "http://" + hostname + ":8080/ccapi/ver100/deviceinformation";
		}
		auto response = this->urlLoader.handleRequest(request);
		if (response.status != 200) {
			LOG_ERROR << "Failed to connect to CCAPI on " << hostname;
			return false;
		}
		
		auto json = nlohmann::json::parse(response.data);

		if (json.contains("manufacturer")) {
			this->deviceInfo.manufacturer = json["manufacturer"].get<string>();
		}

		if (json.contains("productname")) {
			this->deviceInfo.productName = json["productname"].get<string>();
		}

		if (json.contains("guid")) {
			this->deviceInfo.guid = json["guid"].get<string>();
		}

		if (json.contains("serialnumber")) {
			this->deviceInfo.serialNumber = json["serialnumber"].get<string>();
		}

		if (json.contains("macaddress")) {
			this->deviceInfo.macAddress = json["macaddress"].get<string>();
		}

		if (json.contains("firmwareversion")) {
			this->deviceInfo.firmwareVersion = json["firmwareversion"].get<string>();
		}

		return true;
	}

	//----------
	RemoteDevice::DeviceInfo
		RemoteDevice::getDeviceInfo() const
	{
		return this->deviceInfo;
	}

	//----------
	void
		RemoteDevice::update()
	{
		if (this->hostname.empty()) {
			return;
		}

		if (this->waitingForPhoto) {
			this->poll();
		}

		this->frameIsNew = false;

		// Receive incoming images
		{
			ofBuffer buffer;

			while (this->incomingImages.tryReceive(buffer)) {
				this->frameIsNew = true;
			}

			if (this->frameIsNew) {
				this->image.load(buffer);
				this->waitingForPhoto = false;
			}
		}
	}

	//----------
	bool
		RemoteDevice::isFrameNew() const
	{
		return this->frameIsNew;
	}

	//----------
	ofImage &
		RemoteDevice::getImage()
	{
		return this->image;
	}

	//----------
	void 
		RemoteDevice::takePhoto(bool autoFocus)
	{
		ofHttpRequest request;
		{
			request.method = ofHttpRequest::POST;
			request.url = this->getBaseURL() + "shooting/control/shutterbutton";

			nlohmann::json requestData;
			requestData["af"] = autoFocus;
			request.body = requestData.dump();
		}

		auto response = this->urlLoader.handleRequest(request);

		if (response.status != 200) {
			LOG_ERROR << "Couldn't take photo : " << response.data;
		}

		this->waitingForPhoto = true;
	}

	//----------
	string
		RemoteDevice::getBaseURL() const
	{
		return "http://" + this->hostname + ":8080/ccapi/ver100/";
	}

	//----------
	void
		RemoteDevice::poll()
	{
		ofHttpRequest request;
		{
			request.method = ofHttpRequest::GET;
			request.url = this->getBaseURL() + "event/polling";
		}

		auto response = this->urlLoader.handleRequest(request);

		if (response.status != 200) {
			LOG_ERROR << "Couldn't poll.";
			return;
		}

		auto json = nlohmann::json::parse(response.data);
		if (json.contains("addedcontents")) {
			for (const auto& filenameJson : json["addedcontents"]) {
				this->getFileFromCamera(filenameJson.get<string>());
			}
		}
	}

	//----------
	void
		RemoteDevice::getFileFromCamera(const string& address)
	{
		auto response = ofLoadURL("http://" + this->hostname + ":8080" + address);
		if (response.status == 200) {
			this->incomingImages.send(response.data);
		}
	}
}