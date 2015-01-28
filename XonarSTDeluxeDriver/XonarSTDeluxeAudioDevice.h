//
//  XonarSTDeluxeAudioDevice.h
//  XonarSTDeluxeDriver
//
//  Created by Gene Chen on 2015-01-27.
//  Copyright (c) 2015 Gene Chen. All rights reserved.
//

#ifndef _XONARSTDELUXEAUDIODEVICE_H
#define _XONARSTDELUXEAUDIODEVICE_H

#include <IOKit/audio/IOAudioDevice.h>

typedef struct XonarSTDeluxeAudioDeviceRegisters {
    UInt32	reg1;
    UInt32	reg2;
    UInt32	reg3;
    UInt32	reg4;
} XonarSTDeluxeAudioDeviceRegisters;

class IOPCIDevice;
class IOMemoryMap;

#define XonarSTDeluxeAudioDevice com_GeneC_driver_XonarSTDeluxeDevice

class XonarSTDeluxeAudioDevice : public IOAudioDevice
{
    friend class XonarSTDeluxeAudioEngine;
    
    OSDeclareDefaultStructors(XonarSTDeluxeAudioDevice)
    
    IOPCIDevice					*pciDevice;
    IOMemoryMap					*deviceMap;
    
    XonarSTDeluxeAudioDeviceRegisters	*deviceRegisters;
    
    virtual bool initHardware(IOService *provider);
    virtual bool createAudioEngine();
    virtual void free();
    
    static IOReturn volumeChangeHandler(IOService *target, IOAudioControl *volumeControl, SInt32 oldValue, SInt32 newValue);
    virtual IOReturn volumeChanged(IOAudioControl *volumeControl, SInt32 oldValue, SInt32 newValue);
    
    static IOReturn outputMuteChangeHandler(IOService *target, IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue);
    virtual IOReturn outputMuteChanged(IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue);
    
    static IOReturn gainChangeHandler(IOService *target, IOAudioControl *gainControl, SInt32 oldValue, SInt32 newValue);
    virtual IOReturn gainChanged(IOAudioControl *gainControl, SInt32 oldValue, SInt32 newValue);
    
    static IOReturn inputMuteChangeHandler(IOService *target, IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue);
    virtual IOReturn inputMuteChanged(IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue);
};

#endif /* _XONARSTDELUXEAUDIODEVICE_H */