#include "Utils.h"
#include "ofLog.h"

#include <chrono>

using namespace std;

#define CASE_RETURN(Category, Name) case Category ## _ ## Name: return #Name
namespace ofxCanon {
	//----------
	void logError(const string & actionName, EdsUInt32 errorCode) {
		ofLogError("ofxCanon") << actionName << " failed with error " << errorToString(errorCode);
	}

	//----------
	void logWarning(const std::string & actionName, EdsUInt32 errorCode) {
		ofLogWarning("ofxCanon") << actionName << " failed with error " << errorToString(errorCode);
	}

	//----------
	string propertyToString(EdsPropertyID propertyID) {
		switch (propertyID) {
			// Camera Settings Properties
			CASE_RETURN(kEdsPropID, Unknown);
			CASE_RETURN(kEdsPropID, OwnerName);
			CASE_RETURN(kEdsPropID, MakerName);
			CASE_RETURN(kEdsPropID, DateTime);
			CASE_RETURN(kEdsPropID, FirmwareVersion);
			CASE_RETURN(kEdsPropID, BatteryLevel);
			CASE_RETURN(kEdsPropID, CFn);
			CASE_RETURN(kEdsPropID, SaveTo);
			CASE_RETURN(kEdsPropID, CurrentStorage);
			CASE_RETURN(kEdsPropID, CurrentFolder);

			CASE_RETURN(kEdsPropID, BatteryQuality);

			CASE_RETURN(kEdsPropID, BodyIDEx);
			CASE_RETURN(kEdsPropID, HDDirectoryStructure);

			// Image properties
			CASE_RETURN(kEdsPropID, ImageQuality);
			CASE_RETURN(kEdsPropID, Orientation);
			CASE_RETURN(kEdsPropID, ICCProfile);
			CASE_RETURN(kEdsPropID, FocusInfo);
			CASE_RETURN(kEdsPropID, WhiteBalance);
			CASE_RETURN(kEdsPropID, ColorTemperature);
			CASE_RETURN(kEdsPropID, WhiteBalanceShift);
			CASE_RETURN(kEdsPropID, ColorSpace);
			CASE_RETURN(kEdsPropID, PictureStyle);
			CASE_RETURN(kEdsPropID, PictureStyleDesc);
			CASE_RETURN(kEdsPropID, PictureStyleCaption);

			// Image GPS Properties
			CASE_RETURN(kEdsPropID, GPSVersionID);
			CASE_RETURN(kEdsPropID, GPSLatitudeRef);
			CASE_RETURN(kEdsPropID, GPSLatitude);
			CASE_RETURN(kEdsPropID, GPSLongitudeRef);
			CASE_RETURN(kEdsPropID, GPSLongitude);
			CASE_RETURN(kEdsPropID, GPSAltitudeRef);
			CASE_RETURN(kEdsPropID, GPSAltitude);
			CASE_RETURN(kEdsPropID, GPSTimeStamp);
			CASE_RETURN(kEdsPropID, GPSSatellites);
			CASE_RETURN(kEdsPropID, GPSStatus);
			CASE_RETURN(kEdsPropID, GPSMapDatum);
			CASE_RETURN(kEdsPropID, GPSDateStamp);

			// Capture Properties
			CASE_RETURN(kEdsPropID, AEMode);
			CASE_RETURN(kEdsPropID, DriveMode);
			CASE_RETURN(kEdsPropID, ISOSpeed);
			CASE_RETURN(kEdsPropID, MeteringMode);
			CASE_RETURN(kEdsPropID, AFMode);
			CASE_RETURN(kEdsPropID, Av);
			CASE_RETURN(kEdsPropID, Tv);
			CASE_RETURN(kEdsPropID, ExposureCompensation);
			CASE_RETURN(kEdsPropID, FocalLength);
			CASE_RETURN(kEdsPropID, AvailableShots);
			CASE_RETURN(kEdsPropID, Bracket);
			CASE_RETURN(kEdsPropID, WhiteBalanceBracket);
			CASE_RETURN(kEdsPropID, LensName);
			CASE_RETURN(kEdsPropID, AEBracket);
			CASE_RETURN(kEdsPropID, FEBracket);
			CASE_RETURN(kEdsPropID, ISOBracket);
			CASE_RETURN(kEdsPropID, NoiseReduction);
			CASE_RETURN(kEdsPropID, FlashOn);
			CASE_RETURN(kEdsPropID, RedEye);
			CASE_RETURN(kEdsPropID, FlashMode);
			CASE_RETURN(kEdsPropID, LensStatus);
			CASE_RETURN(kEdsPropID, Artist);
			CASE_RETURN(kEdsPropID, Copyright);
			CASE_RETURN(kEdsPropID, AEModeSelect);
			CASE_RETURN(kEdsPropID, PowerZoom_Speed);

			// EVF Properties
			CASE_RETURN(kEdsPropID, Evf_OutputDevice);
			CASE_RETURN(kEdsPropID, Evf_Mode);
			CASE_RETURN(kEdsPropID, Evf_WhiteBalance);
			CASE_RETURN(kEdsPropID, Evf_ColorTemperature);
			CASE_RETURN(kEdsPropID, Evf_DepthOfFieldPreview);

			// EVF IMAGE DATA Properties
			CASE_RETURN(kEdsPropID, Evf_Zoom);
			CASE_RETURN(kEdsPropID, Evf_ZoomPosition);
			CASE_RETURN(kEdsPropID, Evf_Histogram);
			CASE_RETURN(kEdsPropID, Evf_ImagePosition);
			CASE_RETURN(kEdsPropID, Evf_HistogramStatus);
			CASE_RETURN(kEdsPropID, Evf_AFMode);

			CASE_RETURN(kEdsPropID, Record);

			CASE_RETURN(kEdsPropID, Evf_HistogramY);
			CASE_RETURN(kEdsPropID, Evf_HistogramR);
			CASE_RETURN(kEdsPropID, Evf_HistogramG);
			CASE_RETURN(kEdsPropID, Evf_HistogramB);

			CASE_RETURN(kEdsPropID, Evf_CoordinateSystem);
			CASE_RETURN(kEdsPropID, Evf_ZoomRect);
			CASE_RETURN(kEdsPropID, Evf_ImageClipRect);

			CASE_RETURN(kEdsPropID, Evf_PowerZoom_CurPosition);
			CASE_RETURN(kEdsPropID, Evf_PowerZoom_MaxPosition);
			CASE_RETURN(kEdsPropID, Evf_PowerZoom_MinPosition);

		default:
			return "Unknown";
		}
	}

