#include "Control.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include "../../../ofxCanon/pairs/ofxMachineVision/Device/Canon.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Canon {
			//---------
			Control::Control() {
				RULR_NODE_INIT_LISTENER;
			}

			//---------
			string Control::getTypeName() const {
				return "Canon::Control";
			}

			//---------
			void Control::init() {
				this->addInput<Item::Camera>();
			}

			//---------
			void Control::update() {
				// check if we need to rebuild the interface
				bool needsRebuild = false;
				auto cameraNode = this->getInput<Item::Camera>();
				if (cameraNode != this->cached.cameraNode.lock()) {
					// Camera connection changed
					needsRebuild = true;
				}
				else {
					if (cameraNode) {
						// Check grabber changed
						if (cameraNode->getGrabber() != this->cached.cameraGrabber.lock()) {
							needsRebuild = true;
						}
						else {
							if (cameraNode->getGrabber()) {
								// Check device changed
								if (cameraNode->getGrabber()->getDevice() != this->cached.cameraDevice.lock()) {
									needsRebuild = true;
								}
								else {
									// Check lens has changed
									auto canonDevice = dynamic_pointer_cast<ofxMachineVision::Device::Canon>(cameraNode->getGrabber()->getDevice());
									if (canonDevice->getCamera()) {
										if (canonDevice->getCamera()->isLensNew()) {
											needsRebuild = true;
										}
									}
								}
							}
						}
					}
				}

				if (needsRebuild) {
					this->rebuildGui();
				}
			}

			//---------
			ofxCvGui::PanelPtr Control::getPanel() {
				this->panel = ofxCvGui::Panels::makeWidgets();
				return this->panel;
			}

			//----------
			void Control::rebuildGui() {
				this->panel->clear();
				auto cameraNode = this->getInput<Item::Camera>();
				if(cameraNode) {
					auto grabber = cameraNode->getGrabber();
					if (grabber) {
						auto device = dynamic_pointer_cast<ofxMachineVision::Device::Canon>(grabber->getDevice());
						if (device) {
							auto simpleCamera = device->getCamera();
							if (simpleCamera) {
								auto cameraThread = simpleCamera->getCameraThread();
								if (cameraThread) {
									auto device = cameraThread->device;

									string bodyName;
									string lensName;
									vector<int> isoOptions;
									vector<float> apertureOptions;
									vector<float> shutterSpeedOptions;

									cameraThread->device->performInCameraThreadBlocking([&]() {
										bodyName = device->getDeviceInfo().description;
										lensName = device->getLensInfo().lensName;
										isoOptions = device->getISOOptions();
										apertureOptions = device->getApertureOptions();
										shutterSpeedOptions = device->getShutterSpeedOptions();
									});

									{
										auto widget = this->panel->addLiveValue<string>("Body", [bodyName]() {
											return bodyName;
										});
									}

									{
										auto widget = this->panel->addLiveValue<string>("Lens", [lensName]() {
											return lensName;
										});
									}

									{
										auto widget = this->panel->addMultipleChoice("ISO");
										for (const auto & option : isoOptions) {
											widget->addOption(ofToString(option));
										}
									}

									{
										auto widget = this->panel->addMultipleChoice("Aperture");
										for (const auto & option : apertureOptions) {
											widget->addOption(ofToString(option));
										}
									}

									{
										auto widget = this->panel->addMultipleChoice("Shutter Speed");
										for (const auto & option : shutterSpeedOptions) {
											widget->addOption(ofToString(option));
										}
									}
								}
							}
						}
					}

				}
			}
		}
	}
}