#pragma once

#include "EDSDK.h"
#include <string>
#include <map>

namespace ofxCanon {
	std::string errorToString(EdsError);
	std::string propertyToString(EdsPropertyID);
	std::string cameraCommandToString(EdsCameraCommand);
	
	float rationalToFloat(EdsRational);

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
}