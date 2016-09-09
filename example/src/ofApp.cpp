#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
	auto devices = ofxCanon::listDevices();

	for (auto device : devices) {
		device->open();
		device->takePicture();
		this->device = device;
		break;
	}
}

//--------------------------------------------------------------
void ofApp::update(){
	if (!device) {
		return;
	}

	this->device->idleFunction();

	auto & pixels = this->device->getPhoto();
	if (pixels.isAllocated()) {
		this->preview.loadData(pixels);
	}	
}

//--------------------------------------------------------------
void ofApp::draw(){
	if (this->preview.isAllocated()) {
		this->preview.draw(ofGetCurrentViewport());
	}

	if(this->device) {
		stringstream status;

		{
			auto deviceInfo = this->device->getDeviceInfo();
			status << "Camera : " << deviceInfo.description << endl;
			status << "Port : " << deviceInfo.port << endl;
			status << endl;
			status << "Manufacturer : " << deviceInfo.owner.manufacturer << endl;
			status << "Owner : " << deviceInfo.owner.owner << endl;
			status << "Artist : " << deviceInfo.owner.artist << endl;
			status << "Copyright : " << deviceInfo.owner.copyright << endl;
			status << endl;
			status << "Battery level : " << deviceInfo.battery.batteryLevel << endl;
			status << "Battery quality : " << deviceInfo.battery.batteryQuality << endl;
			status << "PSU present : " << deviceInfo.battery.psuPresent << endl;
			status << endl;
		}

		{
			auto lensInfo = this->device->getLensInfo();
			if (lensInfo.lensAttached) {
				status << "Lens : " << lensInfo.lensName << endl;
				status << "Current focal length : " << lensInfo.currentFocalLength << endl;
				status << "Minimum focal length : " << lensInfo.minimumFocalLength << endl;
				status << "Maximum focal length : " << lensInfo.maximumFocalLength << endl;
			}
			else {
				status << "No lens attached" << endl;
			}
			status << endl;
		}

		{
			status << "[I]SO : \t\t" << device->getISO() << endl;
			status << "[A]perture : \t\t" << device->getAperture() << endl;
			status << "[S]hutter Speed : \t" << device->getShutterSpeed() << endl;
		}
		ofDrawBitmapStringHighlight(status.str(), 30, 30, ofColor(200, 100, 100), ofColor::white);
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	if (key == 'r') {
		this->device.reset();
		this->setup();
	}

	if (!this->device) {
		return;
	}

	switch (key) {
	case ' ':
		this->device->takePicture();
		break;
	case 'i':
	case 'I':
	{
		auto result = ofSystemTextBoxDialog("Set ISO");
		if (!result.empty()) {
			auto ISO = ofToInt(result);
			device->setISO(ISO);
		}
		break;
	}
	case 'a':
	case 'A':
	{
		auto result = ofSystemTextBoxDialog("Set Aperture");
		if (!result.empty()) {
			auto aperture = ofToFloat(result);
			device->setAperture(aperture);
		}
		break;
	}
	case 's':
	case 'S':
	{
		auto result = ofSystemTextBoxDialog("Set Shutter Speed");
		if (!result.empty()) {
			auto shutterSpeed = ofToFloat(result);
			device->setShutterSpeed(shutterSpeed);
		}
		break;

	}
	default:
		break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
	
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
