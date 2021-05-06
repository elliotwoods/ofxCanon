#include "LiveView.h"

#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxCanon.h"
#include "../../../ofxCanon/pairs/ofxMachineVision/Device/Canon.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Canon {
			//----------
			LiveView::LiveView() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string LiveView::getTypeName() const {
				return "Canon::LiveView";
			}

			//----------
			void LiveView::init() {
				RULR_NODE_UPDATE_LISTENER;

				this->manageParameters(this->parameters);

				this->addInput<Item::Camera>();

				this->panel = make_shared<ofxCvGui::Panels::Draws>();
				this->panel->setCaption("LiveView");
				this->panel->onDraw += [this](ofxCvGui::DrawArguments & args) {
					if (!this->panel->getDrawObject()) {
						//we have no target to draw, let's give a notice to the user
						ofxCvGui::Utils::drawText("Connect Item::Camera with device type Canon and set enabled to true.", args.localBounds);
					}
				};
			}
			
			//----------
			void LiveView::update() {
				this->updateDrawObject();
			}


			//----------
			ofxCvGui::PanelPtr LiveView::getPanel() {
				return this->panel;
			}

			//----------
			void LiveView::updateDrawObject() {
				//try and set the view target to the canon device
				auto camera = this->getInput<Item::Camera>();
				if (camera) {
					auto grabber = camera->getGrabber();
					auto device = dynamic_pointer_cast<ofxMachineVision::Device::Canon>(grabber->getDevice());
					if (device) {
						auto canonCamera = device->getCamera();
						if (canonCamera) {
							if (this->parameters.enabled) {
								canonCamera->setLiveView(true);
								if (canonCamera->isLiveDataReady()) {
									this->panel->setDrawObject(canonCamera->getLiveTexture());

									//we succeeded so lets exit
									return;
								}
							}
							else {
								canonCamera->setLiveView(false);
							}
							
						}
					}
				}

				//if we got here, then we're not valid and let's clear the object
				this->panel->clearDrawObject();
			}
		}
	}
}