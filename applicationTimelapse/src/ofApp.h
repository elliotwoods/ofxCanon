#pragma once

#include "ofMain.h"
#include "ofxCvGui.h"
#include "ofxCanon.h"

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
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
    
    struct Parameters : ofParameterGroup {
        ofParameter<int> timeBetweenExposures {"Time between exposures [s]", 5};
        ofParameter<float> timeUntilNextExposure{"Time until next exposure [s]", 5};
        ofParameter<bool> run {"Run", false};
        Parameters() {
            this->add(timeBetweenExposures);
            this->add(timeUntilNextExposure);
            this->add(run);
        }
    } parameters;
    
    ofxCvGui::Builder gui;
    ofImage preview;
    
    ofxCanon::Simple camera;
};
