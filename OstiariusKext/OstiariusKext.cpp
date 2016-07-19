//
//  OstiariusKext.cpp
//  Ostiarius
//
//  Created by Patrick Wardle on 1/2/16.
//  Copyright (c) 2016 Objective-See. All rights reserved.
//

#include "OstiariusKext.hpp"

//required macro
OSDefineMetaClassAndStructors(com_objectiveSee_OstiariusKext, IOService)

//kauth callback for KAUTH_SCOPE_FILEOP events
// ->kill any unsigned, non-approved binaries from the internet
static int processExec(kauth_cred_t credential, void* idata, kauth_action_t action, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)
{
    //mount point
    mount_t mount = {0};
    
    //pid
    int pid = -1;
    
    //path
    char path[MAXPATHLEN+1] = {0};
    
    //path length
    int pathLength = MAXPATHLEN;
    
    //quarantine flags
    unsigned int qFlags = 0;
    
    //dmg path
    char dmgPath[MAXPATHLEN+1] = {0};
    
    //dmg vnode
    vnode_t dmgVnode = NULLVP;
    
    //locked flag
    boolean_t wasLocked = FALSE;
    
    //offset pointer
    unsigned char* offsetPointer = NULL;
    
    //vfsstatfs struct
    struct vfsstatfs* vfsstat = NULL;
    
    //vfs context
    vfs_context_t vfsContext = NULL;
    
    //ignore all non exec events
    if(KAUTH_FILEOP_EXEC != action)
    {
        //bail
        goto bail;
    }
    
    //ignore any non 'regular' vnodes
    if(0 == vnode_isreg((vnode_t)arg0))
    {
        //bail
        goto bail;
    }
    
    //get pid
    pid = proc_selfpid();
    
    //zero out path
    bzero(&path, sizeof(path));
    
    //get path
    if(0 != vn_getpath((vnode_t)arg0, path, &pathLength))
    {
        //err msg
        DEBUG_PRINT(("OSTIARIUS ERROR: vn_getpath() failed\n"));
        
        //bail
        goto bail;
    }
    
    //dbg msg
    DEBUG_PRINT(("OSTIARIUS: new process: %s %d\n", path, pid));
    
    /* STEP 1:
       check if binary is from the internet, by checking its quarantine attributes */
    
    //get quarantine attribute flags
    qFlags = getQAttrFlags((vnode_t)arg0);
    
    //no q flags indicates binary not from the internet
    // ->unless, its from a DMG, then need to find/check DMG
    if(0 == qFlags)
    {
        //from a dmg?
        // ->nope, allow process
        if(0 != strncmp("/Volumes/", path, strlen("/Volumes/")))
        {
            //dbg msg
            DEBUG_PRINT(("OSTIARIUS: no quarantine attributes and not from .dmg, so allowing\n"));
            
            //bail
            goto bail;
        }
        
        //dbg msg
        DEBUG_PRINT(("OSTIARIUS: process appears to be from a dmg, so looking up/checking disk image\n"));
        
        //get mount struct for vnode's filesystem
        mount = vnode_mount((vnode_t)arg0);
        if(NULL == mount)
        {
            //bail
            goto bail;
        }
        
        //get vfsstatfs struct for mount
        vfsstat = vfs_statfs(mount);
        if(NULL == vfsstat)
        {
            //bail
            goto bail;
        }
        
        //zero out dmg path
        bzero(&dmgPath, sizeof(dmgPath));
        
        //find disk image that file was mounted from
        findDMG(vfsstat->f_mntfromname, (char*)&dmgPath);
        if(0 == strlen((const char*)dmgPath))
        {
            //dbg msg
            DEBUG_PRINT(("OSTIARIUS: no disk image found, so allowing\n"));
            
            //bail
            goto bail;
        }
        
        //dbg msg
        DEBUG_PRINT(("OSTIARIUS: found disk image path: %s\n", dmgPath));
        
        //create a vfs context
        vfsContext = vfs_context_create(NULL);
        if(NULL == vfsContext)
        {
            //bail
            goto bail;
        }
        
        //get vnode for disk image path
        if(0 != vnode_lookup(dmgPath, 0, &dmgVnode, vfsContext))
        {
            //err msg
            DEBUG_PRINT(("OSTIARIUS ERROR: failed find vnode for dmg path\n"));
            
            //bail
            goto bail;
        }
        
        //finally get q attr flags for DMG
        // ->if it doesn't have any, allow as its not from the internet
        qFlags = getQAttrFlags(dmgVnode);
        if(0 == qFlags)
        {
            //dbg msg
            DEBUG_PRINT(("OSTIARIUS: no quarantine attributes on dmg, so allowing\n"));
            
            //bail
            goto bail;
        }
        
    }// no qAttrz
    
    /* STEP 2:
       check if binary was previously allowed by checking quarantine attribute flags */
    
    //CoreServicesUIAgent (user-mode) sets flags to 0x40 when user clicks 'allow'
    // ->so just allow such binaries
    if(0 != (qFlags & 0x40))
    {
        //dbg msg
        DEBUG_PRINT(("OSTIARIUS: previously approved, so allowing\n"));
        
        //bail
        goto bail;
    }
    
    //dbg msg
    DEBUG_PRINT(("OSTIARIUS: binary is from the internet and has not been user-approved\n"));
    
    /* STEP 3:
       check if binary is signed (method inspired by Gatekeerper/csfg_get_platform_binary, tx @osxreverser!) */
    
    //lock vnode
    lck_mtx_lock((lck_mtx_t *)arg0);
    
    //set lock flag
    wasLocked = TRUE;
    
    //init offset pointer
    offsetPointer = (unsigned char*)(vnode_t)arg0;
    
    //get pointer to struct ubc_info in vnode struct
    // ->disasm from kernel:  mov rax, [vnode+70h]
    offsetPointer += 0x70;
    if(0 == *(unsigned long*)(offsetPointer))
    {
        //bail
        goto bail;
    }
    
    //dbg msg
    //DEBUG_PRINT(("OSTIARIUS: vnode 'vu_ubcinfo': %p, points to: %p\n", offsetPointer, (void*)*(unsigned long*)(offsetPointer)));
    
    //dref pointer
    // ->get addr of struct ubc_info
    offsetPointer = (unsigned char*)*(unsigned long*)(offsetPointer);
    
    //get pointer to cs_blob struct from struct ubc_info
    // ->disasm from kernel: mov rax, [ubc_info+50h]
    offsetPointer += 0x50;
    
    //dbg msg
    //DEBUG_PRINT(("OSTIARIUS: ubc_info 'cs_blob': %p\n", offsetPointer));
    
    //non-null csBlogs means process's binary is signed
    // ->note: yah, its a limitation that the binary could be signed, but invalidly
    if(0 != *(unsigned long*)(offsetPointer))
    {
        //dbg msg
        DEBUG_PRINT(("OSTIARIUS: %s is signed (non-NULL cs_blob), so allowing\n", path));
        
        //bail
        goto bail;
    }
    
    //dbg msg
    // ->always print
    IOLog("OSTIARIUS: %s is from the internet & is unsigned -> BLOCKING!\n", path);
    
    //kill the process
    // ->can't return 'KAUTH_RESULT_DENY', because its ignored (see 'Mac OS X Internals')
    proc_signal(pid, SIGKILL);

//bail
bail:
    
    //unlock vnode
    if(TRUE == wasLocked)
    {
        //unlock
        lck_mtx_unlock((lck_mtx_t *)arg0);
        
        //unset
        wasLocked = FALSE;
    }
    
    //release vfs context
    if(NULL != vfsContext)
    {
        //release
        vfs_context_rele(vfsContext);
        
        //unset
        vfsContext = NULL;
    }
    
    return KAUTH_RESULT_DEFER;
}

