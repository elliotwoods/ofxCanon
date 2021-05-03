#include "ofApp.h"

//----------
template<typename DataType, typename WidgetType>
bool addParameterWidget(shared_ptr<ofxCvGui::Panels::Widgets> inspector
	, shared_ptr<ofxMachineVision::AbstractParameter> parameter
	, ofxMachineVision::Grabber::Simple & grabber) {
	auto typedOfParameter = parameter->getParameterTyped<DataType>();
	if (typedOfParameter) {
		auto widget = make_shared<WidgetType>(*typedOfParameter);

		auto units = parameter->getUnits();
		if (!units.empty()) {
			widget->setCaption(widget->getCaption() + " [" + units + "]");
		}

		//we don't use weak pointers here, just presume that we should never arrive here if grabber closed / param disappeared
		widget->onValueChange += [&grabber, parameter](const DataType&) {
			grabber.syncToDevice(*parameter);
		};

		inspector->add(widget);
		return true;
	}
	else {
		return false;
	}
}

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetVerticalSync(true); // this seems to have no effect on windows/64/glfw currently
	ofSetFrameRate(60);

	gui.init();
	
	this->grabber.open(0);
	this->grabber.startCapture();
	const auto specification = this->grabber.getDeviceSpecification();

	gui.add(this->grabber, specification.getManufacturer() + " - " + specification.getModelName());

	auto widgets = gui.addWidgets();

	//inspector
	widgets->add(make_shared<ofxCvGui::Widgets::LiveValueHistory>("Application FPS", []() {
		return ofGetFrameRate();
	}));
	widgets->add(make_shared<ofxCvGui::Widgets::LiveValueHistory>("Grabber FPS", [this]() {
		return this->grabber.getFps();
	}));
	widgets->addIndicatorBool("Grabber device open", [this]() {
		return this->grabber.getIsDeviceOpen();
		});
	widgets->addButton("Take picture", [this]() {
		this->grabber.singleShot();
		}, ' ');
	
	//add interface parameters
	{
		const auto& parameters = this->grabber.getDeviceParameters();
		for (auto & parameter : parameters) {
			if (addParameterWidget<float, ofxCvGui::Widgets::Slider>(widgets, parameter, this->grabber)) continue;
			if (addParameterWidget<int, ofxCvGui::Widgets::EditableValue<int>>(widgets, parameter, this->grabber)) continue;
			if (addParameterWidget<bool, ofxCvGui::Widgets::Toggle>(widgets, parameter, this->grabber)) continue;
		}
	}
}

//--------------------------------------------------------------
void ofApp::update(){
	this->grabber.update();
}

//--------------------------------------------------------------
void ofApp::draw(){

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
