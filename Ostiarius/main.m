//
//  main.m
//  Ostiarius
//
//  Created by Patrick Wardle on 1/2/16.
//  Copyright (c) 2016 Objective-See. All rights reserved.
//

#import <syslog.h>
#import <Cocoa/Cocoa.h>

/* METHODS DECLARATIONS */

//spawn self as root
BOOL spawnAsRoot(char* path2Self);

/*CODE */

//main
int main(int argc, char *argv[])
{
    //return var
    int retVar = -1;
    
    //check for r00t
    // ->then spawn self via auth exec
    if(0 != geteuid())
    {
        //err msg
        syslog(LOG_ERR, "OSTIARIUS: non-root instance (%d)\n", getpid());
        
        //spawn as root
        if(YES != spawnAsRoot(argv[0]))
        {
            //err msg
            syslog(LOG_ERR, "OSTIARIUS ERROR: failed to spawn self as r00t\n");
            
            //bail
            goto bail;
        }
        
        //happy
        retVar = 0;
    }
    
    //otherwise
    // ->just kick off app, as we're root now
    else
    {
        //app away
        retVar = NSApplicationMain(argc, (const char **)argv);
    }
    
//bail
bail:
    
    return retVar;
}


//spawn self as root
BOOL spawnAsRoot(char* path2Self)
{
    //return/status var
    BOOL bRet = NO;
    
    //authorization ref
    AuthorizationRef authorizatioRef = {0};
    
    //args
    const char* args[0x10] = {0};
    
    //1st arg: pid of self
    // ->brute-forcing fails on this pid, so need to pass & record it
    args[0] = [[[NSNumber numberWithInt:getpid()] description] UTF8String];
    
    //end args with NULL
    args[1] = NULL;
    
    //flag indicating auth ref was created
    BOOL authRefCreated = NO;
    
    //status code
    OSStatus osStatus = -1;
    
    //create authorization ref
    // ->and check
    osStatus = AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment, kAuthorizationFlagDefaults, &authorizatioRef);
    if(errAuthorizationSuccess != osStatus)
    {
        //err msg
        syslog(LOG_ERR, "OSTIARIUS ERROR: AuthorizationCreate() failed with %d", osStatus);
        
        //bail
        goto bail;
    }
    
    //set flag indicating auth ref was created
    authRefCreated = YES;
    
    //spawn self as r00t w/ install flag (will ask user for password)
    // ->and check
    osStatus = AuthorizationExecuteWithPrivileges(authorizatioRef, path2Self, 0, (char* const*)&args, NULL);
    
    //check
    if(errAuthorizationSuccess != osStatus)
    {
        //err msg
        syslog(LOG_ERR, "OSTIARIUS ERROR: AuthorizationExecuteWithPrivileges() failed with %d", osStatus);
        
        //bail
        goto bail;
    }
    
    //no errors
    bRet = YES;
    
//bail
bail:
    
    //free auth ref
    if(YES == authRefCreated)
    {
        //free
        AuthorizationFree(authorizatioRef, kAuthorizationFlagDefaults);
    }
    
    return bRet;
}
