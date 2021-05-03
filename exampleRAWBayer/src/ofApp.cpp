#include "ofApp.h"
#include "FreeImage.h"
#include "ofxCvMin.h"

//--------------------------------------------------------------
void ofApp::setup() {
	auto devices = ofxCanon::listDevices();

	this->gui.init();

	auto strip = this->gui.addStrip();
	strip->setCellSizes({ -1, -1, 300 });

	{
		auto imagePanel = ofxCvGui::Panels::makeImage(this->image, "RAW bayer image");
		strip->add(imagePanel);

		auto imagePanelWeak = std::weak_ptr<ofxCvGui::Panels::Image>(imagePanel);
		imagePanel->onMouse += [this, imagePanelWeak](ofxCvGui::MouseArguments& args) {
			auto imagePanel = imagePanelWeak.lock();
			glm::vec4 mouse4{ args.local.x, args.local.y, 0.0f, 1.0f };
			auto mouseInImage = glm::inverse(imagePanel->getPanelToImageTransform()) * mouse4;
			mouseInImage.x = floor(mouseInImage.x);
			mouseInImage.y = floor(mouseInImage.y);
			this->mousePositionInImage = mouseInImage;
		};
		imagePanel->onKeyboard += [this, imagePanelWeak](ofxCvGui::KeyboardArguments& args) {
			auto selection = this->parameters.selection.get();

			switch (args.key) {
			case 'a':
			case 'A':
				selection.x = this->mousePositionInImage.x;
				selection.y = this->mousePositionInImage.y;
				break;
			case 'b':
			case 'B':
				selection.width = this->mousePositionInImage.x - selection.x;
				selection.height = this->mousePositionInImage.y - selection.y;
				break;
			}

			this->parameters.selection.set(selection);
		};

		imagePanel->onDrawImage += [this](ofxCvGui::DrawImageArguments& args) {
			ofPushStyle();
			{
				ofNoFill();
				ofSetLineWidth(1.0f);
				ofDrawCircle(this->mousePositionInImage, 5);
				ofDrawRectangle(this->parameters.selection);
			}
			ofPopStyle();
		};
	}
	{
		auto resultsPanel = ofxCvGui::Panels::makeImage(this->result, "Result");
		strip->add(resultsPanel);
	}
	{
		auto widgetsPanel = ofxCvGui::Panels::makeWidgets();
		strip->add(widgetsPanel);
		widgetsPanel->addIndicatorBool("Device connected", [this]() {
			return (bool) this->device;
			});
		widgetsPanel->addButton("Take photo ", [this] {
			this->takePhoto();
			}, ' ');
		widgetsPanel->addButton("Calibrate white balance", [this] {
			this->calibrateWhiteBalance();
			});
		widgetsPanel->addButton("Calc result from white balance", [this] {
			this->calculateResultFromWhiteBalance();
			});
		widgetsPanel->addButton("Calc result from blur", [this] {
			this->calculateResultFromBlurKernel();
			});
		widgetsPanel->addButton("Calc result auto", [this] {
			this->calculateResultAuto();
			});
		widgetsPanel->addTitle("A = selection top left", ofxCvGui::Widgets::Title::Level::H3);
		widgetsPanel->addTitle("B = selection bottom right", ofxCvGui::Widgets::Title::Level::H3);

		widgetsPanel->addParameterGroup(this->parameters);
	}

	for (auto device : devices) {
		device->open();
		this->device = device;
		break;
	}
	if (!this->device) {
		ofSystemAlertDialog("No camera detected");
		ofExit();
	}

	this->image.getTexture().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
	this->result.getTexture().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
}

//--------------------------------------------------------------
void ofApp::update(){
	if (device) {
		device->update();
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
		ofLoadImage(this->image.getPixels()
			, *result.encodedBuffer
			, imageLoadSettings);

		if (this->parameters.normalize) {
			auto& pixels = this->image.getPixels();
			auto data = pixels.getData();

			std::vector<uint16_t> values;

			for (size_t i = 0; i < pixels.size(); i+= 16) {
				values.push_back(data[i]);
			}

			std::sort(values.begin(), values.end());

			// take 99% percentile as max
			auto maxValue = values.at(values.size() * 99 / 100);

			float normFactor = (float)std::numeric_limits<uint16_t>::max() / (float)maxValue;
			for (size_t i = 0; i < pixels.size(); i++) {
				data[i] = (uint16_t) ((float) data[i] * normFactor);
			}
		}

		this->image.update();
	}
}

enum class Color {
	Green = 0
	, Red = 1
	, Blue = 2
};