	//----------
	string errorToString(EdsError errorCode) {
		switch (errorCode) {

			/*-----------------------------------------------------------------------
			ED-SDK Function Success Code
			------------------------------------------------------------------------*/
			CASE_RETURN(EDS_ERR, OK);

			/*-----------------------------------------------------------------------
			ED-SDK Generic Error IDs
			------------------------------------------------------------------------*/
			/* Miscellaneous errors */
			CASE_RETURN(EDS_ERR, UNIMPLEMENTED);
			CASE_RETURN(EDS_ERR, INTERNAL_ERROR);
			CASE_RETURN(EDS_ERR, MEM_ALLOC_FAILED);
			CASE_RETURN(EDS_ERR, MEM_FREE_FAILED);
			CASE_RETURN(EDS_ERR, OPERATION_CANCELLED);
			CASE_RETURN(EDS_ERR, INCOMPATIBLE_VERSION);
			CASE_RETURN(EDS_ERR, NOT_SUPPORTED);
			CASE_RETURN(EDS_ERR, UNEXPECTED_EXCEPTION);
			CASE_RETURN(EDS_ERR, PROTECTION_VIOLATION);
			CASE_RETURN(EDS_ERR, MISSING_SUBCOMPONENT);
			CASE_RETURN(EDS_ERR, SELECTION_UNAVAILABLE);

			/* File, errors */
			CASE_RETURN(EDS_ERR, FILE_IO_ERROR);
			CASE_RETURN(EDS_ERR, FILE_TOO_MANY_OPEN);
			CASE_RETURN(EDS_ERR, FILE_NOT_FOUND);
			CASE_RETURN(EDS_ERR, FILE_OPEN_ERROR);
			CASE_RETURN(EDS_ERR, FILE_CLOSE_ERROR);
			CASE_RETURN(EDS_ERR, FILE_SEEK_ERROR);
			CASE_RETURN(EDS_ERR, FILE_TELL_ERROR);
			CASE_RETURN(EDS_ERR, FILE_READ_ERROR);
			CASE_RETURN(EDS_ERR, FILE_WRITE_ERROR);
			CASE_RETURN(EDS_ERR, FILE_PERMISSION_ERROR);
			CASE_RETURN(EDS_ERR, FILE_DISK_FULL_ERROR);
			CASE_RETURN(EDS_ERR, FILE_ALREADY_EXISTS);
			CASE_RETURN(EDS_ERR, FILE_FORMAT_UNRECOGNIZED);
			CASE_RETURN(EDS_ERR, FILE_DATA_CORRUPT);
			CASE_RETURN(EDS_ERR, FILE_NAMING_NA);

			/* Directory errors */
			CASE_RETURN(EDS_ERR, DIR_NOT_FOUND);
			CASE_RETURN(EDS_ERR, DIR_IO_ERROR);
			CASE_RETURN(EDS_ERR, DIR_ENTRY_NOT_FOUND);
			CASE_RETURN(EDS_ERR, DIR_ENTRY_EXISTS);
			CASE_RETURN(EDS_ERR, DIR_NOT_EMPTY);

			/* Property errors */
			CASE_RETURN(EDS_ERR, PROPERTIES_UNAVAILABLE);
			CASE_RETURN(EDS_ERR, PROPERTIES_MISMATCH);
			CASE_RETURN(EDS_ERR, PROPERTIES_NOT_LOADED);

			/* Function Parameter errors */
			CASE_RETURN(EDS_ERR, INVALID_PARAMETER);
			CASE_RETURN(EDS_ERR, INVALID_HANDLE);
			CASE_RETURN(EDS_ERR, INVALID_POINTER);
			CASE_RETURN(EDS_ERR, INVALID_INDEX);
			CASE_RETURN(EDS_ERR, INVALID_LENGTH);
			CASE_RETURN(EDS_ERR, INVALID_FN_POINTER);
			CASE_RETURN(EDS_ERR, INVALID_SORT_FN);

			/* Device errors */
			CASE_RETURN(EDS_ERR, DEVICE_NOT_FOUND);
			CASE_RETURN(EDS_ERR, DEVICE_BUSY);
			CASE_RETURN(EDS_ERR, DEVICE_INVALID);
			CASE_RETURN(EDS_ERR, DEVICE_EMERGENCY);
			CASE_RETURN(EDS_ERR, DEVICE_MEMORY_FULL);
			CASE_RETURN(EDS_ERR, DEVICE_INTERNAL_ERROR);
			CASE_RETURN(EDS_ERR, DEVICE_INVALID_PARAMETER);
			CASE_RETURN(EDS_ERR, DEVICE_NO_DISK);
			CASE_RETURN(EDS_ERR, DEVICE_DISK_ERROR);
			CASE_RETURN(EDS_ERR, DEVICE_CF_GATE_CHANGED);
			CASE_RETURN(EDS_ERR, DEVICE_DIAL_CHANGED);
			CASE_RETURN(EDS_ERR, DEVICE_NOT_INSTALLED);
			CASE_RETURN(EDS_ERR, DEVICE_STAY_AWAKE);
			CASE_RETURN(EDS_ERR, DEVICE_NOT_RELEASED);

			/* Stream errors */
			CASE_RETURN(EDS_ERR, STREAM_IO_ERROR);
			CASE_RETURN(EDS_ERR, STREAM_NOT_OPEN);
			CASE_RETURN(EDS_ERR, STREAM_ALREADY_OPEN);
			CASE_RETURN(EDS_ERR, STREAM_OPEN_ERROR);
			CASE_RETURN(EDS_ERR, STREAM_CLOSE_ERROR);
			CASE_RETURN(EDS_ERR, STREAM_SEEK_ERROR);
			CASE_RETURN(EDS_ERR, STREAM_TELL_ERROR);
			CASE_RETURN(EDS_ERR, STREAM_READ_ERROR);
			CASE_RETURN(EDS_ERR, STREAM_WRITE_ERROR);
			CASE_RETURN(EDS_ERR, STREAM_PERMISSION_ERROR);
			CASE_RETURN(EDS_ERR, STREAM_COULDNT_BEGIN_THREAD);
			CASE_RETURN(EDS_ERR, STREAM_BAD_OPTIONS);
			CASE_RETURN(EDS_ERR, STREAM_END_OF_STREAM);

			/* Communications errors */
			CASE_RETURN(EDS_ERR, COMM_PORT_IS_IN_USE);
			CASE_RETURN(EDS_ERR, COMM_DISCONNECTED);
			CASE_RETURN(EDS_ERR, COMM_DEVICE_INCOMPATIBLE);
			CASE_RETURN(EDS_ERR, COMM_BUFFER_FULL);
			CASE_RETURN(EDS_ERR, COMM_USB_BUS_ERR);

			/* Lock/Unlock */
			CASE_RETURN(EDS_ERR, USB_DEVICE_LOCK_ERROR);
			CASE_RETURN(EDS_ERR, USB_DEVICE_UNLOCK_ERROR);

			/* STI/WIA */
			CASE_RETURN(EDS_ERR, STI_UNKNOWN_ERROR);
			CASE_RETURN(EDS_ERR, STI_INTERNAL_ERROR);
			CASE_RETURN(EDS_ERR, STI_DEVICE_CREATE_ERROR);
			CASE_RETURN(EDS_ERR, STI_DEVICE_RELEASE_ERROR);
			CASE_RETURN(EDS_ERR, DEVICE_NOT_LAUNCHED);

			CASE_RETURN(EDS_ERR, ENUM_NA);
			CASE_RETURN(EDS_ERR, INVALID_FN_CALL);
			CASE_RETURN(EDS_ERR, HANDLE_NOT_FOUND);
			CASE_RETURN(EDS_ERR, INVALID_ID);
			CASE_RETURN(EDS_ERR, WAIT_TIMEOUT_ERROR);

			/* PTP */
			CASE_RETURN(EDS_ERR, SESSION_NOT_OPEN);
			CASE_RETURN(EDS_ERR, INVALID_TRANSACTIONID);
			CASE_RETURN(EDS_ERR, INCOMPLETE_TRANSFER);
			CASE_RETURN(EDS_ERR, INVALID_STRAGEID);
			CASE_RETURN(EDS_ERR, DEVICEPROP_NOT_SUPPORTED);
			CASE_RETURN(EDS_ERR, INVALID_OBJECTFORMATCODE);
			CASE_RETURN(EDS_ERR, SELF_TEST_FAILED);
			CASE_RETURN(EDS_ERR, PARTIAL_DELETION);
			CASE_RETURN(EDS_ERR, SPECIFICATION_BY_FORMAT_UNSUPPORTED);
			CASE_RETURN(EDS_ERR, NO_VALID_OBJECTINFO);
			CASE_RETURN(EDS_ERR, INVALID_CODE_FORMAT);
			CASE_RETURN(EDS_ERR, UNKNOWN_VENDOR_CODE);
			CASE_RETURN(EDS_ERR, CAPTURE_ALREADY_TERMINATED);
			CASE_RETURN(EDS_ERR, PTP_DEVICE_BUSY);
			CASE_RETURN(EDS_ERR, INVALID_PARENTOBJECT);
			CASE_RETURN(EDS_ERR, INVALID_DEVICEPROP_FORMAT);
			CASE_RETURN(EDS_ERR, INVALID_DEVICEPROP_VALUE);
			CASE_RETURN(EDS_ERR, SESSION_ALREADY_OPEN);
			CASE_RETURN(EDS_ERR, TRANSACTION_CANCELLED);
			CASE_RETURN(EDS_ERR, SPECIFICATION_OF_DESTINATION_UNSUPPORTED);
			CASE_RETURN(EDS_ERR, NOT_CAMERA_SUPPORT_SDK_VERSION);

			/* PTP Vendor */
			CASE_RETURN(EDS_ERR, UNKNOWN_COMMAND);
			CASE_RETURN(EDS_ERR, OPERATION_REFUSED);
			CASE_RETURN(EDS_ERR, LENS_COVER_CLOSE);
			CASE_RETURN(EDS_ERR, LOW_BATTERY);
			CASE_RETURN(EDS_ERR, OBJECT_NOTREADY);
			CASE_RETURN(EDS_ERR, CANNOT_MAKE_OBJECT);
			CASE_RETURN(EDS_ERR, MEMORYSTATUS_NOTREADY);

			/* Take Picture errors */
			CASE_RETURN(EDS_ERR, TAKE_PICTURE_AF_NG);
			CASE_RETURN(EDS_ERR, TAKE_PICTURE_RESERVED);
			CASE_RETURN(EDS_ERR, TAKE_PICTURE_MIRROR_UP_NG);
			CASE_RETURN(EDS_ERR, TAKE_PICTURE_SENSOR_CLEANING_NG);
			CASE_RETURN(EDS_ERR, TAKE_PICTURE_SILENCE_NG);
			CASE_RETURN(EDS_ERR, TAKE_PICTURE_NO_CARD_NG);
			CASE_RETURN(EDS_ERR, TAKE_PICTURE_CARD_NG);
			CASE_RETURN(EDS_ERR, TAKE_PICTURE_CARD_PROTECT_NG);
			CASE_RETURN(EDS_ERR, TAKE_PICTURE_MOVIE_CROP_NG);
			CASE_RETURN(EDS_ERR, TAKE_PICTURE_STROBO_CHARGE_NG);
			CASE_RETURN(EDS_ERR, TAKE_PICTURE_NO_LENS_NG);
			CASE_RETURN(EDS_ERR, TAKE_PICTURE_SPECIAL_MOVIE_MODE_NG);
			CASE_RETURN(EDS_ERR, TAKE_PICTURE_LV_REL_PROHIBIT_MODE_NG);

		default:
			return "Unknown";
		}
	}

