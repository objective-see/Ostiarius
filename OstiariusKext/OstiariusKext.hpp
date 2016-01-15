//
//  OstiariusKext.hpp
//  Ostiarius
//
//  Created by Patrick Wardle on 1/2/16.
//  Copyright (c) 2016 Objective-See. All rights reserved.
//

#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>

extern "C"
{
    
#include <string.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <libkern/libkern.h>
#include <libkern/version.h>
#include <libkern/OSMalloc.h>
#include <security/mac_policy.h>
    
}

/* DEFINES */

//kext's superclass
#define super IOService

/* GLOBALS */

//kauth listener
// ->scope KAUTH_SCOPE_FILEOP
kauth_listener_t kauthListener = NULL;

//alloc tag
OSMallocTag allocTag = NULL;

//buffer size for quarantine attributes
// ->value extracted from '_quarantine_get_flags' disassembly in Quarantine.kext
#define QATTR_SIZE 0x1001

//quarantine flag identifier
#define QFLAGS_STRING_ID "com.apple.quarantine"

//print macros
#ifdef DEBUG
# define DEBUG_PRINT(x) IOLog x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

/* METHODS */

//given a vnode
// ->get it's quarantine attribute flags
unsigned int getQAttrFlags(vnode_t vnode);

//given a 'BSD name' for a mounted filesystem (ex: '/dev/disk1s2')
// ->find the orginal disk image (dmg) that was mounted at this location
void findDMG(char* mountFrom, char* diskImage);

//given a parent
// ->finds (first) child that matches specified class name
IORegistryEntry* findChild(IORegistryEntry* parent, const char* name);

//class declaration
class com_objectiveSee_OstiariusKext : public IOService
{
    //structs
    OSDeclareDefaultStructors(com_objectiveSee_OstiariusKext)
    
public:
    
    //start
    virtual bool start(IOService *provider) override;
    
    //stop
    virtual void stop(IOService *provider) override;
};


