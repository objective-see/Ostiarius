//
//  Configure.h
//  Ostiarius
//
//  Created by Patrick Wardle on 1/2/16.
//  Copyright (c) 2016 Objective-See. All rights reserved.
//

#ifndef __Ostiarius_Configure_h
#define __Ostiarius_Configure_h

@interface Configure : NSObject
{
    
}


/* METHODS */

//performs install || uninstall logic
-(BOOL)configure:(NSUInteger)parameter;

//install kext
// ->copy kext (bundle) to /Library/Extensions and set permissions
-(BOOL)installKext;

//start kext
-(BOOL)startKext;

//stop kext
-(BOOL)stopKext;

//unload and remove kext
-(BOOL)uninstallKext;

@end

#endif
