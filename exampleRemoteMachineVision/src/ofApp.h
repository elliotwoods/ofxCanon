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

		ofParameter<string> hostname{ "Hostname", "10.0.0.180" };
		ofxMachineVision::GrabberPtr grabber;
};
