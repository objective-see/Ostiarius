//
//  Logging.h
//  Ostiarius
//
//  Created by Patrick Wardle on 1/2/16.
//  Copyright (c) 2016 Objective-See. All rights reserved.
//

#ifndef __Ostiarius__Logging__
#define __Ostiarius__Logging__

#import <syslog.h>

//log a msg to syslog
// ->also disk, if error
void logMsg(int level, NSString* msg);

#endif
