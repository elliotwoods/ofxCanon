#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
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
        widgetsPanel->addButton("Take photo", [this]() {
            this->camera.takePhoto();
        })->setHeight(100.0f);
                                
        rootGroup->add(widgetsPanel);
        
        rootGroup->setCellSizes({-1, 300});
    }
    
    auto devices = ofxCanon::listDevices();
    if(devices.empty()) {
        ofSystemAlertDialog("No Canon cameras found");
    }
    this->camera.setup();
}

//--------------------------------------------------------------
void ofApp::update(){
    this->camera.update();
    if(this->camera.isPhotoNew()) {
        this->preview = this->camera.getPhotoPixels();
        this->preview.update();
    }
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