//given a vnode
// ->get it's quarantine attribute flags
unsigned int getQAttrFlags(vnode_t vnode)
{
    //quarantine flags
    unsigned int qFlags = 0;
    
    //q attrs buffer
    char* qAttr = NULL;
    
    //actual size of attrs
    size_t qAttrLength = 0;
    
    //alloc buffer
    qAttr = (char*)OSMalloc(QATTR_SIZE, allocTag);
    if(NULL == qAttr)
    {
        //bail
        goto bail;
    }
    
    //get quarantine attributes
    // ->if this 'fails', simply means binary doesn't have quarantine attributes
    if(0 != mac_vnop_getxattr(vnode, QFLAGS_STRING_ID, qAttr, QATTR_SIZE-1, &qAttrLength))
    {
        //dbg msg
        DEBUG_PRINT(("OSTIARIUS: mac_vnop_getxattr() didn't find any quarantine attributes\n"));
        
        //bail
        goto bail;
    }
    
    //null-terminate attributes
    qAttr[qAttrLength] = 0x0;
    
    //dbg msg
    DEBUG_PRINT(("OSTIARIUS: quarantine attributes: %s\n", qAttr));
    
    //grab flags
    // ->format will look something like: 0002;567c4986;Safari;8122B05F-C448-4FA4-B2CC-30A8E50BE65B
    if(1 != sscanf(qAttr, "%04x", &qFlags))
    {
        //err msg
        DEBUG_PRINT(("OSTIARIUS ERROR: sscanf('%s',...) failed\n", qAttr));
        
        //bail
        goto bail;
    }
    
    //dbg msg
    DEBUG_PRINT(("OSTIARIUS: value for %s -> %x...\n", QFLAGS_STRING_ID, qFlags));
    
//bail
bail:
    
    //free q attr buffer
    if(NULL != qAttr)
    {
        //free
        OSFree(qAttr, QATTR_SIZE, allocTag);
        
        //unset
        qAttr = NULL;
    }

    return qFlags;
}


