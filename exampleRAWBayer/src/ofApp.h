#pragma once

#include "ofMain.h"
#include "ofxCanon.h"
#include "ofxCvGui.h"

class ofApp : public ofBaseApp {

public:
	void setup();
	void update();
	void draw();
	void takePhoto();
	void calibrateWhiteBalance();
	void calculateResultFromWhiteBalance();
	void calculateResultFromBlurKernel();
	void calculateResultAuto();

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

	ofShortImage image;
	ofShortImage result;

	shared_ptr<ofxCanon::Device> device;

	ofxCvGui::Builder gui;

	struct : ofParameterGroup {
		ofParameter<bool> normalize{ "Normalize", true };
		ofParameter<ofRectangle> selection{ "Selection", ofRectangle() };

		struct : ofParameterGroup {
			ofParameter<float> standardDeviations{ "Standard deviations", 2.0f, 0.01f, 10.0f };
			ofParameter<float> redFactor{ "Red factor", 1.0f, 0.0f, 10.0f };
			ofParameter<float> blueFactor{ "Blue factor", 1.0f, 0.0f, 10.0f };
			ofParameter<float> blurRadius{ "Blur radius", 100.0f, 0.0f, 10000.0f };
			PARAM_DECLARE("Calibration", standardDeviations, redFactor, blueFactor, blurRadius);
		} calibration;
		PARAM_DECLARE("Parameters", normalize, selection, calibration);
	} parameters;

	glm::vec2 mousePositionInImage;
};
