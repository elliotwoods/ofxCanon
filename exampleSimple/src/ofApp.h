#pragma once

#include "ofMain.h"

#include "ofxCanon.h"

namespace ofxEdsdk {
	using Camera = ofxCanon::Simple;
}

class ofApp : public ofBaseApp {
public:
	void setup();
	void exit();
	void update();
	void draw();
	void keyPressed(int key);

	ofxEdsdk	::Camera camera;

	bool bIsRecordingMovie = false;
};
