#include "ofApp.h"
#include "../../../addons/ofxCanon/pairs/ofxMachineVision/Device/CanonRemote.h"
using namespace ofxCvGui;

//----------
template<typename DataType, typename WidgetType>
bool addParameterWidget(shared_ptr<ofxCvGui::Panels::Widgets> inspector
	, shared_ptr<ofxMachineVision::AbstractParameter> parameter
	, shared_ptr<ofxMachineVision::Grabber::Simple> grabber) {
	auto typedOfParameter = parameter->getParameterTyped<DataType>();
	if (typedOfParameter) {
		auto widget = make_shared<WidgetType>(*typedOfParameter);

		auto units = parameter->getUnits();
		if (!units.empty()) {
			widget->setCaption(widget->getCaption() + " [" + units + "]");
		}

		//we don't use weak pointers here, just presume that we should never arrive here if grabber closed / param disappeared
		widget->onValueChange += [grabber, parameter](const DataType&) {
			grabber->syncToDevice(*parameter);
		};

		inspector->add(widget);
		return true;
	}
	else {
		return false;
	}
}

//--------------------------------------------------------------
void ofApp::setup() {
	ofxMachineVision::Device::FactoryRegister::X().add<ofxMachineVision::Device::CanonRemote>();

	this->initialisationSettings = ofxMachineVision::Device::CanonRemote().getDefaultSettings();

	this->gui.init();
	{
		auto widgets = this->gui.addWidgets();

		widgets->addFps();
		widgets->addMemoryUsage();
		widgets->addTitle("Open a device");

		widgets->addTitle("Initialisation settings", ofxCvGui::Widgets::Title::Level::H2);
		{
			widgets->addParameterGroup(*this->initialisationSettings);
		}

		widgets->addButton("Open", [this, widgets]() {
			auto factory = ofxMachineVision::Device::FactoryRegister::X().get<ofxMachineVision::Device::CanonRemote>();
			auto device = factory->makeUntyped();
			auto grabber = make_shared<ofxMachineVision::Grabber::Simple>();
			grabber->setDevice(device);

			if (grabber->open(this->initialisationSettings)) {
				auto panel = ofxCvGui::Panels::makeBaseDraws(*grabber, factory->getModuleTypeName());
				panel->onUpdate += [grabber](ofxCvGui::UpdateArguments&) {
					grabber->update();
				};

				widgets->addButton("Take photo", [grabber]() {
					grabber->singleShot();
					}, ' ');

				//Add device parameters
				{
					const auto& deviceParameters = grabber->getDeviceParameters();
					for (const auto& parameter : deviceParameters) {
						//try and add the parameter to the inspector
						if (addParameterWidget<float, Widgets::Slider>(widgets, parameter, grabber)) {}
						else if (addParameterWidget<int, Widgets::EditableValue<int>>(widgets, parameter, grabber)) {}
						else if (addParameterWidget<bool, Widgets::Toggle>(widgets, parameter, grabber)) {}
						else if (addParameterWidget<string, Widgets::EditableValue<string>>(widgets, parameter, grabber)) {}
					}
				}

				this->gui.add(panel);
			}
		});
	}

}

//--------------------------------------------------------------
void ofApp::update() {

}
