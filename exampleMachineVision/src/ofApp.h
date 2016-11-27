#pragma once

#include "ofxMachineVision.h"
#include "ofxCvGui.h"
#include "ofMain.h"
#include "../pairs/ofxMachineVision/Device/Canon.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
	
		ofxMachineVision::SimpleGrabber<ofxMachineVision::Device::Canon> grabber;
		ofxCvGui::Builder gui;
};