	//----------
	std::string cameraCommandToString(EdsCameraCommand cameraCommand) {
		switch (cameraCommand) {
			CASE_RETURN(kEdsCameraCommand, TakePicture);
			CASE_RETURN(kEdsCameraCommand, ExtendShutDownTimer);
			CASE_RETURN(kEdsCameraCommand, BulbStart);
			CASE_RETURN(kEdsCameraCommand, BulbEnd);
			CASE_RETURN(kEdsCameraCommand, DoEvfAf);
			CASE_RETURN(kEdsCameraCommand, DriveLensEvf);
			CASE_RETURN(kEdsCameraCommand, DoClickWBEvf);

			CASE_RETURN(kEdsCameraCommand, PressShutterButton);
			CASE_RETURN(kEdsCameraCommand, DrivePowerZoom);
			CASE_RETURN(kEdsCameraCommand, SetRemoteShootingMode);

		default:
			return "Unknown";
		}
	}

	//----------
	template<typename RawType>
	RawType decode(const map<EdsUInt32, RawType> & encodings, EdsUInt32 encoded) {
		auto findIterator = encodings.find(encoded);
		if (findIterator != encodings.end()) {
			return findIterator->second;
		}
		else {
			//error value
			return 0;
		}
	}

	//----------
	template<typename RawType>
	EdsUInt32 encode(const map<EdsUInt32, RawType> & encodings, RawType value) {
		for (auto & encodedIterator : encodings) {
			if (encodedIterator.second == value) {
				return encodedIterator.first;
			}
		}

		//error value
		return 0xffffffff;
	}

