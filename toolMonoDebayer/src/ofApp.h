#pragma once

#include "ofMain.h"
#include "ofxCanon.h"
#include "ofxCvGui.h"

class ofApp : public ofBaseApp {

public:
	void setup() override;
	void update() override;
	void draw() override;

	void takePhoto();
	void process();

	void dragEvent(ofDragInfo dragInfo) override;
	void processFile(const std::string&);
	ofShortImage raw;
	ofShortImage result;
	ofImage standardProcess;

	shared_ptr<ofxCvGui::Panels::Image> resultPanel;
	shared_ptr<ofxCvGui::Panels::Image> standardProcessPanel;

	shared_ptr<ofxCanon::Device> device;

	ofxCvGui::Builder gui;

	struct : ofParameterGroup {
		struct : ofParameterGroup {
			ofParameter<bool> enabled{ "Enabled", true };
			ofParameter<float> percentile{ "Percentile %", 0.99, 0, 1 };
			ofParameter<float> ignoreTop{ "Ignore top %", 0.01, 0, 1 };
			ofParameter<float> normalizeTo{ "Normalize to", 0.5f, 0, 1};
			PARAM_DECLARE("Normalize", enabled, percentile, ignoreTop, normalizeTo);
		} normalize;

		struct : ofParameterGroup {
			ofParameter<bool> enabled{ "Enabled", true };
			ofParameter<int> dilationIterations{ "Dilation iterations", 2 };
			PARAM_DECLARE("Mono debayer", enabled, dilationIterations);
		} monoDebayer;
		
		struct : ofParameterGroup {
			ofParameter<bool> onProcess{ "On process", true };
			ofParameter<string> fileType{ "File type", "tiff" };
			ofParameter<bool> as16Bit{ "16 bit", true };
			PARAM_DECLARE("Save", onProcess, fileType, as16Bit);
		} save;
		
		
		PARAM_DECLARE("Parameters", normalize, monoDebayer, save);
	} parameters;
};
