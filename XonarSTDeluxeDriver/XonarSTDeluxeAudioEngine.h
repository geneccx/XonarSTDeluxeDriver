//
//  XonarSTDeluxeAudioEngine.h
//  XonarSTDeluxeDriver
//
//  Created by Gene Chen on 2015-01-27.
//  Copyright (c) 2015 Gene Chen. All rights reserved.
//

#ifndef _XONARSTDELUXEAUDIOENGINE_H
#define _XONARSTDELUXEAUDIOENGINE_H

#include <IOKit/audio/IOAudioEngine.h>

#include "XonarSTDeluxeAudioDevice.h"

#define XonarSTDeluxeAudioEngine com_GeneC_driver_XonarSTDeluxeEngine

class IOFilterInterruptEventSource;
class IOInterruptEventSource;

struct xonar_info;

class XonarSTDeluxeAudioEngine : public IOAudioEngine
{
    OSDeclareDefaultStructors(XonarSTDeluxeAudioEngine)
    
    xonar_info                      *deviceInfo;
    
    SInt16							*outputBuffer;
    SInt16							*inputBuffer;
    
    IOFilterInterruptEventSource	*interruptEventSource;
    
public:
    
    virtual bool init(xonar_info *deviceInfo);
    virtual void free();
    
    virtual bool initHardware(IOService *provider);
    virtual void stop(IOService *provider);
    
    virtual IOAudioStream *createNewAudioStream(IOAudioStreamDirection direction, void *sampleBuffer, UInt32 sampleBufferSize);
    
    virtual IOReturn performAudioEngineStart();
    virtual IOReturn performAudioEngineStop();
    
    virtual UInt32 getCurrentSampleFrame();
    
    virtual IOReturn performFormatChange(IOAudioStream *audioStream, const IOAudioStreamFormat *newFormat, const IOAudioSampleRate *newSampleRate);

    static void interruptHandler(OSObject *owner, IOInterruptEventSource *source, int count);
    static bool interruptFilter(OSObject *owner, IOFilterInterruptEventSource *source);
    virtual void filterInterrupt(int index);
};

#endif /* _XONARSTDELUXEAUDIOENGINE_H */