	//----------
	const map<EdsUInt32, int> & getISOEncodings() {
		static map<EdsUInt32, int> ISOEncodings;
		if (ISOEncodings.empty()) {
			ISOEncodings[0x00000000] = 0;
			ISOEncodings[0x00000040] = 50;
			ISOEncodings[0x00000048] = 100;
			ISOEncodings[0x0000004b] = 125;
			ISOEncodings[0x0000004d] = 160;
			ISOEncodings[0x00000050] = 200;
			ISOEncodings[0x00000053] = 250;
			ISOEncodings[0x00000055] = 320;
			ISOEncodings[0x00000058] = 400;
			ISOEncodings[0x0000005b] = 500;
			ISOEncodings[0x0000005d] = 640;
			ISOEncodings[0x00000060] = 800;
			ISOEncodings[0x00000063] = 1000;
			ISOEncodings[0x00000065] = 1250;
			ISOEncodings[0x00000068] = 1600;
			ISOEncodings[0x0000006b] = 2000;
			ISOEncodings[0x0000006d] = 2500;
			ISOEncodings[0x00000070] = 3200;
			ISOEncodings[0x00000073] = 4000;
			ISOEncodings[0x00000075] = 5000;
			ISOEncodings[0x00000078] = 6400;
			ISOEncodings[0x0000007b] = 8000;
			ISOEncodings[0x0000007d] = 10000;
			ISOEncodings[0x00000080] = 12800;
			ISOEncodings[0x00000088] = 25600;
			ISOEncodings[0x00000090] = 51200;
			ISOEncodings[0x00000098] = 102400;
			ISOEncodings[0x000000a0] = 204800;
			ISOEncodings[0x000000a8] = 409600;
		}
		return ISOEncodings;
	}

