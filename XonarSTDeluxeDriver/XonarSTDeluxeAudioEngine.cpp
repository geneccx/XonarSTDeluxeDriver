//
//  XonarSTDeluxeAudioEngine.cpp
//  XonarSTDeluxeDriver
//
//  Created by Gene Chen on 2015-01-27.
//  Copyright (c) 2015 Gene Chen. All rights reserved.
//

#include "XonarSTDeluxeAudioEngine.h"

#include <IOKit/IOLib.h>

#include <IOKit/IOFilterInterruptEventSource.h>

#define INITIAL_SAMPLE_RATE	44100
#define NUM_SAMPLE_FRAMES	16384
#define NUM_CHANNELS		2
#define BIT_DEPTH			16

#define BUFFER_SIZE			(NUM_SAMPLE_FRAMES * NUM_CHANNELS * BIT_DEPTH / 8)

#define super IOAudioEngine

OSDefineMetaClassAndStructors(XonarSTDeluxeAudioEngine, IOAudioEngine)

bool XonarSTDeluxeAudioEngine::init(XonarSTDeluxeAudioDeviceRegisters *regs)
{
    bool result = false;
    
    IOLog("XonarSTDeluxeAudioEngine[%p]::init(%p)\n", this, regs);
    
    if (!regs) {
        goto Done;
    }
    
    if (!super::init(NULL)) {
        goto Done;
    }
    
    deviceRegisters = regs;
    
    result = true;
    
Done:
    
    return result;
}

bool XonarSTDeluxeAudioEngine::initHardware(IOService *provider)
{
    bool result = false;
    IOAudioSampleRate initialSampleRate;
    IOAudioStream *audioStream;
    IOWorkLoop *workLoop;
    
    IOLog("XonarSTDeluxeAudioEngine[%p]::initHardware(%p)\n", this, provider);
    
    if (!super::initHardware(provider)) {
        goto Done;
    }
    
    // Setup the initial sample rate for the audio engine
    initialSampleRate.whole = INITIAL_SAMPLE_RATE;
    initialSampleRate.fraction = 0;
    
    setDescription("Xonar Essence ST Deluxe Audio Engine");
    
    setSampleRate(&initialSampleRate);
    
    // Set the number of sample frames in each buffer
    setNumSampleFramesPerBuffer(NUM_SAMPLE_FRAMES);
    
    workLoop = getWorkLoop();
    if (!workLoop) {
        goto Done;
    }
    
    // Create an interrupt event source through which to receive interrupt callbacks
    // In this case, we only want to do work at primary interrupt time, so
    // we create an IOFilterInterruptEventSource which makes a filtering call
    // from the primary interrupt interrupt who's purpose is to determine if
    // our secondary interrupt handler is to be called.  In our case, we
    // can do the work in the filter routine and then return false to
    // indicate that we do not want our secondary handler called
    interruptEventSource = IOFilterInterruptEventSource::filterInterruptEventSource(this,
                                                                                    XonarSTDeluxeAudioEngine::interruptHandler,
                                                                                    XonarSTDeluxeAudioEngine::interruptFilter,
                                                                                    audioDevice->getProvider());
    if (!interruptEventSource) {
        goto Done;
    }
    
    // In order to allow the interrupts to be received, the interrupt event source must be
    // added to the IOWorkLoop
    // Additionally, interrupts will not be firing until the interrupt event source is
    // enabled by calling interruptEventSource->enable() - this probably doesn't need to
    // be done until performAudioEngineStart() is called, and can probably be disabled
    // when performAudioEngineStop() is called and the audio engine is no longer running
    // Although this really depends on the specific hardware
    workLoop->addEventSource(interruptEventSource);
    
    // Allocate our input and output buffers - a real driver will likely need to allocate its buffers
    // differently
    outputBuffer = (SInt16 *)IOMalloc(BUFFER_SIZE);
    if (!outputBuffer) {
        goto Done;
    }
    
    inputBuffer = (SInt16 *)IOMalloc(BUFFER_SIZE);
    if (!inputBuffer) {
        goto Done;
    }
    
    // Create an IOAudioStream for each buffer and add it to this audio engine
    audioStream = createNewAudioStream(kIOAudioStreamDirectionOutput, outputBuffer, BUFFER_SIZE);
    if (!audioStream) {
        goto Done;
    }
    
    addAudioStream(audioStream);
    audioStream->release();
    
    audioStream = createNewAudioStream(kIOAudioStreamDirectionInput, inputBuffer, BUFFER_SIZE);
    if (!audioStream) {
        goto Done;
    }
    
    addAudioStream(audioStream);
    audioStream->release();
    
    result = true;
    
Done:
    
    return result;
}

