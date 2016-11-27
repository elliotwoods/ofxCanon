#include "Handlers.h"
#include "Device.h"
#include "Utils.h"

namespace ofxCanon {
	//----------
	EdsError EDSCALLBACK Handlers::handleObjectEvent(EdsObjectEvent eventType, EdsBaseRef object, EdsVoid* context) {
		if (object) {
			auto device = static_cast<ofxCanon::Device*>(context);
			if (device) {
				switch (eventType) {
				case kEdsObjectEvent_DirItemCreated:
				case kEdsObjectEvent_DirItemRequestTransfer:
				{
					auto directoryItem = (EdsDirectoryItemRef)object;
					if (directoryItem) {
						device->performInCameraThread([device, directoryItem]() {
							device->download(directoryItem);
						});
					}
					break;
				}
				case kEdsObjectEvent_DirItemRemoved:
					break;
				default:
					EdsRelease(object);
					break;
				}
			}
		}

		return EDS_ERR_OK;
	}

	//----------
	EdsError EDSCALLBACK Handlers::handlePropertyEvent(EdsPropertyEvent eventType, EdsPropertyID propertyId, EdsUInt32 param, EdsVoid* context) {
		auto device = static_cast<ofxCanon::Device*>(context);
		
		switch (eventType) {
		case kEdsPropertyEvent_All:
			if (device->logDeviceCallbacks) {
				ofLogNotice("ofxCanon") << "Property event all " << propertyToString(propertyId) << " : " << param;
			}
			break;
		case kEdsPropertyEvent_PropertyChanged:
			if (device->logDeviceCallbacks) {
				ofLogNotice("ofxCanon") << "Property changed " << propertyToString(propertyId) << " : " << param;
			}
			
			device->performInCameraThread([=]() {
				device->pollProperty(propertyId);
			});
			break;
		case kEdsPropertyEvent_PropertyDescChanged:
			if (device->logDeviceCallbacks) {
				ofLogNotice("ofxCanon") << "Property description changed " << propertyToString(propertyId) << " : " << param;
			}

			device->performInCameraThread([=]() {
				auto propertyIdCopy = propertyId;
				ofNotifyEvent(device->onParameterOptionsChange, propertyIdCopy);
			});
			break;
		}
		return EDS_ERR_OK;
	}

	//----------
	EdsError EDSCALLBACK Handlers::handleStateEvent(EdsStateEvent eventType, EdsUInt32 param, EdsVoid* context) {
		auto device = static_cast<ofxCanon::Device*>(context);

		switch (eventType) {
		case kEdsStateEvent_WillSoonShutDown:
			{
				//Extend the shutdown timer if the camera is about to fall asleep
				device->performInCameraThread([=]() {
					EdsSendCommand(device->camera, kEdsCameraCommand_ExtendShutDownTimer, 0);
				});
				break;
			}
		case kEdsStateEvent_CaptureError:
			{
				//TODO : capture failure
				//fix this
				break;
			}
		}
		return EDS_ERR_OK;
	}
}
