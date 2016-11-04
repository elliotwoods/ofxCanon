#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
	auto devices = ofxCanon::listDevices();

	for (auto device : devices) {
		device->open();
		this->device = device;
		break;
	}
	if (!this->device) {
		ofSystemAlertDialog("No camera detected");
		ofExit();
	}
}

//--------------------------------------------------------------
void ofApp::update(){
	if (device) {
		device->update();
	}


}

//--------------------------------------------------------------
void ofApp::draw(){
	if (this->preview.isAllocated()) {
		this->preview.draw(ofGetCurrentViewport());
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	ofShortPixels pixels16;
	if (this->device->takePhoto(pixels16)) {
		cout << "Photo successfully taken, now calculating histogram" << endl;
		this->preview.loadData(pixels16);
		this->preview.loadData(pixels16);
		this->bitHistogram.clear();
		this->bitHistogram.resize(16, 0);
		for (const auto & pixel : pixels16) {
			for (int i = 0; i < 16; i++) {
				if (pixel & 1 << i) {
					this->bitHistogram[i]++;
				}
			}
		}
		cout << "Histogram : " << endl;
		for (int i = 0; i < 16; i++) {
			cout << " Bit #" << i << ":\t" << this->bitHistogram[i] << endl;
		}
		cout << "If some bits are blank, it likely means that you're shooting in jpeg mode and probably want to change to RAW mode";
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