void XonarSTDeluxeAudioEngine::free()
{
    IOLog("XonarSTDeluxeAudioEngine[%p]::free()\n", this);
    
    // We need to free our resources when we're going away
    
    if (interruptEventSource) {
        interruptEventSource->release();
        interruptEventSource = NULL;
    }
    
    if (outputBuffer) {
        IOFree(outputBuffer, BUFFER_SIZE);
        outputBuffer = NULL;
    }
    
    if (inputBuffer) {
        IOFree(inputBuffer, BUFFER_SIZE);
        inputBuffer = NULL;
    }
    
    super::free();
}

IOAudioStream *XonarSTDeluxeAudioEngine::createNewAudioStream(IOAudioStreamDirection direction, void *sampleBuffer, UInt32 sampleBufferSize)
{
    IOAudioStream *audioStream;
    
    audioStream = new IOAudioStream;
    if (audioStream) {
        if (!audioStream->initWithAudioEngine(this, direction, 1)) {
            audioStream->release();
        } else {
            IOAudioSampleRate rate;
            IOAudioStreamFormat format = {
                2,												// num channels
                kIOAudioStreamSampleFormatLinearPCM,			// sample format
                kIOAudioStreamNumericRepresentationSignedInt,	// numeric format
                BIT_DEPTH,										// bit depth
                BIT_DEPTH,										// bit width
                kIOAudioStreamAlignmentHighByte,				// high byte aligned - unused because bit depth == bit width
                kIOAudioStreamByteOrderBigEndian,				// big endian
                true,											// format is mixable
                0												// driver-defined tag - unused by this driver
            };
            
            // As part of creating a new IOAudioStream, its sample buffer needs to be set
            // It will automatically create a mix buffer should it be needed
            audioStream->setSampleBuffer(sampleBuffer, sampleBufferSize);
            
            // This device only allows a single format and a choice of 2 different sample rates
            rate.fraction = 0;
            rate.whole = 44100;
            audioStream->addAvailableFormat(&format, &rate, &rate);
            rate.whole = 48000;
            audioStream->addAvailableFormat(&format, &rate, &rate);
            
            // Finally, the IOAudioStream's current format needs to be indicated
            audioStream->setFormat(&format);
        }
    }
    
    return audioStream;
}

void XonarSTDeluxeAudioEngine::stop(IOService *provider)
{
    IOLog("XonarSTDeluxeAudioEngine[%p]::stop(%p)\n", this, provider);
    
    // When our device is being stopped and torn down, we should go ahead and remove
    // the interrupt event source from the IOWorkLoop
    // Additionally, we'll go ahead and release the interrupt event source since it isn't
    // needed any more
    if (interruptEventSource) {
        IOWorkLoop *wl;
        
        wl = getWorkLoop();
        if (wl) {
            wl->removeEventSource(interruptEventSource);
        }
        
        interruptEventSource->release();
        interruptEventSource = NULL;
    }
    
    // Add code to shut down hardware (beyond what is needed to simply stop the audio engine)
    // There may be nothing needed here
    
    super::stop(provider);
}

