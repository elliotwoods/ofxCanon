#include "RemoteDevice.h"
#include "CustomRequest.h"

#define LOG_ERROR ofLogError("ofxCanon::RemoteDevice")

namespace ofxCanon {
	//----------
	RemoteDevice::RemoteDevice()
	{

	}

	//----------
	RemoteDevice::~RemoteDevice()
	{
		this->close();
	}

	//----------
	bool
		RemoteDevice::open(const string & hostname)
	{
		this->hostname = hostname;

		ofHttpRequest request;
		{
			request.method = ofHttpRequest::GET;
			request.url = "http://" + hostname + ":8080/ccapi/ver100/deviceinformation";
			request.timeoutSeconds = 5.0;
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

		this->thread.state = Thread::State::Running;
		this->thread.thread = std::thread([this]() {
			while (this->thread.state == Thread::State::Running) {
				// perform action queue
				std::function<void()> action;
				while (this->thread.actionQueue.tryReceive(action)) {
					try {
						action();
					}
					catch (const Exception& e) {
						LOG_ERROR << e.message;
					}
				}

				// poll
				this->poll();
			}
			});

		return true;
	}

	//----------
	void
		RemoteDevice::close()
	{
		if (this->thread.state != Thread::State::Closed) {
			this->thread.state = Thread::State::Joining;
			this->thread.thread.join();
			this->thread.state = Thread::State::Closed;
		}
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

		// Receive incoming actions
		{
			std::function<void()> action;
			while (this->thread.mainThreadActionQueue.tryReceive(action)) {
				action();
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
	bool 
		RemoteDevice::takePhoto(bool autoFocus)
	{
		this->thread.actionQueue.send([this, autoFocus]() {
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
				return false;
			}
			});


		this->waitingForPhoto = true;
		return true;
	}

	//----------
	string
		RemoteDevice::getBaseURL() const
	{
		return "http://" + this->hostname + ":8080/ccapi/ver100/";
	}

	//----------
	void
		RemoteDevice::setKeepFilesOnDevice(bool value)
	{
		this->keepFilesOnDevice = value;
	}

	//----------
	bool
		RemoteDevice::getKeepFilesOnDevice() const
	{
		return this->keepFilesOnDevice;
	}

	//----------
	void
		RemoteDevice::setDownloadJPEG(bool value)
	{
		this->downloadJPEG = value;
	}

	//----------
	bool
		RemoteDevice::getDownloadJPEG() const
	{
		return this->downloadJPEG;
	}

	//----------
	void
		RemoteDevice::setDownloadRAW(bool value)
	{
		this->downloadRAW = value;
	}

	//----------
	bool
		RemoteDevice::getDownloadRAW() const
	{
		return this->downloadRAW;
	}

	//----------
	bool
		RemoteDevice::getShootingMode(string& value) const
	{
		auto json = this->get("shooting/settings/shootingmode");
		if (!json.contains("value")) {
			return false;
		}
		else {
			value = json["value"].get<string>();
			return true;
		}
	}

	//----------
	bool
		RemoteDevice::setShootingMode(const string& value)
	{
		nlohmann::json json;
		json["value"] = value;
		auto result = this->put("shooting/settings/shootingmode", json);
		return result.contains("value");
	}

	//----------
	bool
		RemoteDevice::getISO(int& value) const
	{
		auto json = this->get("shooting/settings/shootingmode");
		if (!json.contains("value")) {
			return false;
		}
		else {
			value = this->convertISOFromDevice(json["value"].get<string>());
			return true;
		}
	}

	//----------
	bool
		RemoteDevice::setISO(int value)
	{
		nlohmann::json json;
		json["value"] = this->convertISOToDevice(value);
		auto result = this->put("shooting/settings/iso", json);
		return result.contains("value");
	}

	//----------
	bool
		RemoteDevice::getAperture(float& value) const
	{
		auto json = this->get("shooting/settings/av");
		if (!json.contains("value")) {
			return false;
		}
		else {
			value = this->convertApertureFromDevice(json["value"].get<string>());
			return true;
		}
	}

	//----------
	bool
		RemoteDevice::setAperture(float value)
	{
		nlohmann::json json;
		json["value"] = RemoteDevice::convertApertureToDevice(value);
		auto result = this->put("shooting/settings/av", json);
		return result.contains("value");
	}

	//----------
	bool
		RemoteDevice::getShutterSpeed(float& value) const
	{
		auto json = this->get("shooting/settings/tv");
		if (!json.contains("value")) {
			return false;
		}
		else {
			value = this->convertShutterSpeedFromDevice(json["value"].get<string>());
			return true;
		}
	}

	//----------
	bool
		RemoteDevice::setShutterSpeed(float value)
	{
		nlohmann::json json;
		json["value"] = RemoteDevice::convertShutterSpeedToDevice(value);
		auto result = this->put("shooting/settings/tv", json);
		return result.contains("value");
	}

	//----------
	int
		RemoteDevice::convertISOFromDevice(string text) const
	{
		if (text == "auto") {
			return 0;
		}
		else {
			return ofToInt(text);
		}
	}

	//----------
	string
		RemoteDevice::convertISOToDevice(int value) const
	{
		// get all options
		map<int, string> optionsByValue;
		{
			auto optionStrings = this->getOptions("shooting/settings/iso");
			for (const auto& optionString : optionStrings) {
				auto optionValue = this->convertISOFromDevice(optionString);
				optionsByValue.emplace(optionValue, optionString);
			}
		}

		if (optionsByValue.empty()) {
			LOG_ERROR << "Failed to get options";
			return "";
		}

		// find closest option
		auto find = optionsByValue.lower_bound(value);
		if (find != optionsByValue.end()) {
			return find->second;
		}
		else {
			return optionsByValue.rbegin()->second;
		}
	}

	//----------
	float
		RemoteDevice::convertApertureFromDevice(string text) const
	{
		// try using f
		{
			auto components = ofSplitString(text, "f", false);
			if (components.size() >= 2) {
				return ofToFloat(components[1]);
			}
		}

		LOG_ERROR << "Failed to convert aperture value " << text;
		return 0.0f;
	}

	//----------
	string
		RemoteDevice::convertApertureToDevice(float value) const
	{
		// get all options
		map<float, string> optionsByValue;
		{
			auto optionStrings = this->getOptions("shooting/settings/av");
			for (const auto& optionString : optionStrings) {
				auto optionValue = this->convertApertureFromDevice(optionString);
				optionsByValue.emplace(optionValue, optionString);
			}
		}

		if (optionsByValue.empty()) {
			LOG_ERROR << "Failed to get options";
			return "";
		}

		// find closest option
		auto find = optionsByValue.lower_bound(value);
		if (find != optionsByValue.end()) {
			return find->second;
		}
		else {
			return optionsByValue.rbegin()->second;
		}
	}


	//----------
	float
		RemoteDevice::convertShutterSpeedFromDevice(string text) const
	{
		// try using "
		{
			auto components = ofSplitString(text, "\"", false);
			if (components.size() >= 2) {
				float value = ofToFloat(components[0]);
				if (!components[1].empty()) {
					value += ofToFloat(components[1]) / 10.0f;
				}
				return value;
			}
		}

		// try using /
		{
			auto components = ofSplitString(text, "/", false);
			if (components.size() >= 2) {
				auto numerator = ofToFloat(components[0]);
				auto denominator = ofToFloat(components[1]);
				return numerator / denominator;
			}
		}

		LOG_ERROR << "Can't decode shutter speed value " << text;
		return 0.0f;
	}

	//----------
	string
		RemoteDevice::convertShutterSpeedToDevice(float value) const
	{
		// get all options
map<float, string> optionsByValue;
{
	auto optionStrings = this->getOptions("shooting/settings/tv");
	for (const auto& optionString : optionStrings) {
		auto optionValue = this->convertShutterSpeedFromDevice(optionString);
		optionsByValue.emplace(optionValue, optionString);
	}
}

if (optionsByValue.empty()) {
	LOG_ERROR << "Failed to get options";
	return "";
}

// find closest option
auto find = optionsByValue.lower_bound(value);
if (find != optionsByValue.end()) {
	return find->second;
}
else {
	return optionsByValue.rbegin()->second;
}
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

		if (json.contains("shootingmode")) {
			if (json["shootingmode"].contains("value")) {
				auto value = json["shootingmode"]["value"].get<string>();
				this->thread.mainThreadActionQueue.send([this, value]() {
					auto valueCopy = value;
					ofNotifyEvent(this->deviceEvents.onShootingModeChange, valueCopy, this);
					});
			}
		}

		if (json.contains("tv")) {
			if (json["tv"].contains("value")) {
				auto stringValue = json["tv"]["value"].get<string>();
				auto value = this->convertShutterSpeedFromDevice(stringValue);
				this->thread.mainThreadActionQueue.send([this, value]() {
					auto valueCopy = value;
					ofNotifyEvent(this->deviceEvents.onShutterSpeedChange, valueCopy, this);
					});
			}
		}

		if (json.contains("av")) {
			if (json["av"].contains("value")) {
				auto stringValue = json["av"]["value"].get<string>();
				auto value = this->convertApertureFromDevice(stringValue);
				this->thread.mainThreadActionQueue.send([this, value]() {
					auto valueCopy = value;
					ofNotifyEvent(this->deviceEvents.onApertureChange, valueCopy, this);
					});
			}
		}

		if (json.contains("iso")) {
			if (json["iso"].contains("value")) {
				auto stringValue = json["iso"]["value"].get<string>();
				int value;
				if (stringValue == "auto") {
					value = 0;
				}
				else {
					value = ofToInt(stringValue);
				}
				this->thread.mainThreadActionQueue.send([this, value]() {
					auto valueCopy = value;
					ofNotifyEvent(this->deviceEvents.onISOChange, valueCopy, this);
					});
			}
		}

		if (json.contains("addedcontents")) {
			for (const auto& filenameJson : json["addedcontents"]) {
				auto filepath = filenameJson.get<string>();
				auto extension = ofToLower(ofFilePath::getFileExt(filepath));

				if ((extension == "jpeg" || extension == "jpg") && this->downloadJPEG) {
					this->getFileFromCamera(filepath);
				}
				if ((extension == "cr2" || extension == "cr3") && this->downloadRAW) {
					this->getFileFromCamera(filepath);
				}
				if (!this->keepFilesOnDevice) {
					this->deleteFileOnCamera(filepath);
				}
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

	//----------
	void
		RemoteDevice::deleteFileOnCamera(const string& address)
	{
		auto url = "http://" + this->hostname + ":8080" + address;
		auto response = sendCustomRequest(url, nlohmann::json(), 1.0, "DELETE");

		if (response.status != 200) {
			auto responseJson = nlohmann::json::parse(response.data);
			LOG_ERROR << responseJson;
		}
	}

	//----------
	nlohmann::json
		RemoteDevice::get(const string& address) const
	{
		auto url = this->getBaseURL() + address;
		auto response = ofLoadURL(url);
		if (!response.status == 200) {
			LOG_ERROR << "Failed to get " << address << " : " << response.data;
			return nlohmann::json();
		}
		else {
			return nlohmann::json::parse(response.data);
		}
	}

	//----------
	nlohmann::json
		RemoteDevice::put(const string& address, const nlohmann::json& requestBody)
	{
		auto url = this->getBaseURL() + address;

		auto response = sendCustomRequest(this->getBaseURL() + address, requestBody, 1.0, "PUT");

		if (response.status != 200) {
			LOG_ERROR << "Couldn't put to : " << address << " : " << response.data;
			return nlohmann::json();
		}
		else {
			return nlohmann::json::parse(response.data);
		}
	}

	//----------
	vector<string>
		RemoteDevice::getOptions(const string& address) const
	{
		auto response = this->get(address);
		if (response.contains("ability")) {
			vector<string> options;
			for (const auto& ability : response["ability"]) {
				options.push_back(ability.get<string>());
			}
			return options;
		}

		LOG_ERROR << "Failed to get options";
		return vector<string>();
	}
}