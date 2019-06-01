#include "stdafx.h"
#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
	ofSetVerticalSync(true);

	this->gui.init();
	{
		auto rootGroup = this->gui.addStrip();

		auto imagePanel = ofxCvGui::Panels::makeImage(this->preview);
		rootGroup->add(imagePanel);

		auto widgetsPanel = ofxCvGui::Panels::makeWidgets();
		widgetsPanel->addFps();
		widgetsPanel->addMemoryUsage();
		widgetsPanel->addParameterGroup(this->parameters);

		widgetsPanel->addButton("New session", [this]() {
			this->newSession();
		});

		widgetsPanel->addButton("Reset counter", [this]() {
			this->parameters.photoIndex.set(0);
		});

		widgetsPanel->addButton("Compress movie", [this]() {
			this->compressMovie();
		});

		widgetsPanel->addEditableValue<float>("Shutter time", [this]() {
			return this->cameraDevice->getShutterSpeed();
		}, [this](const string & valueString) {
			if (!valueString.empty()) {
				auto value = ofToFloat(valueString);
				this->cameraDevice->setShutterSpeed(value);
			}
		});

		widgetsPanel->addEditableValue<float>("ISO", [this]() {
			return this->cameraDevice->getISO();
		}, [this](const string & valueString) {
			if (!valueString.empty()) {
				auto value = ofToInt(valueString);
				this->cameraDevice->setISO(value);
			}
		});

		widgetsPanel->addEditableValue<float>("Aperture", [this]() {
			return this->cameraDevice->getAperture();
		}, [this](const string & valueString) {
			if (!valueString.empty()) {
				auto value = ofToFloat(valueString);
				this->cameraDevice->setAperture(value);
			}
		});

		widgetsPanel->addButton("Take photo", [this]() {
			this->takePhoto();
		}, ' ')->setHeight(100.0f);

		widgetsPanel->addToggle("Run timelapse", [this]() {
			return this->run;
		}, [this](bool value) {
			this->run = value;

			if (value) {
				this->index = 0;
				if (this->parameters.captureOnStartRun) {
					this->takePhoto();
				}
				this->lastCaptureTrigger = chrono::high_resolution_clock::now();
			}
		});

		//remaining time widget
		{
			auto widget = widgetsPanel->addBlank();
			widget->onDraw += [this](ofxCvGui::DrawArguments & args) {
				if (!this->run) {
					ofxCvGui::Utils::drawText("Stopped", args.localBounds, false);
				}
				else {
					auto remaining = (this->lastCaptureTrigger + chrono::seconds(this->parameters.timeBetweenExposures)) - chrono::high_resolution_clock::now();
					auto millisecondsRemaining = chrono::duration_cast<chrono::milliseconds>(remaining).count();
					millisecondsRemaining -= millisecondsRemaining % 100;

					auto bounds = args.localBounds;
					bounds.width *= (float)millisecondsRemaining / (float)(this->parameters.timeBetweenExposures * 1000.0f);

					//draw background
					ofPushStyle();
					{
						ofSetColor(255);
						ofDrawRectangle(bounds);

						auto & font = ofxAssets::font(ofxCvGui::getDefaultTypeface(), 40);
						font.drawString(ofToString((float)millisecondsRemaining / 1000.0f) + "s", bounds.width, bounds.height);
					}
					ofPopStyle();
				}
			};
		}

		widgetsPanel->addIndicatorBool("Waiting for photo", [this]() {
			return this->asyncCapture.valid();
		});

		rootGroup->add(widgetsPanel);

		rootGroup->setCellSizes({ -1, 300 });
	}

	//list cameras
	auto deviceList = ofxCanon::listDevices();
	if (deviceList.empty()) {
		ofSystemAlertDialog("No Canon cameras found");
		ofExit();
	}
	else {
		//we take the first camera
		this->cameraDevice = *deviceList.begin();
	}

	this->cameraDevice->open();
	ofAddListener(this->cameraDevice->onUnrequestedPhotoReceived, this, & ofApp::callbackPhotoReceived);
	this->newSession();
}

