# ofxCanon

An openFrameworks addon for interfacing with Canon DSLR cameras.

![exampleLiveView](https://github.com/elliotwoods/ofxCanon/blob/master/screenshot.PNG?raw=true)

## Features

* Photo taking
  * Blocking or Async (using C++11 `std::future`)
  * 8bit and 16bit
* Live view capture
* ISO / Aperture / Shutter speed settings (+ ofParameter support)
* Lens information (+ events when lens is changed)
* Stricter threading model
    * Ability to call functions in different threads 'remotely' without instruction queues.
    * `std::future` for async photo capture
    * Use it directly with your own threads, or use the `Simple` class to manage threads for you
* Backwards compatability with `ofxEdsdk::Camera`

## Requirements

* openFrameworks 0.9.0 or later
* You must download the EDSDK from Canon ([Canon Europe EDSDK download](http://www.didp.canon-europa.com), [Canon USA EDSDK download](http://consumer.usa.canon.com/cusa/support/consumer/eos_slr_camera_systems/eos_digital_slr_cameras/digital_rebel_xt?fileURL=ps_sdk_form&pageKeyCode=downloadLicense&id=0901e02480057a74_1&productOverviewCid=0901e0248003ce28&keycode=Sdk_Lic)).
* (Optional : [ofxAddonLib](http://github.com/elliotwoods/ofxAddonLib) if you're using Visual Studio)

## Tested

This addon is tested with:

### 2016.11.04

Commit #[6b1b18b](https://github.com/elliotwoods/ofxCanon/commit/6b1b18b3edada5cfd52796f748429901da5eeb4d)

| IDE / Platform | EDSDK v. | Camera  | Machine  | Success | 
|----------------|----------|---------|----------|---------|
| VS2015         | 3.5.0    | EOS100D | x86, x64 | YES     |
| VS2015         | 3.5.0    | EOS550D | x86, x64 | YES     |
| XCode          | 3.5.0    | EOS100D | x86, x64 | YES     |
| XCode          | 3.5.0    | EOS550D | x86, x64 | YES     |
| XCode          | 3.9.0    | EOS1DX  | x86, x64 | [YES](https://github.com/elliotwoods/ofxCanon/issues/8#issuecomment-706388906)     |

# Usage

1. Download the necessary libs from Canon (check the `libs/context_??.txt` file for your platform to see what files you need where)

## XCode

1. Use the Project Generator to add ofxCanon to a new or existing project
2. Set your macOS Deployment target to be `10.8` or later

Optional : For 32-bit builds you can add the `DPP.framework` to your project if you want to use the EDSDK (rather than FreeImage) to develop RAW images and recover metadata from images. Note : if you don't definitely know you that need this then don't worry about it.

## Visual Studio

Check [instructions from ofxAddonLib](https://github.com/elliotwoods/ofxAddonLib#how-to-use-an-addon-which-uses-ofxaddonlib-pattern).

# ofxEdsdk compatability 

This addon began as a rewrite of the fantastic [ofxEdsdk](https://github.com/kylemcdonald/ofxEdsdk) by Kyle McDonald + adding some features (although some may be missing).

Compatability with ofxEdsdk is provided by the `ofxCanon::Simple` class which has the same interface as `ofxEdsdk::Camera` (i.e. all the method names are the same and it should act as a drop-in replacement if you need it).

Some users of ofxEdsdk (myself included) have experienced issues with ofxEdsdk, and this addon is intended as a way to circumvent those issues. However, if ofxEdsdk is working well for you, then great! I'd suggest to stick with it and come back if you encounter an issue :).