IOReturn XonarSTDeluxeAudioEngine::performAudioEngineStart()
{
    IOLog("XonarSTDeluxeAudioEngine[%p]::performAudioEngineStart()\n", this);
    
    // The interruptEventSource needs to be enabled to allow interrupts to start firing
    assert(interruptEventSource);
    interruptEventSource->enable();
    
    // When performAudioEngineStart() gets called, the audio engine should be started from the beginning
    // of the sample buffer.  Because it is starting on the first sample, a new timestamp is needed
    // to indicate when that sample is being read from/written to.  The function takeTimeStamp()
    // is provided to do that automatically with the current time.
    // By default takeTimeStamp() will increment the current loop count in addition to taking the current
    // timestamp.  Since we are starting a new audio engine run, and not looping, we don't want the loop count
    // to be incremented.  To accomplish that, false is passed to takeTimeStamp().
    takeTimeStamp(false);
    
    // Add audio - I/O start code here
    
//#error performAudioEngineStart() - driver will not work until audio engine start code is added
    
    return kIOReturnSuccess;
}

IOReturn XonarSTDeluxeAudioEngine::performAudioEngineStop()
{
    IOLog("XonarSTDeluxeAudioEngine[%p]::performAudioEngineStop()\n", this);
    
    // Assuming we don't need interrupts after stopping the audio engine, we can disable them here
    assert(interruptEventSource);
    interruptEventSource->disable();
    
    // Add audio - I/O stop code here
    
//#error performAudioEngineStop() - driver will not work until audio engine stop code is added
    
    return kIOReturnSuccess;
}

UInt32 XonarSTDeluxeAudioEngine::getCurrentSampleFrame()
{
    IOLog("XonarSTDeluxeAudioEngine[%p]::getCurrentSampleFrame()\n", this);
    
    // In order for the erase process to run properly, this function must return the current location of
    // the audio engine - basically a sample counter
    // It doesn't need to be exact, but if it is inexact, it should err towards being before the current location
    // rather than after the current location.  The erase head will erase up to, but not including the sample
    // frame returned by this function.  If it is too large a value, sound data that hasn't been played will be
    // erased.
    
//#error getCurrentSampleFrame() - driver will not work until correct sample frame is returned
    
    // Change to return the real value
    return 0;
}

IOReturn XonarSTDeluxeAudioEngine::performFormatChange(IOAudioStream *audioStream, const IOAudioStreamFormat *newFormat, const IOAudioSampleRate *newSampleRate)
{
    IOLog("XonarSTDeluxeAudioEngine[%p]::peformFormatChange(%p, %p, %p)\n", this, audioStream, newFormat, newSampleRate);
    
    // Since we only allow one format, we only need to be concerned with sample rate changes
    // In this case, we only allow 2 sample rates - 44100 & 48000, so those are the only ones
    // that we check for
    if (newSampleRate) {
        switch (newSampleRate->whole) {
            case 44100:
                IOLog("/t-> 44.1kHz selected\n");
                
                // Add code to switch hardware to 44.1khz
                break;
            case 48000:
                IOLog("/t-> 48kHz selected\n");
                
                // Add code to switch hardware to 48kHz
                break;
            default:
                // This should not be possible since we only specified 44100 and 48000 as valid sample rates
                IOLog("/t Internal Error - unknown sample rate selected.\n");
                break;
        }
    }
    
    return kIOReturnSuccess;
}


void XonarSTDeluxeAudioEngine::interruptHandler(OSObject *owner, IOInterruptEventSource *source, int count)
{
    // Since our interrupt filter always returns false, this function will never be called
    // If the filter returned true, this function would be called on the IOWorkLoop
    return;
}

bool XonarSTDeluxeAudioEngine::interruptFilter(OSObject *owner, IOFilterInterruptEventSource *source)
{
    XonarSTDeluxeAudioEngine *audioEngine = OSDynamicCast(XonarSTDeluxeAudioEngine, owner);
    
    // We've cast the audio engine from the owner which we passed in when we created the interrupt
    // event source
    if (audioEngine) {
        // Then, filterInterrupt() is called on the specified audio engine
        audioEngine->filterInterrupt(source->getIntIndex());
    }
    
    return false;
}

void XonarSTDeluxeAudioEngine::filterInterrupt(int index)
{
    // In the case of our simple device, we only get interrupts when the audio engine loops to the
    // beginning of the buffer.  When that happens, we need to take a timestamp and increment
    // the loop count.  The function takeTimeStamp() does both of those for us.  Additionally,
    // if a different timestamp is to be used (other than the current time), it can be passed
    // in to takeTimeStamp()
    takeTimeStamp();
}