//--------------------------------------------------------------
void ofApp::update() {
	this->cameraDevice->update();

	if (this->run) {
		auto durationDifference = chrono::high_resolution_clock::now() - this->lastCaptureTrigger;
		auto interval = chrono::seconds(this->parameters.timeBetweenExposures);
		if (durationDifference >= interval) {
			this->takePhoto();

			//we use this logic so that we retain accuracy over long periods of time regardless of frame rate jitter
			this->lastCaptureTrigger = this->lastCaptureTrigger + interval;
		}
	}

	if (!this->oscReceiver) {
		this->oscReceiver = make_shared<ofxOscReceiver>();
		this->oscReceiver->setup(this->parameters.oscPort);
	}
	else {
		ofxOscMessage message;
		while (this->oscReceiver->getNextMessage(&message)) {
			this->takePhoto();
		}

		if (this->parameters.oscPort != this->oscReceiver->getPort()) {
			this->oscReceiver.reset();
		}
	}
}

//--------------------------------------------------------------
void ofApp::draw() {

}

//----------
void ofApp::takePhoto() {
	auto camera = this->cameraDevice->getRawCamera();
	EdsError error = EDS_ERR_OK;

	this->cameraDevice->setDownloadEnabled(this->parameters.downloadPhoto.get());

	//press shutter
	if (this->parameters.downloadPhoto) {
		EdsUInt32 saveTo = kEdsSaveTo_Host;
		error = EdsSetPropertyData(camera, kEdsPropID_SaveTo, 0, sizeof(saveTo), &saveTo);
		if (error != EDS_ERR_OK) goto fail;
	}
	else {
		EdsUInt32 saveTo = kEdsSaveTo_Camera;
		error = EdsSetPropertyData(camera, kEdsPropID_SaveTo, 0, sizeof(saveTo), &saveTo);
		if (error != EDS_ERR_OK) goto fail;
	}

	// take photo
	{
		error = EdsSendCommand(camera, kEdsCameraCommand_TakePicture, 0);
		if (error != EDS_ERR_OK) goto fail;
	}

	return;

fail:
	ofLogError() << ofxCanon::errorToString(error);
}

//----------
void ofApp::newSession() {
	// new session name
	{
		time_t rawtime;
		struct tm * timeinfo;
		char buffer[80];

		time(&rawtime);
		timeinfo = localtime(&rawtime);

		strftime(buffer, 80, "%Y%m%d_%H%M", timeinfo);
		this->parameters.sessionName.set(buffer);
	}

	this->parameters.photoIndex.set(0);
}

//--------------------------------------------------------------
void ofApp::compressMovie() {
	auto sessionFolder = ofToDataPath(this->getSessionFolder(), true);
	auto command = "ffmpeg -y -r 24 -start_number 0 -i \"" + sessionFolder + "/%6d.JPG\" -vf scale=1920:-2 -pix_fmt yuv420p -vcodec libx264 -crf 10 \"" + sessionFolder + ".mp4\"";
	ofSystem(command);
	cout << command << endl;
}

//--------------------------------------------------------------
string ofApp::getSessionFolder() const {
	return "Sessions/" + this->parameters.sessionName.get();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}

//--------------------------------------------------------------
void ofApp::callbackPhotoReceived(ofxCanon::Device::PhotoCaptureResult & photoResult) {
	if (photoResult) {
		if (this->parameters.downloadPhoto) {
			ofLoadImage(this->preview.getPixels(), *photoResult.encodedBuffer);

			auto directoryName = this->getSessionFolder();

			//make directory if it doesn't exist
			{
				ofDirectory::createDirectory(directoryName, true, true);
			}

			//save the file
			{
				string filenameTrunk = ofToString(this->parameters.photoIndex.get());
				while (filenameTrunk.size() < 6) {
					filenameTrunk = "0" + filenameTrunk;
				}

				ofFile file;
				file.open(directoryName + "/" + filenameTrunk + ".jpg", ofFile::WriteOnly, true);
				file << *photoResult.encodedBuffer;
				file.close();
			}

			//increment photo index
			{
				this->parameters.photoIndex.set(this->parameters.photoIndex.get() + 1);
			}
			this->preview.update();
		}
	}
	else {
		ofLogWarning() << "Photo capture failed : " << ofxCanon::errorToString(photoResult.errorReturned);
	}
}