//given a 'BSD name' for a mounted filesystem (ex: '/dev/disk1s2')
// ->find the orginal disk image (dmg) that was mounted at this location
void findDMG(char* mountFrom, char* diskImage)
{
    //matching dictionary for service lookup
    OSDictionary* matchingDictionary = NULL;
    
    //(top-level) iterator
    OSIterator* iterator = NULL;
    
    //registry entry for each IOHDIXHDDriveOutKernel entry
    IORegistryEntry* ioHDIXHD = NULL;
    
    //candidate disk image
    char* candidateDiskImage = NULL;
    
    //child
    IORegistryEntry *child = NULL;
    
    //image path data
    OSData* imagePathData = NULL;
    
    //image path length
    unsigned int imagePathLength = 0;
    
    //BSD name
    OSString* bsdName = NULL;
    
    //BSD name bytes
    const char* bsdNameBytes = NULL;
    
    //offset pointer
    char* offsetPointer = NULL;
    
    //get matching dictionary for 'IOHDIXHDDriveOutKernel'
    matchingDictionary = IOService::serviceMatching("IOHDIXHDDriveOutKernel");
    if(NULL == matchingDictionary)
    {
        //err msg
        DEBUG_PRINT(("OSTIARIUS ERROR: serviceMatching() failed\n"));
        
        //bail
        goto bail;
    }
    
    //get iterator
    iterator = IOService::getMatchingServices(matchingDictionary);
    if(NULL == iterator)
    {
        //err msg
        DEBUG_PRINT(("OSTIARIUS ERROR: getMatchingServices() failed\n"));
        
        //bail
        goto bail;
    }
    
    //iterate over all IOHDIXHDDriveOutKernel entries
    // ->grab 'image-path' then iterate down to get 'BSD Name'
    while(NULL != (ioHDIXHD = (IORegistryEntry*)iterator->getNextObject()))
    {
        //make sure candidate image is always freed
        // ->obv. this only comes into play during multiple iterations
        if(NULL != candidateDiskImage)
        {
            //free
            OSFree(candidateDiskImage, imagePathLength+1, allocTag);
            
            //unset
            candidateDiskImage = NULL;
        }
        
        //get 'image-path' property
        imagePathData = OSDynamicCast(OSData, ioHDIXHD->getProperty("image-path"));
        if(NULL == imagePathData)
        {
            //ignore
            continue;
        }
        
        //extract length
        imagePathLength = imagePathData->getLength();
        if(0 == imagePathLength)
        {
            //ignore
            continue;
        }
        
        //alloc buffer for NULL-terminated disk image path
        candidateDiskImage = (char*)OSMalloc(imagePathLength+1, allocTag);
        if(NULL == candidateDiskImage)
        {
            //ignore
            continue;
        }
        
        //copy in bytes
        memcpy(candidateDiskImage, imagePathData->getBytesNoCopy(), imagePathLength);
        
        //NULL terminate
        candidateDiskImage[imagePathLength] = 0x0;
        
        //dbg msg
        DEBUG_PRINT(("OSTIARIUS: image-path: %s\n", candidateDiskImage));
        
        //find 'IODiskImageBlockStorageDeviceOutKernel' child
        child = findChild(ioHDIXHD, "IODiskImageBlockStorageDeviceOutKernel");
        if(NULL == child)
        {
            //err msg
            DEBUG_PRINT(("OSTIARIUS ERROR: failed to find child 'IODiskImageBlockStorageDeviceOutKernel'\n"));
            
            //ignore
            continue;
        }
        
        //find 'IOBlockStorageDriver' child
        child = findChild(child, "IOBlockStorageDriver");
        if(NULL == child)
        {
            //err msg
            DEBUG_PRINT(("OSTIARIUS ERROR: failed to find child 'IOBlockStorageDriver'\n"));
            
            //ignore
            continue;
        }
        
        //find 'IOMedia' child
        child = findChild(child, "IOMedia");
        if(NULL == child)
        {
            //err msg
            DEBUG_PRINT(("OSTIARIUS ERROR: failed to find child 'IOMedia'\n"));
            
            //ignore
            continue;
        }
        
        //get 'BSD Name'
        bsdName = OSDynamicCast(OSString, child->getProperty("BSD Name"));
        if(NULL == bsdName)
        {
            //err msg
            DEBUG_PRINT(("OSTIARIUS ERROR: failed to get BSD NAME\n"));
            
            //ignore
            continue;
        }
        
        //get BSD name's bytes
        bsdNameBytes = bsdName->getCStringNoCopy();
        
        //dbg msg
        DEBUG_PRINT(("OSTIARIUS: BSD-name: %s\n", bsdNameBytes));
        
        //sanity check
        // mount from should start w/ /dev/
        if(0 != strncmp("/dev/", mountFrom, strlen("/dev/")))
        {
            //ignore
            continue;
        }
        
        //init offset pointer
        // ->skip over '/dev/'
        offsetPointer = mountFrom + strlen("/dev/");
        if(0 == strlen(offsetPointer))
        {
            //ignore
            continue;
        }
    
        //offset pointer should now point to something like, 'disk1s2'
        // ->compare w/ BSD name which should be something like 'disk1'
        if(0 != strncmp(bsdNameBytes, offsetPointer, strlen(bsdNameBytes)))
        {
            //dbg msg
            DEBUG_PRINT(("OSTIARIUS: %s != %s\n", mountFrom, bsdNameBytes));
            
            //ignore
            continue;
        }
        
        //dbg msg
        DEBUG_PRINT(("OSTIARIUS: %s == %s\n", mountFrom, bsdNameBytes));
        
        //save disk image's path into [out] parameter
        strncpy(diskImage, candidateDiskImage, MAXPATHLEN);
        
        //all set
        break;
    
    } //all 'IOHDIXHDDriveOutKernel' entries
    
//bail
bail:
    
    //release matching dictionary
    if(NULL != matchingDictionary)
    {
        //release
        matchingDictionary->release();
        
        //unset
        matchingDictionary = NULL;
        
    }
    
    //release iterator
    if(NULL != iterator)
    {
        //release
        iterator->release();
        
        //unset
        iterator = NULL;
    }
    
    //free candidate disk image buffer
    if(NULL != candidateDiskImage)
    {
        //free
        OSFree(candidateDiskImage, imagePathLength+1, allocTag);
        
        //unset
        candidateDiskImage = NULL;
    }

    return;
}