	//----------
	int decodeISO(EdsUInt32 isoEncoded) {
		return decode(getISOEncodings(), isoEncoded);
	}

	//----------
	EdsUInt32 encodeISO(int isoValue) {
		return encode(getISOEncodings(), isoValue);
	}

#pragma warning (disable : 4305)
	//----------
	const std::map<EdsUInt32, float> & getApertureEncodings() {
		static map<EdsUInt32, float> ApertureEncodings;
		if (ApertureEncodings.empty()) {
			ApertureEncodings[0x08] = 1;
			ApertureEncodings[0x0B] = 1.1;
			ApertureEncodings[0x0C] = 1.2;
			ApertureEncodings[0x0D] = 1.2; // thirds
			ApertureEncodings[0x10] = 1.4;
			ApertureEncodings[0x13] = 1.6;
			ApertureEncodings[0x14] = 1.8;
			ApertureEncodings[0x15] = 1.8; // thirds
			ApertureEncodings[0x18] = 2;
			ApertureEncodings[0x1B] = 2.2;
			ApertureEncodings[0x1C] = 2.5;
			ApertureEncodings[0x1D] = 2.5; // thirds
			ApertureEncodings[0x20] = 2.8;
			ApertureEncodings[0x23] = 3.2;
			ApertureEncodings[0x24] = 3.5;
			ApertureEncodings[0x25] = 3.5; // thirds
			ApertureEncodings[0x28] = 4;
			ApertureEncodings[0x2B] = 4.5;
			ApertureEncodings[0x2C] = 4.5; // thirds
			ApertureEncodings[0x2D] = 5.0;
			ApertureEncodings[0x30] = 5.6;
			ApertureEncodings[0x33] = 6.3;
			ApertureEncodings[0x34] = 6.7;
			ApertureEncodings[0x35] = 7.1;
			ApertureEncodings[0x38] = 8;
			ApertureEncodings[0x3B] = 9;
			ApertureEncodings[0x3C] = 9.5;
			ApertureEncodings[0x3D] = 10;
			ApertureEncodings[0x40] = 11;
			ApertureEncodings[0x43] = 13;
			ApertureEncodings[0x44] = 13; // thirds
			ApertureEncodings[0x45] = 14;
			ApertureEncodings[0x48] = 16;
			ApertureEncodings[0x4B] = 18;
			ApertureEncodings[0x4C] = 19;
			ApertureEncodings[0x4D] = 20;
			ApertureEncodings[0x50] = 22;
			ApertureEncodings[0x53] = 25;
			ApertureEncodings[0x54] = 27;
			ApertureEncodings[0x55] = 29;
			ApertureEncodings[0x58] = 32;
			ApertureEncodings[0x5B] = 36;
			ApertureEncodings[0x5C] = 38;
			ApertureEncodings[0x5D] = 40;
			ApertureEncodings[0x60] = 45;
			ApertureEncodings[0x63] = 51;
			ApertureEncodings[0x64] = 54;
			ApertureEncodings[0x65] = 57;
			ApertureEncodings[0x68] = 64;
			ApertureEncodings[0x6B] = 72;
			ApertureEncodings[0x6C] = 76;
			ApertureEncodings[0x6D] = 80;
			ApertureEncodings[0x70] = 91;
			ApertureEncodings[0xffffffff] = 0;
		}
		return ApertureEncodings;
	}
#pragma warning (default: 4305)

