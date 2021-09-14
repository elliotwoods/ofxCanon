#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
	this->gui.init();

	this->gui.add(this->device.getImage());

	{
		auto widgets = this->gui.addWidgets();

		widgets->addFps();

		widgets->addEditableValue(this->hostname);
		widgets->addButton("Setup", [this]() {
			this->device.setup(this->hostname.get());
			}, 's');
		widgets->addLiveValue<string>("Product name", [this]() {
			return this->device.getDeviceInfo().productName;
			});
		widgets->addLiveValue<string>("Serial", [this]() {
			return this->device.getDeviceInfo().serialNumber;
			});
		widgets->addLiveValue<string>("Firmware version", [this]() {
			return this->device.getDeviceInfo().firmwareVersion;
			});
		widgets->addToggle(this->autofocus);
		widgets->addButton("Take photo", [this]() {
			this->device.takePhoto(this->autofocus.get());
			}, ' ');
	}
}


//--------------------------------------------------------------
void ofApp::update(){
	this->device.update();
}

//--------------------------------------------------------------
void ofApp::draw(){

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

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
