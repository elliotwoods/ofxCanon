#pragma once

#include "ofxRulr.h"
#include "ofxCanon.h"
#include "ofxMachineVision.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Canon {
			class Control : public Nodes::Base {
			public:
				Control();
				string getTypeName() const override;

				void init();
				void update();

				ofxCvGui::PanelPtr getPanel() override;
			protected:
				void rebuildGui();

				shared_ptr<ofxCvGui::Panels::Widgets> panel;

				struct {
					weak_ptr<Nodes::Base> cameraNode;
					weak_ptr<ofxMachineVision::Grabber::Base> cameraGrabber;
					weak_ptr<ofxMachineVision::Device::Base> cameraDevice;
				} cached;
			};
		}
	}
}