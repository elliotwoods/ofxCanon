#pragma once

#include "EDSDK.h"

#include "ofFileUtils.h"

#include <string>
#include <map>
#include <queue>
#include <chrono>
#include <mutex>

#define ERROR_GOTO_FAIL(ERRORCODE, ACTIONNAME) \
if(ERRORCODE != EDS_ERR_OK) { \
	logError(ACTIONNAME, ERRORCODE); \
	goto fail; \
}

#define ERROR_THROW(ERRORCODE, ACTIONNAME) \
if(ERRORCODE != EDS_ERR_OK) { \
	logError(ACTIONNAME, ERRORCODE); \
	throw(ERRORCODE); \
}

#define WARNING(ERRORCODE, ACTIONNAME) \
if(ERRORCODE != EDS_ERR_OK) { \
	logWarning(ACTIONNAME, ERRORCODE); \
}

namespace ofxCanon {
	void logError(const std::string & actionName, EdsUInt32 errorCode);
	void logWarning(const std::string & actionName, EdsUInt32 errorCode);

	std::string errorToString(EdsError);
	std::string propertyToString(EdsPropertyID);
	std::string cameraCommandToString(EdsCameraCommand);

	//--
	// ISO
	//--
	//
	const std::map<EdsUInt32, int> & getISOEncodings();
	int decodeISO(EdsUInt32 isoEncoded);
	EdsUInt32 encodeISO(int isoValue);
	//
	// Notes:
	//	* Returns 0xffffffff : 0 if no setting available
	//	* 0x0 ; 0 =  Auto
	//--


	//--
	// Aperture
	//--
	//
	const std::map<EdsUInt32, float> & getApertureEncodings();
	float decodeAperture(EdsUInt32 apertureEncoded);
	EdsUInt32 encodeAperture(float apertureValue);
	//
	// Notes :
	//	* Returns 0xffffffff : 0 if no setting available
	//	* 0xffffffff ; 0 = invalid value or no settings changed
	//--


	//--
	// Shutter Speed
	//--
	//
	const std::map<EdsUInt32, float> & getShutterSpeedEncodings();
	float decodeShutterSpeed(EdsUInt32 shutterSpeedEncoded);
	EdsUInt32 encodeShutterSpeed(float shutterSpeedValue);
	//
	// Notes:
	//	* Returns 0xffffffff : 0 if no setting available
	//	* 0x0C ; 0 = Bulb
	//--

	float rationalToFloat(EdsRational);
	shared_ptr<ofBuffer> getBuffer(EdsStreamRef);

	class FramerateCounter {
	public:
		void update();
		void addFrame(chrono::high_resolution_clock::time_point = chrono::high_resolution_clock::now());
		float getFrameRate() const;
	protected:
		queue<chrono::high_resolution_clock::time_point> frameTimes;
		mutex frameTimesMutex;

		float frameRate = 0.0f;
	};
}