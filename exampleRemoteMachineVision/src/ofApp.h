#pragma once

#include "ofMain.h"
#include "ofxCanon.h"
#include "ofxCvGui.h"
#include "ofxMachineVision.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();

		ofxCvGui::Builder gui;

		shared_ptr<ofxMachineVision::Device::Base::InitialisationSettings> initialisationSettings;

		ofxMachineVision::GrabberPtr grabber;
};
