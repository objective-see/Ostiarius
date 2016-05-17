//
//  Configure.m
//  Ostiarius
//
//  Created by Patrick Wardle on 1/2/16.
//  Copyright (c) 2016 Objective-See. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "Logging.h"
#import "Consts.h"
#import "Configure.h"
#import "Utilities.h"

@implementation Configure

//performs install || uninstall logic
-(BOOL)configure:(NSUInteger)parameter
{
    //return var
    BOOL bRet = NO;
    
    //error flag
    BOOL bAnyErrors = NO;
    
    //install
    // ->copy kext to system directory, then start it
    if(ACTION_INSTALL_FLAG == parameter)
    {
        //dbg msg
        logMsg(LOG_DEBUG, @"installing...");
        
        //always try stop kext
        [self stopKext];
        
        //always remove kext
        [self uninstallKext];
        
        //install kext
        if(YES != [self installKext])
        {
            //bail
            goto bail;
        }
        
        //dbg msg
        logMsg(LOG_DEBUG, @"installed kext");
        
        //start kext
        if(YES != [self startKext])
        {
            //bail
            goto bail;
        }
        
        //dbg msg
        logMsg(LOG_DEBUG, @"started kext");
    }
    //uninstall
    // ->stop & delete kext
    else if(ACTION_UNINSTALL_FLAG == parameter)
    {
        //dbg msg
        logMsg(LOG_DEBUG, @"uninstalling...");
        
        //stop kext
        if(YES != [self stopKext])
        {
            //err msg
            logMsg(LOG_ERR, @"failed to stop kext");
            
            //set error flag
            bAnyErrors = YES;
            
            //don't bail
            // ->still try remove kext
        }
        //debug logic only
        // ->display message
        else
        {
            //dbg msg
            logMsg(LOG_DEBUG, @"stopped kext");
        }
        
        //remove kext
        if(YES != [self uninstallKext])
        {
            //bail
            goto bail;
        }
        
        //dbg msg
        logMsg(LOG_DEBUG, @"uninstalled kext");
    }

    //no errors
    bRet = YES;
    
//bail
bail:
    
    //any errors?
    // ->set return to NO
    if(YES == bAnyErrors)
    {
        //unset
        bRet = NO;
    }
    
    return bRet;
}

//install kext
// ->copy kext (bundle) to /Library/Extensions and set permissions
-(BOOL)installKext
{
    //return/status var
    BOOL bRet = NO;
    
    //error
    NSError* error = nil;
    
    //move kext into /Libary/Extensions
    // ->orginally stored in applications /Resource bundle
    if(YES != [[NSFileManager defaultManager] copyItemAtPath:[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:KEXT_NAME] toPath:kextPath() error:&error])
    {
        //err msg
        logMsg(LOG_ERR, [NSString stringWithFormat:@"ERROR: failed to copy kext into /Library/Extensions (%@)", error]);
        
        //bail
        goto bail;
    }
    
    //dbg msg
    logMsg(LOG_DEBUG, [NSString stringWithFormat:@"copied kext to %@", kextPath()]);
    
    //always set group/owner to root/wheel
    setFileOwner(kextPath(), @0, @0, YES);
    
    //no errors
    bRet = YES;
    
//bail
bail:
    
    return bRet;
}

//start kext
// ->simply execs 'kextload' <kext path>
-(BOOL)startKext
{
    //return var
    BOOL bRet = NO;
    
    //status
    NSUInteger status = -1;
    
    //parameter array
    NSMutableArray* parameters = nil;
    
    //init pararm array
    parameters = [NSMutableArray array];
    
    //add kext path as first (and only) arg
    [parameters addObject:kextPath()];
    
    //dbg msg
    logMsg(LOG_DEBUG, [NSString stringWithFormat:@"starting kext with %@", parameters]);

    //load kext
    status = execTask(KEXT_LOAD, parameters);
    if(STATUS_SUCCESS != status)
    {
        //err msg
        logMsg(LOG_ERR, [NSString stringWithFormat:@"starting kext failed with %lu", (unsigned long)status]);
        
        //bail
        goto bail;
    }

    //happy
    bRet = YES;

//bail
bail:

    return bRet;
}

//unload and remove kext
-(BOOL)uninstallKext
{
    //return/status var
    BOOL bRet = NO;
    
    //error
    NSError* error = nil;
    
    //path to kext
    NSString* path = nil;
    
    //get kext's path
    path = kextPath();
    
    //dbg msg
    logMsg(LOG_DEBUG, [NSString stringWithFormat:@"uninstalling kext (%@)", path]);
    
    //delete kext
    if(YES != [[NSFileManager defaultManager] removeItemAtPath:path error:&error])
    {
        //err msg
        logMsg(LOG_ERR, [NSString stringWithFormat:@"failed to delete kext (%@)", error]);
        
        //bail
        goto bail;
    }
    
    //dbg msg
    logMsg(LOG_DEBUG, @"deleted kext");
    
    //no errors
    bRet = YES;
    
//bail
bail:
    
    return bRet;
}

//stop kext
// ->simply execs 'kextunload' -b <kext path>
-(BOOL)stopKext
{
    //return var
    BOOL bRet = NO;
    
    //status
    NSUInteger status = -1;
    
    //parameter array
    NSMutableArray* parameters = nil;
    
    //init pararm array
    parameters = [NSMutableArray array];
    
    //add -b as first arg
    [parameters addObject:@"-b"];
    
    //add kext bundle id/label as second arg
    [parameters addObject:KEXT_LABEL];
    
    //dbg msg
    logMsg(LOG_DEBUG, [NSString stringWithFormat:@"stopping kext with %@", parameters]);
    
    //unload kext
    status = execTask(KEXT_UNLOAD, parameters);
    if(STATUS_SUCCESS != status)
    {
        //err msg
        logMsg(LOG_ERR, [NSString stringWithFormat:@"stopping kext failed with %lu", (unsigned long)status]);
        
        //bail
        goto bail;
    }
    
    //happy
    bRet = YES;
    
//bail
bail:
    
    return bRet;
}


@end

