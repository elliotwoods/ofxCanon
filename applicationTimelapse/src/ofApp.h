#pragma once

#include "ofMain.h"
#include "ofxCvGui.h"
#include "ofxCanon.h"
#include "ofxOsc.h"

class ofApp : public ofBaseApp {

public:
	void setup();
	void update();
	void draw();

	void takePhoto();
	void newSession();
	void compressMovie();
	string getSessionFolder() const;

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

	void callbackPhotoReceived(ofxCanon::Device::PhotoCaptureResult &);

	struct Parameters : ofParameterGroup {
		ofParameter<string> sessionName{ "Session name", "" };
		ofParameter<int> photoIndex{ "Photo index", 0 };

		ofParameter<float> timeBetweenExposures{ "Time between exposures [s]", 10, 0, 60 };
		ofParameter<bool> captureOnStartRun{ "Capture on start run", true };
		ofParameter<bool> saveEnabed{ "Save enabled", true };
		ofParameter<bool> downloadPhoto{ "Download photo", true };
		ofParameter<int> oscPort{"OSC Port", 5000};
		Parameters() {
			this->add(sessionName);
			this->add(photoIndex);

			this->add(timeBetweenExposures);
			this->add(captureOnStartRun);
			this->add(saveEnabed);
			this->add(downloadPhoto);
			this->add(oscPort);
		}
	} parameters;

	ofxCvGui::Builder gui;
	ofImage preview;

	bool run = false;
	size_t index = 0;

	shared_ptr<ofxCanon::Device> cameraDevice;
	future<ofxCanon::Device::PhotoCaptureResult> asyncCapture;
	chrono::high_resolution_clock::time_point lastCaptureTrigger;

	shared_ptr<ofxOscReceiver> oscReceiver;
};