	//----------
	float decodeAperture(EdsUInt32 apertureEncoded) {
		return decode(getApertureEncodings(), apertureEncoded);
	}

	//----------
	EdsUInt32 encodeAperture(float apertureValue) {
		return encode(getApertureEncodings(), apertureValue);
	}

#pragma warning (disable : 4305)
	//----------
	const std::map<EdsUInt32, float> & getShutterSpeedEncodings() {
		static map<EdsUInt32, float> ShutterSpeedEncodings;
		if (ShutterSpeedEncodings.empty()) {
			ShutterSpeedEncodings[0x0C] = 0;
			ShutterSpeedEncodings[0x10] = 30;
			ShutterSpeedEncodings[0x13] = 25;
			ShutterSpeedEncodings[0x14] = 20;
			ShutterSpeedEncodings[0x15] = 20.3;
			ShutterSpeedEncodings[0x18] = 15;
			ShutterSpeedEncodings[0x1B] = 13;
			ShutterSpeedEncodings[0x1C] = 10;
			ShutterSpeedEncodings[0x1D] = 10.3;
			ShutterSpeedEncodings[0x20] = 8;
			ShutterSpeedEncodings[0x23] = 6.3;
			ShutterSpeedEncodings[0x24] = 6;
			ShutterSpeedEncodings[0x25] = 5;
			ShutterSpeedEncodings[0x28] = 4;
			ShutterSpeedEncodings[0x2B] = 3.2;
			ShutterSpeedEncodings[0x2C] = 3;
			ShutterSpeedEncodings[0x2D] = 2.5;
			ShutterSpeedEncodings[0x30] = 2;
			ShutterSpeedEncodings[0x33] = 1.6;
			ShutterSpeedEncodings[0x34] = 1.5;
			ShutterSpeedEncodings[0x35] = 1.3;
			ShutterSpeedEncodings[0x38] = 1;
			ShutterSpeedEncodings[0x3B] = 0.8;
			ShutterSpeedEncodings[0x3C] = 0.7;
			ShutterSpeedEncodings[0x3D] = 0.6;
			ShutterSpeedEncodings[0x40] = 0.5;
			ShutterSpeedEncodings[0x43] = 0.4;
			ShutterSpeedEncodings[0x44] = 0.3;
			ShutterSpeedEncodings[0x45] = 0.33;
			ShutterSpeedEncodings[0x48] = 1.0 / 4.0;
			ShutterSpeedEncodings[0x4B] = 1.0 / 5.0;
			ShutterSpeedEncodings[0x4D] = 1.0 / 6.0; // discovered empirically
			ShutterSpeedEncodings[0x53] = 1.0 / 10.0; // discovered empirically
			ShutterSpeedEncodings[0x55] = 1.0 / 13.0; // discovered empirically
			ShutterSpeedEncodings[0x5D] = 1.0 / 25.0;
			ShutterSpeedEncodings[0x60] = 1.0 / 30.0;
			ShutterSpeedEncodings[0x63] = 1.0 / 40.0;
			ShutterSpeedEncodings[0x64] = 1.0 / 45.0;
			ShutterSpeedEncodings[0x65] = 1.0 / 50.0;
			ShutterSpeedEncodings[0x68] = 1.0 / 60.0;
			ShutterSpeedEncodings[0x6B] = 1.0 / 80.0;
			ShutterSpeedEncodings[0x6C] = 1.0 / 90.0;
			ShutterSpeedEncodings[0x6D] = 1.0 / 100.0;
			ShutterSpeedEncodings[0x70] = 1.0 / 125.0;
			ShutterSpeedEncodings[0x73] = 1.0 / 160.0;
			ShutterSpeedEncodings[0x74] = 1.0 / 180.0;
			ShutterSpeedEncodings[0x75] = 1.0 / 200.0;
			ShutterSpeedEncodings[0x78] = 1.0 / 250.0;
			ShutterSpeedEncodings[0x7B] = 1.0 / 320.0;
			ShutterSpeedEncodings[0x7C] = 1.0 / 350.0;
			ShutterSpeedEncodings[0x7D] = 1.0 / 400.0;
			ShutterSpeedEncodings[0x80] = 1.0 / 500.0;
			ShutterSpeedEncodings[0x83] = 1.0 / 640.0;
			ShutterSpeedEncodings[0x84] = 1.0 / 750.0;
			ShutterSpeedEncodings[0x85] = 1.0 / 800.0;
			ShutterSpeedEncodings[0x88] = 1.0 / 1000.0;
			ShutterSpeedEncodings[0x8B] = 1.0 / 1250.0;
			ShutterSpeedEncodings[0x8C] = 1.0 / 1500.0;
			ShutterSpeedEncodings[0x8D] = 1.0 / 1600.0;
			ShutterSpeedEncodings[0x90] = 1.0 / 2000.0;
			ShutterSpeedEncodings[0x93] = 1.0 / 2500.0;
			ShutterSpeedEncodings[0x94] = 1.0 / 3000.0;
			ShutterSpeedEncodings[0x95] = 1.0 / 3200.0;
			ShutterSpeedEncodings[0x98] = 1.0 / 4000.0;
			ShutterSpeedEncodings[0x9B] = 1.0 / 5000.0;
		}
		return ShutterSpeedEncodings;
	}
#pragma warning (default: 4305)