Color getColor(int x, int y) {
	if (x % 2 == y % 2) {
		return Color::Green;
	}
	else if (x % 2 == 1) {
		return Color::Blue;
	}
	else {
		return Color::Red;
	}
}

//--------------------------------------------------------------
void ofApp::calibrateWhiteBalance() {
	if (!this->image.isAllocated()) {
		ofSystemAlertDialog("Image is not allocated");
		return;
	}

	auto & selection = this->parameters.selection.get();
	if (selection.width <= 0 || selection.height <= 0) {
		ofSystemAlertDialog("Your selection width/height need to be greater than 0");
		return;
	}
	ofRectangle imageBounds(0, 0, this->image.getWidth(), this->image.getHeight());
	if (!imageBounds.inside(selection.getTopLeft())
		|| !imageBounds.inside(selection.getBottomRight())) {
		ofSystemAlertDialog("Your selection is outside the image bounds");
		return;
	}

	vector<float> blueFactors, redFactors;

	const auto& pixels = this->image.getPixels();
	const auto data = pixels.getData();
	const auto width = pixels.getWidth();

	auto sampleAdjacent = [&pixels, data, width](size_t x, size_t y) {
		float total = 0;
		float count = 0;
		if (x - 1 >= 0) {
			total += data[(x - 1) + y * width];
			count++;
		}
		if (x + 1 < pixels.getWidth()) {
			total += data[(x + 1) + y * width];
			count++;
		}
		if (y - 1 >= 0) {
			total += data[x + (y - 1) * width];
			count++;
		}
		if (y + 1 < pixels.getHeight()) {
			total += data[x + (y + 1) * width];
			count++;
		}

		return total / count;
	};

	for (size_t y = selection.y; y < selection.getBottom(); y++) {
		for (size_t x = selection.x; x < selection.getRight(); x++) {
			auto color = getColor(x, y);

			if (color == Color::Green) {
				continue;
			}

			// take the surrounding green tiles (up/down/left/right are green for red and blue)
			auto meanAdjacentGreen = sampleAdjacent(x, y);
			auto factor = meanAdjacentGreen / (float) data[x + y * width];
			switch (color) {
			case Color::Red:
				redFactors.push_back(factor);
				break;
			case Color::Blue:
				blueFactors.push_back(factor);
				break;
			}
		}
	}

	auto mean = [](const vector<float>& data) {
		float accumulator = 0.0f;
		float count = 0;
		for (const auto& data : data) {
			accumulator += data;
			count++;
		}
		return accumulator / count;
	};

	auto standardDeviation = [](const vector<float>& data, const float & mean) {
		float accumulator = 0.0f;
		float count = 0;
		for (const auto& data : data) {
			auto delta = data - mean;
			accumulator += delta * delta;
			count++;
		}
		return accumulator / count;
	};

	auto trimDataSet = [](const vector<float>& data, const float& mean, const float& maxDeviation2) {
		vector<float> selectedData;
		for (const auto& data : data) {
			auto delta = data - mean;
			if (delta * delta <= maxDeviation2) {
				selectedData.push_back(data);
			}
		}
		return selectedData;
	};

	auto takeTrimmedAverage = [&](const vector<float>& data, const float standardDeviations) {
		auto priorMean = mean(data);
		auto priorStandardDeviation = standardDeviation(data, priorMean);
		auto trimmedDataSet = trimDataSet(data, priorMean, priorStandardDeviation * standardDeviations);
		return mean(trimmedDataSet);
	};

	this->parameters.calibration.redFactor = takeTrimmedAverage(redFactors, this->parameters.calibration.standardDeviations);
	this->parameters.calibration.blueFactor = takeTrimmedAverage(blueFactors, this->parameters.calibration.standardDeviations);
}

//--------------------------------------------------------------
void ofApp::calculateResultFromWhiteBalance() {
	//allocate the result
	this->result.getPixels() = this->image.getPixels();
	auto& pixels = this->result.getPixels();

	auto input = this->image.getPixels().getData();
	auto output = this->result.getPixels().getData();

	const auto redFactor = this->parameters.calibration.redFactor.get();
	const auto blueFactor = this->parameters.calibration.blueFactor.get();
	for (size_t y = 0; y < pixels.getHeight(); y++) {
		for (size_t x = 0; x < pixels.getWidth(); x++) {
			auto color = getColor(x, y);
			switch (color) {
			case Color::Green:
				*output = *input;
				break;
			case Color::Red:
			case Color::Blue:
				const auto& factor = color == Color::Red
					? redFactor
					: blueFactor;
				auto result = floor((float)*input * (float)factor);
				if (result > (float)std::numeric_limits<uint16_t>::max()) {
					*output = std::numeric_limits<uint16_t>::max();
				}
				else {
					*output = (uint16_t)result;
				}
				break;
			}

			input++;
			output++;
		}
	}

	result.update();
}

