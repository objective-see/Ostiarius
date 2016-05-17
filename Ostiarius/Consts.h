//
//  Const.h
//  Ostiarius
//
//  Created by Patrick Wardle on 1/2/16.
//  Copyright (c) 2016 Objective-See. All rights reserved.
//

#ifndef __Ostiarius_Consts_h
#define __Ostiarius_Consts_h

//general error URL
#define FATAL_ERROR_URL @"https://objective-see.com/errors.html"

//kext bundle name
#define KEXT_NAME @"Ostiarius.kext"

//kext label
#define KEXT_LABEL @"com.objective-see.OstiariusKext"

//path to kextload
#define KEXT_LOAD @"/sbin/kextload"

//path to kextunload
#define KEXT_UNLOAD @"/sbin/kextunload"

//action to install
// ->also button title
#define ACTION_INSTALL @"Install"

//action to uninstall
// ->also button title
#define ACTION_UNINSTALL @"Uninstall"

//action to kick off UI installer
#define ACTION_UNINSTALL_UI @"Uninstall_UI"

//button title
// ->Close
#define ACTION_CLOSE @"Close"

//flag to uninstall
#define ACTION_UNINSTALL_FLAG 0

//flag to install
#define ACTION_INSTALL_FLAG 1

//frame shift
// ->for status msg to avoid activity indicator
#define FRAME_SHIFT 45

//status OK
#define STATUS_SUCCESS 0

//error msg
#define KEY_ERROR_MSG @"errorMsg"

//sub msg
#define KEY_ERROR_SUB_MSG @"errorSubMsg"

//error URL
#define KEY_ERROR_URL @"errorURL"

//flag for error popup
#define KEY_ERROR_SHOULD_EXIT @"shouldExit"

#endif