	//----------
	float decodeShutterSpeed(EdsUInt32 shutterSpeedEncoded) {
		return decode(getShutterSpeedEncodings(), shutterSpeedEncoded);
	}

	//----------
	EdsUInt32 encodeShutterSpeed(float shutterSpeedValue) {
		return encode(getShutterSpeedEncodings(), shutterSpeedValue);
	}

	//----------
	float rationalToFloat(EdsRational rational) {
		return (float)rational.numerator / (float)rational.denominator;
	}

	//----------
	shared_ptr<ofBuffer> getBuffer(EdsStreamRef stream) {
		shared_ptr<ofBuffer> buffer;

		EdsUInt64 streamLength;
		ERROR_THROW(EdsGetLength(stream, &streamLength)
			, "Get encoded stream length (i.e. file size)");
		char * encodedData = NULL;
		ERROR_THROW(EdsGetPointer(stream, (EdsVoid**)& encodedData)
			, "Get pointer to encoded data");
		return make_shared<ofBuffer>(encodedData, streamLength);
	}

#pragma mark FramerateCounter
	//----------
	void FramerateCounter::update() {
		unique_lock<mutex> lock(this->frameTimesMutex);
		if (!this->frameTimes.empty()) {
			auto now = chrono::high_resolution_clock::now();
			auto old = this->frameTimes.front();
			auto size = this->frameTimes.size();
			while (this->frameTimes.size() > 30) {
				this->frameTimes.pop();
			}
			this->frameRate = (double)(size - 1) * 1e6 / (double)chrono::duration_cast<chrono::microseconds>(now - old).count();
		}
	}

	//----------
	void FramerateCounter::addFrame(chrono::high_resolution_clock::time_point timePoint) {
		unique_lock<mutex> lock(this->frameTimesMutex);
		this->frameTimes.push(timePoint);
	}

	//----------
	float FramerateCounter::getFrameRate() const {
		return this->frameRate;
	}
}
