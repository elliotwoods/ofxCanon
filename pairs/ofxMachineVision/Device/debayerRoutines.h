#pragma once

namespace ofxMachineVision {
	namespace Device {

		template<typename PixelsType>
		void Canon::normalize(ofPixels_<PixelsType>& pixels, float percentile, float ignoreTop, float normalizeTo) {
			if (percentile > 1.0f || percentile < 0.0f) {
				throw(ofxMachineVision::Exception("Percentile parameter is out of range"));
			}

			if (ignoreTop >= 1.0f || ignoreTop < 0.0f) {
				throw(ofxMachineVision::Exception("Ignore top percentile parameter is out of range"));
			}

			if (percentile == 1.0f) {
				// use opencv in this situation
				auto image = ofxCv::toCv(pixels);
				cv::normalize(image
					, image
					, (float)std::numeric_limits<PixelsType>::max() * normalizeTo
					, 0.0
					, cv::NormTypes::NORM_INF);
			}
			else {
				// do it ourselves otherwise

				auto data = pixels.getData();

				std::vector<PixelsType> values;

				// skip to every 16th pixel to speed up
				for (size_t i = ignoreTop * (float)pixels.size(); i < pixels.size(); i += 16) {
					values.push_back(data[i]);
				}

				std::sort(values.begin(), values.end());

				auto maxValue = values.at(((float)values.size() - 1) * percentile);

				float normFactor = (float)std::numeric_limits<PixelsType>::max() / (float)maxValue * normalizeTo;
				for (size_t i = 0; i < pixels.size(); i++) {
					data[i] = (PixelsType)(min((float)data[i] * normFactor, (float) std::numeric_limits<PixelsType>::max()));
				}
			}
		}
	}
}