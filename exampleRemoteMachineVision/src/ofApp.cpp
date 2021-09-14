#include "ofApp.h"
#include "../../../addons/ofxCanon/pairs/ofxMachineVision/Device/CanonRemote.h"
using namespace ofxCvGui;

//--------------------------------------------------------------
void ofApp::setup() {
	ofxMachineVision::Device::FactoryRegister::X().add<ofxMachineVision::Device::CanonRemote>();

	this->gui.init();
	{
		auto widgets = this->gui.addWidgets();

		widgets->addTitle("Open a device");

		widgets->addEditableValue(this->hostname);
		widgets->addButton("Open", [this, widgets]() {
			auto factory = ofxMachineVision::Device::FactoryRegister::X().get<ofxMachineVision::Device::CanonRemote>();
			auto device = factory->makeUntyped();
			auto grabber = make_shared<ofxMachineVision::Grabber::Simple>();
			grabber->setDevice(device);
			auto settingsUntyped = device->getDefaultSettings();
			auto settings = dynamic_pointer_cast<ofxMachineVision::Device::CanonRemote::InitialisationSettings>(settingsUntyped);
			settings->hostname = this->hostname;
			if (grabber->open(settings)) {
				auto panel = ofxCvGui::Panels::makeBaseDraws(*grabber, factory->getModuleTypeName());
				panel->onUpdate += [grabber](ofxCvGui::UpdateArguments&) {
					grabber->update();
				};

				widgets->addButton("Take photo (" + settings->hostname.get() + ")", [grabber]() {
					grabber->singleShot();
					});

				this->gui.add(panel);
			}
		});
	}

}

//--------------------------------------------------------------
void ofApp::update() {

}
