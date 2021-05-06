#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxCvGui/Panels/Draws.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Canon {
			class LiveView : public Base {
			public:
				LiveView();
				string getTypeName() const override;

				void init();
				void update();

				ofxCvGui::PanelPtr getPanel() override;
			protected:
				void updateDrawObject();
				shared_ptr<ofxCvGui::Panels::Draws> panel;

				struct : ofParameterGroup {
					ofParameter<bool> enabled{ "Enabled", true };
					PARAM_DECLARE("LiveView", enabled);
				} parameters;
			};
		}
	}
}