//given a parent
// ->finds (first) child that matches specified class name
IORegistryEntry* findChild(IORegistryEntry* parent, const char* name)
{
    //iterator
    OSIterator* iterator = NULL;
    
    //candidate child
    IORegistryEntry* candidateChild = NULL;
    
    //child's metaclass
    const OSMetaClass *metaClass = NULL;
    
    //(matched) child
    IORegistryEntry* child = NULL;
    
    //dbg msg
    DEBUG_PRINT(("OSTIARIUS: looking for child: %s\n", name));
    
    //init iterator
    iterator = parent->getChildIterator(gIOServicePlane);
    if(NULL == iterator)
    {
        //bail
        goto bail;
    }
    
    //iterate over all children
    while((candidateChild = (IORegistryEntry*)iterator->getNextObject()))
    {
        //get meta-class
        metaClass = candidateChild->getMetaClass();
        if(NULL == metaClass)
        {
            //ignore
            continue;
        }
        
        //dbg
        DEBUG_PRINT(("OSTIARIUS: child -> %s/%s\n", candidateChild->getName(gIOServicePlane), metaClass->getClassName()));
        
        //check for match
        if(0 == strncmp(metaClass->getClassName(), name, strlen(name)))
        {
            //found child
            child = candidateChild;
            
            //bail
            break;
        }
    
    } //all children
    
//bail
bail:
    
    //release iterator
    if(NULL != iterator)
    {
        //release
        iterator->release();
        
        //unset
        iterator = NULL;
    }

    return child;
}