//--------------------------------------------------------------
void ofApp::calculateResultFromBlurKernel() {
	//allocate the result
	this->result.getPixels() = this->image.getPixels();
	auto& pixels = this->result.getPixels();

	auto input = this->image.getPixels().getData();
	auto output = this->result.getPixels().getData();

	auto width = pixels.getWidth();
	auto height = pixels.getHeight();
	for (size_t y = 0; y < height; y++) {
		for (size_t x = 0; x < width; x++) {
			if (x > 0
				&& y > 0
				&& x < width - 1
				&& y < height - 1) {
				auto accumulate = 0;
				accumulate += (uint32_t)*input;
				accumulate += (uint32_t)*(input - 1);
				accumulate += (uint32_t)*(input + 1);
				accumulate += (uint32_t)*(input - width);
				accumulate += (uint32_t)*(input + width);
				*output = (uint16_t)(accumulate / 5);
			}
			
			input++;
			output++;
		}
	}

	result.update();
}

//--------------------------------------------------------------
void ofApp::calculateResultAuto() {
	auto inputPixels = this->image.getPixels();
	auto width = inputPixels.getWidth();
	auto height = inputPixels.getHeight();
	auto input = ofxCv::toCv(inputPixels);

	//create masks for red, green, blue planes
	cv::Mat redMask(height, width, CV_8U);
	cv::Mat greenMask(height, width, CV_8U);
	cv::Mat blueMask(height, width, CV_8U);
	{
		redMask.setTo(cv::Scalar(0));
		greenMask.setTo(cv::Scalar(0));
		blueMask.setTo(cv::Scalar(0));

		auto redOut = redMask.data;
		auto greenOut = greenMask.data;
		auto blueOut = blueMask.data;

		for (size_t y = 0; y < height; ++y) {
			for (size_t x = 0; x < width; ++x) {
				auto color = getColor(x, y);
				switch (color) {
				case Color::Red:
					*redOut = 255;
					break;
				case Color::Green:
					*greenOut = 255;
					break;
				case Color::Blue:
					*blueOut = 255;
					break;
				}

				redOut++;
				greenOut++;
				blueOut++;
			}
		}
	}
	
	//extract red, green, blue planes
	cv::Mat redPlane(height, width, CV_16U);
	cv::Mat greenPlane(height, width, CV_16U);
	cv::Mat bluePlane(height, width, CV_16U);
	{
		redPlane.setTo(cv::Scalar(0));
		greenPlane.setTo(cv::Scalar(0));
		bluePlane.setTo(cv::Scalar(0));

		cv::copyTo(input, redPlane, redMask);
		cv::copyTo(input, greenPlane, greenMask);
		cv::copyTo(input, bluePlane, blueMask);
	}

	//dilate the planes
	{
		auto crossKernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));
		auto boxKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
		cv::dilate(redPlane, redPlane, boxKernel);
		cv::dilate(greenPlane, greenPlane, crossKernel);
		cv::dilate(bluePlane, bluePlane, boxKernel);
	}

	//promote resolution to float for each plane
	cv::Mat redPlaneFloat, greenPlaneFloat, bluePlaneFloat;
	{
		redPlane.convertTo(redPlaneFloat, CV_32F);
		greenPlane.convertTo(greenPlaneFloat, CV_32F);
		bluePlane.convertTo(bluePlaneFloat, CV_32F);
	}

	//create factors as planes
	cv::Mat redFactor, blueFactor;
	{
		redFactor = greenPlaneFloat / redPlaneFloat;
		blueFactor = greenPlaneFloat / bluePlaneFloat;
	}

	//blue the factor planes
	{
		auto blurSize = cv::Size(this->parameters.calibration.blurRadius.get(), this->parameters.calibration.blurRadius.get());
		cv::blur(redFactor, redFactor, blurSize);
		cv::blur(blueFactor, blueFactor, blurSize);
	}

	//mask copy the factors back into a combined factor plane
	cv::Mat factor(height, width, CV_32F);
	{
		factor.setTo(cv::Scalar(1.0f));
		cv::copyTo(redFactor, factor, redMask);
		cv::copyTo(blueFactor, factor, blueMask);
	}

	//apply the factor plane
	{
		this->result.allocate(width, height, ofImageType::OF_IMAGE_GRAYSCALE);
		auto result = ofxCv::toCv(this->result);
		cv::Mat resultFloat;
		input.convertTo(resultFloat, CV_32F);
		resultFloat = resultFloat.mul(factor);
		resultFloat.convertTo(result, CV_16U);
	}

	this->result.update();
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
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

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
