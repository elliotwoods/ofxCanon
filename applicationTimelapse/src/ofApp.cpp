#include "stdafx.h"
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
            this->takePhoto();
        })->setHeight(100.0f);

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
					bounds.width *= (float) millisecondsRemaining / (float)(this->parameters.timeBetweenExposures * 1000.0f);

					//draw background
					ofPushStyle();
					{
						ofSetColor(255);
						ofDrawRectangle(bounds);

						auto & font = ofxAssets::font(ofxCvGui::getDefaultTypeface(), 40);
						font.drawString(ofToString((float) millisecondsRemaining / 1000.0f) + "s", bounds.width, bounds.height);
					}
					ofPopStyle();
				}
			};
		}

		widgetsPanel->addIndicatorBool("Waiting for photo", [this]() {
			return this->asyncCapture.valid();
		});
                                
        rootGroup->add(widgetsPanel);
        
        rootGroup->setCellSizes({-1, 300});
    }

	//list cameras
    auto deviceList = ofxCanon::listDevices();
    if(deviceList.empty()) {
        ofSystemAlertDialog("No Canon cameras found");
		ofExit();
	}
	else {
		//we take the first camera
		this->cameraDevice = * deviceList.begin();
	}

	this->cameraDevice->open();
}

//--------------------------------------------------------------
void ofApp::update(){
    this->cameraDevice->update();

	if (this->asyncCapture.valid()) {
		auto status = this->asyncCapture.wait_for(std::chrono::milliseconds(10));
		if (status == future_status::ready) {
			auto result = this->asyncCapture.get();
			if (result) {
				ofLoadImage(this->preview.getPixels(), *result.encodedBuffer);
				this->preview.update();
			}
			else {
				ofSystemAlertDialog("Photo capture failed : " + ofxCanon::errorToString(result.errorReturned));
			}
		}
	}

	if (this->run) {
		auto durationDifference = chrono::high_resolution_clock::now() - this->lastCaptureTrigger;
		auto interval = chrono::seconds(this->parameters.timeBetweenExposures);
		if (durationDifference >= interval) {
			this->takePhoto();

			//we use this logic so that we retain accuracy over long periods of time regardless of frame rate jitter
			this->lastCaptureTrigger = this->lastCaptureTrigger + interval;
		}
	}
}

//--------------------------------------------------------------
void ofApp::draw(){

}

//----------
void ofApp::takePhoto() {
	this->asyncCapture = this->cameraDevice->takePhotoAsync();
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
