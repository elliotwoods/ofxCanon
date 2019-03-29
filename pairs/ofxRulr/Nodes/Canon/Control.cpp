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
				RULR_NODE_UPDATE_LISTENER;

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

				//update the values
				{
					auto rawDevice = this->cached.rawDevice.lock();
					if (rawDevice) {
						int ISO;
						float aperture;
						float shutterSpeed;

						rawDevice->performInCameraThreadBlocking([&]() {
							ISO = rawDevice->getISO();
							aperture = rawDevice->getAperture();
							shutterSpeed = rawDevice->getShutterSpeed();
						});

						if (this->isoSelector) {
							this->isoSelector->setSelection(ofToString(ISO));
						}
						if (this->apertureSelector) {
							this->apertureSelector->setSelection(ofToString(aperture));
						}
						this->currentShutterSpeed = shutterSpeed;
					}
				}
			}

			//---------
			ofxCvGui::PanelPtr Control::getPanel() {
				this->panel = ofxCvGui::Panels::makeWidgets();
				return this->panel;
			}

			//----------
			void Control::rebuildGui() {
				//clear the panel to start with
				this->panel->clear();
				this->isoSelector.reset();
				this->apertureSelector.reset();
				this->cached.rawDevice.reset();

				auto cameraNode = this->getInput<Item::Camera>();
				this->cached.cameraNode = cameraNode;

				if(cameraNode) {
					auto grabber = cameraNode->getGrabber();
					this->cached.cameraGrabber = grabber;

					if (grabber) {
						auto device = dynamic_pointer_cast<ofxMachineVision::Device::Canon>(grabber->getDevice());
						this->cached.cameraDevice = device;

						if (device) {
							auto simpleCamera = device->getCamera();
							if (simpleCamera) {
								auto cameraThread = simpleCamera->getCameraThread();
								if (cameraThread) {
									auto device = cameraThread->device;
									this->cached.rawDevice = device;

									string bodyName;
									string lensName;
									vector<int> isoOptions;
									vector<float> apertureOptions;
									vector<float> shutterSpeedOptions;

									device->performInCameraThreadBlocking([&]() {
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
										widget->onValueChange += [device, widget](int) {
											auto isoValue = ofToInt(widget->getSelection());
											device->setISO(isoValue);
										};
										this->isoSelector = widget;
									}

									{
										auto widget = this->panel->addMultipleChoice("Aperture");
										for (const auto & option : apertureOptions) {
											widget->addOption(ofToString(option));
										}
										widget->onValueChange += [device, widget](int) {
											auto apertureValue = ofToFloat(widget->getSelection());
											device->setAperture(apertureValue);
										};
										this->apertureSelector = widget;
									}

									{
										auto widget = this->panel->addEditableValue<float>("Shutter Speed", [this]() {
											return this->currentShutterSpeed;
										}, [this, device](string valueString) {
											auto newShutterSpeed = ofToFloat(valueString);
											device->performInCameraThreadBlocking([&]() {
												device->setShutterSpeed(newShutterSpeed, true);
											});
										});
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