//start function, automatically invoked
// ->check version, register KAuth listener & alloc tag
bool com_objectiveSee_OstiariusKext::start(IOService *provider)
{
    //result
    bool result = false;
    
    //invoke super
    if(true != super::start(provider))
    {
        //bail
        goto bail;
    }
    
    //dbg msg
    DEBUG_PRINT(("OSTIARIUS: starting (system version: %d,%d,%d)...\n", version_major, version_minor, version_revision));
    
    //version check
    // ->only El Capitan, up to 10.11.6 (15.6.0)
    if( (version_major != 15) ||
        (version_minor > 6) )
    {
        //err msg
        IOLog("OSTIARIUS ERROR: %d.%d.%d is an unsupported OS\n", version_major, version_minor, version_revision);
        
        //bail
        goto bail;
    }
    
    //register listener
    // ->scope 'KAUTH_SCOPE_FILEOP'
    kauthListener = kauth_listen_scope(KAUTH_SCOPE_FILEOP, &processExec, NULL);
    if(NULL == kauthListener)
    {
        //err msg
        DEBUG_PRINT(("OSTIARIUS ERROR: kauth_listen_scope('KAUTH_SCOPE_FILEOP',...) failed\n"));
        
        //bail
        goto bail;
    }
    
    //init alloc tag
    allocTag = OSMalloc_Tagalloc("Ostiarius Alloc Tag", OSMT_DEFAULT);
    if(NULL == allocTag)
    {
        //err msg
        DEBUG_PRINT(("OSTIARIUS ERROR: OSMalloc_Tagalloc() failed\n"));
        
        //bail
        goto bail;
    }
    
    //happy
    result = true;
    
//bail
bail:
    
    return result;
}

//stop function, automatically invoked
// ->unregister listener & free alloc tag
void com_objectiveSee_OstiariusKext::stop(IOService *provider)
{
    //dbg msg
    DEBUG_PRINT(("OSTIARIUS: stopping...\n"));
    
    //unregister listener
    if(NULL != kauthListener)
    {
        //unregister
        kauth_unlisten_scope(kauthListener);
        
        //unset
        kauthListener = NULL;
    }
    
    //free alloc tag
    if(NULL != allocTag)
    {
        //free
        OSMalloc_Tagfree(allocTag);
        
        //unset
        allocTag = NULL;
    }
    
    //invoke super
    super::stop(provider);
    
    return;
}