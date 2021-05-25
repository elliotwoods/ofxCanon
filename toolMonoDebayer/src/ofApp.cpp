#include "ofApp.h"
#include "FreeImage.h"
#include "ofxCvMin.h"
#include "ofxMachineVision/Device/Canon.h"

//--------------------------------------------------------------
void ofApp::setup() {
	// Initialize device
	{
		auto devices = ofxCanon::listDevices();

		for (auto device : devices) {
			device->open();
			this->device = device;
			break;
		}
		if (!this->device) {
			ofLogWarning() << "Did not detect any Canon cameras connected via USB. Offline processing mode only.";
		}
	}

	// Initialize GUI
	{
		this->gui.init();

		auto strip = this->gui.addStrip();
		strip->setCellSizes({ -1, -1, 300 });

		{
			auto imagePanel = ofxCvGui::Panels::makeImage(this->raw, "RAW bayer image");
			strip->add(imagePanel);
			imagePanel->onDraw += [this](ofxCvGui::DrawArguments& args) {
				if (!this->raw.isAllocated()) {
					ofxCvGui::Utils::drawText("Use the 'Take photo' button if a camera is attached.\nOr drag in a RAW image or folder of images into this window."
						, args.localBounds
						, true
						, false);
				}
			};
		}

		auto resultsStrip = ofxCvGui::Panels::Groups::makeStrip();
		{
			resultsStrip->setDirection(ofxCvGui::Panels::Groups::Strip::Direction::Vertical);
			strip->add(resultsStrip);
		}
		{
			this->resultPanel = ofxCvGui::Panels::makeImage(this->result, "Result");
			resultsStrip->add(this->resultPanel);
		}
		{
			this->standardProcessPanel = ofxCvGui::Panels::makeImage(this->standardProcess, "Standard process");
			resultsStrip->add(this->standardProcessPanel);
		}
		{
			auto widgetsPanel = ofxCvGui::Panels::makeWidgets();
			strip->add(widgetsPanel);
			widgetsPanel->addIndicatorBool("Device connected", [this]() {
				return (bool)this->device;
				});
			if (this->device) {
				widgetsPanel->addButton("Take photo ", [this] {
					this->takePhoto();
					}, ' ');
			}

			widgetsPanel->addParameterGroup(this->parameters);
		}
	}

	this->raw.getTexture().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
	this->result.getTexture().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
}

//--------------------------------------------------------------
void ofApp::update(){
	if (device) {
		device->update();
	}

	this->standardProcessPanel->setImageZoomState(this->resultPanel->getImageZoomState());
	this->standardProcessPanel->setScroll(this->resultPanel->getScroll());

	if (this->standardProcess.isAllocated()) {
		if (this->standardProcess.getPixels().getNumChannels() == 3) {
			//downsample to greyscale
			ofPixels grayscale;
			grayscale.allocate(this->standardProcess.getWidth(), this->standardProcess.getHeight(), ofImageType::OF_IMAGE_GRAYSCALE);
			auto size = this->standardProcess.getWidth() * this->standardProcess.getHeight();
			auto in = this->standardProcess.getPixels().getData();
			auto out = grayscale.getData();
			for (int i = 0; i < size; i++) {
				*out++ = in[0] / 3 + in[1] / 3 + in[2] / 3;
				in += 3;
			}

			this->standardProcess.getPixels() = grayscale;
			this->standardProcess.update();
		}
	}
}

//--------------------------------------------------------------
void ofApp::draw(){

}

//--------------------------------------------------------------
void ofApp::takePhoto() {
	ofShortPixels pixels16;
	auto future = this->device->takePhotoAsync();
	while (future.wait_for(chrono::milliseconds(10)) != future_status::ready) {
		glfwPollEvents();
		this->device->update();
	}

	auto result = future.get();
	if (result) {
		ofImageLoadSettings imageLoadSettings;
		{
			imageLoadSettings.freeImageFlags = RAW_UNPROCESSED;
		}
		ofLoadImage(this->raw.getPixels()
			, *result.encodedBuffer
			, imageLoadSettings);
		ofLoadImage(this->standardProcess
			, *result.encodedBuffer);
		this->standardProcess.update();

		this->raw.update();
	}

	this->process();

	if (this->parameters.saveOnProcess) {
		// new photo image name
		char filenameMain[80];
		{
			time_t rawtime;
			struct tm* timeinfo;

			time(&rawtime);
			timeinfo = localtime(&rawtime);

			strftime(filenameMain, 80, "%Y%m%d_%H%M", timeinfo);
		}

		auto filename = string(filenameMain) + "." + this->parameters.outputFileType.get();
		ofSaveImage(this->result, filename);
		cout << "Saved to : " << filename << std::endl;
	}
}

//--------------------------------------------------------------
void ofApp::process(){
	this->result = this->raw;

	if (this->parameters.normalize.enabled) {
		ofxMachineVision::Device::Canon::normalize(this->result.getPixels()
			, this->parameters.normalize.percentile.get()
			, this->parameters.normalize.ignoreTop.get()
			, this->parameters.normalize.normalizeTo.get());
	}

	if (this->parameters.monoDebayer.enabled) {
		auto image = ofxCv::toCv(this->result.getPixels());
		ofxMachineVision::Device::Canon::processRawMono(image
			, this->parameters.monoDebayer.dilationIterations.get());
	}

	this->result.update();
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {
	for (const auto& path : dragInfo.files) {
		if (ofFile(path, ofFile::Mode::Reference).isDirectory()) {
			ofDirectory dir(path);
			for (auto& file : dir) {
				this->processFile(file.getAbsolutePath());
			}
		}
		else {
			this->processFile(path);
		}
		
	}

	this->raw.update();
	this->standardProcess.update();
}

//--------------------------------------------------------------
void ofApp::processFile(const string& filename) {
	if (ofToLower(ofFilePath::getFileExt(filename)) != "cr2") {
		cout << "Ignoring " << filename << " (file must have extension CR2)." << std::endl;
		return;
	}

	ofImageLoadSettings imageLoadSettings;
	{
		imageLoadSettings.freeImageFlags = RAW_UNPROCESSED;
	}

	ofLoadImage(this->raw.getPixels()
		, filename
		, imageLoadSettings);
	ofLoadImage(this->standardProcess
		, filename);

	this->process();

	auto outputFilename = ofFilePath::removeExt(filename) + "." + this->parameters.outputFileType.get();
	ofSaveImage(this->result.getPixels(), outputFilename);
	cout << "Saved to : " << outputFilename << std::endl;
}