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

bool XonarSTDeluxeAudioEngine::init(xonar_info *deviceInfo)
{
    bool result = false;
    
    IOLog("XonarSTDeluxeAudioEngine[%p]::init(%p)\n", this, deviceInfo);
    
    if (!deviceInfo) {
        goto Done;
    }
    
    if (!super::init(NULL)) {
        goto Done;
    }
    
    this->deviceInfo = deviceInfo;
    
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
    IOByteCount length;
    
    IOLog("XonarSTDeluxeAudioEngine[%p]::initHardware(%p)\n", this, provider);
    
    if (!super::initHardware(provider)) {
        goto Done;
    }
    
    // Setup the initial sample rate for the audio engine
    initialSampleRate.whole = INITIAL_SAMPLE_RATE;
    initialSampleRate.fraction = 0;
    
    setDescription("Xonar Essence ST Deluxe");
    
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
    
    outputBuffer = IOBufferMemoryDescriptor::withOptions(kIOMemoryPhysicallyContiguous, DEFAULT_BUFFER_BYTES_MULTICH, PAGE_SIZE);
    if (!outputBuffer || outputBuffer->prepare() != kIOReturnSuccess) {
        goto Done;
    }
    
    inputBuffer = IOBufferMemoryDescriptor::withOptions(kIOMemoryPhysicallyContiguous, DEFAULT_BUFFER_BYTES_MULTICH, PAGE_SIZE);
    if (!inputBuffer || inputBuffer->prepare() != kIOReturnSuccess) {
        goto Done;
    }
    
    bzero(outputBuffer->getBytesNoCopy(), outputBuffer->getCapacity());
    bzero(inputBuffer->getBytesNoCopy(), inputBuffer->getCapacity());
    
    physicalAddressOutput = outputBuffer->getPhysicalSegment(0, &length);
    physicalAddressInput = inputBuffer->getPhysicalSegment(0, &length);
    
    // Create an IOAudioStream for each buffer and add it to this audio engine
    audioStream = createNewAudioStream(kIOAudioStreamDirectionOutput, outputBuffer);
    if (!audioStream) {
        goto Done;
    }
    
    addAudioStream(audioStream);
    audioStream->release();
    
    audioStream = createNewAudioStream(kIOAudioStreamDirectionInput, inputBuffer);
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
        outputBuffer->complete();
        outputBuffer->release();
        outputBuffer = NULL;
    }
    
    if (inputBuffer) {
        inputBuffer->complete();
        inputBuffer->release();
        inputBuffer = NULL;
    }
    
    super::free();
}

IOAudioStream *XonarSTDeluxeAudioEngine::createNewAudioStream(IOAudioStreamDirection direction, IOBufferMemoryDescriptor *sampleBuffer)
{
    IOAudioStream *audioStream;
    
    audioStream = new IOAudioStream;
    if (audioStream) {
        if (!audioStream->initWithAudioEngine(this, direction, 1)) {
            audioStream->release();
        } else {
            if(direction == kIOAudioStreamDirectionOutput){
                IOAudioSampleRate rate;
                IOAudioStreamFormat format = {
                    2,												// num channels
                    kIOAudioStreamSampleFormatLinearPCM,			// sample format
                    kIOAudioStreamNumericRepresentationSignedInt,	// numeric format
                    BIT_DEPTH,										// bit depth
                    BIT_DEPTH,										// bit width
                    kIOAudioStreamAlignmentLowByte,				    // low byte aligned - unused because bit depth == bit width
                    kIOAudioStreamByteOrderLittleEndian,			// little endian
                    true,											// format is mixable
                    0												// driver-defined tag - unused by this driver
                };
                
                // As part of creating a new IOAudioStream, its sample buffer needs to be set
                // It will automatically create a mix buffer should it be needed
                audioStream->setSampleBuffer(sampleBuffer->getBytesNoCopy(), sampleBuffer->getCapacity());
                
                rate.fraction = 0;
                rate.whole = 44100;
                audioStream->addAvailableFormat(&format, &rate, &rate);
                rate.whole = 48000;
                audioStream->addAvailableFormat(&format, &rate, &rate);
                rate.whole = 96000;
                audioStream->addAvailableFormat(&format, &rate, &rate);
                rate.whole = 192000;
                audioStream->addAvailableFormat(&format, &rate, &rate);
                
                // Finally, the IOAudioStream's current format needs to be indicated
                audioStream->setFormat(&format);
            } else {
                IOAudioSampleRate rate;
                IOAudioStreamFormat format = {
                    2,												// num channels
                    kIOAudioStreamSampleFormatLinearPCM,			// sample format
                    kIOAudioStreamNumericRepresentationSignedInt,	// numeric format
                    BIT_DEPTH,										// bit depth
                    BIT_DEPTH,										// bit width
                    kIOAudioStreamAlignmentLowByte,				    // low byte aligned - unused because bit depth == bit width
                    kIOAudioStreamByteOrderLittleEndian,			// little endian
                    true,											// format is mixable
                    0												// driver-defined tag - unused by this driver
                };
                
                // As part of creating a new IOAudioStream, its sample buffer needs to be set
                // It will automatically create a mix buffer should it be needed
                audioStream->setSampleBuffer(sampleBuffer->getBytesNoCopy(), sampleBuffer->getCapacity());
                
                rate.fraction = 0;
                rate.whole = 44100;
                audioStream->addAvailableFormat(&format, &rate, &rate);
                rate.whole = 48000;
                audioStream->addAvailableFormat(&format, &rate, &rate);
                rate.whole = 96000;
                audioStream->addAvailableFormat(&format, &rate, &rate);
                rate.whole = 192000;
                audioStream->addAvailableFormat(&format, &rate, &rate);
                
                // Finally, the IOAudioStream's current format needs to be indicated
                audioStream->setFormat(&format);
            }
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

    ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_write_4(MULTICH_ADDR, physicalAddressOutput);
    ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_write_4(MULTICH_SIZE, BUFFER_SIZE / 4 - 1);
    /* what is this 1024 you ask
     * i have no idea
     * oss uses dmap->fragment_size
     * alsa uses params_period_bytes()
     */
    ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_write_4(MULTICH_FRAG, 65536 / 4 - 1);
    
    ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_write_1(MULTICH_MODE,
                    (((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_read_1(MULTICH_MODE) &
                     ~MULTICH_MODE_CH_MASK) | MULTICH_MODE_2CH);
    
    /* setup i2s bits in the i2s register */
    ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_write_1(I2S_MULTICH_FORMAT,
                    (((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_read_1(I2S_MULTICH_FORMAT) &
                     ~I2S_BITS_MASK) | I2S_FMT_BITS16);
    
    /* enable irq */
    ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_write_2(IRQ_MASK,
                    ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_read_2(IRQ_MASK) | CHANNEL_MULTICH);
    /* enable dma */
    ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_write_2(DMA_START,
                    ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_read_2(DMA_START) | CHANNEL_MULTICH);
    
    return kIOReturnSuccess;
}

IOReturn XonarSTDeluxeAudioEngine::performAudioEngineStop()
{
    IOLog("XonarSTDeluxeAudioEngine[%p]::performAudioEngineStop()\n", this);
    
    // Assuming we don't need interrupts after stopping the audio engine, we can disable them here
    assert(interruptEventSource);
    interruptEventSource->disable();
    
    /* disable irq */
    ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_write_2(IRQ_MASK,
                                                              ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_read_2(IRQ_MASK) & ~CHANNEL_MULTICH);
    /* disable dma */
    ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_write_2(DMA_START,
                                                              ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_read_2(DMA_START) & ~CHANNEL_MULTICH);
    return kIOReturnSuccess;
}

UInt32 XonarSTDeluxeAudioEngine::getCurrentSampleFrame()
{
    //IOLog("XonarSTDeluxeAudioEngine[%p]::getCurrentSampleFrame()\n", this);
    
    // In order for the erase process to run properly, this function must return the current location of
    // the audio engine - basically a sample counter
    // It doesn't need to be exact, but if it is inexact, it should err towards being before the current location
    // rather than after the current location.  The erase head will erase up to, but not including the sample
    // frame returned by this function.  If it is too large a value, sound data that hasn't been played will be
    // erased.
    
    UInt32 ptr = ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_read_4(MULTICH_ADDR);
    ptr -= physicalAddressOutput;
    //ptr %=
    
    return ptr / (2 * 4);
}

IOReturn XonarSTDeluxeAudioEngine::performFormatChange(IOAudioStream *audioStream, const IOAudioStreamFormat *newFormat, const IOAudioSampleRate *newSampleRate)
{
    IOLog("XonarSTDeluxeAudioEngine[%p]::peformFormatChange(%p, %p, %p)\n", this, audioStream, newFormat, newSampleRate);
    
    if (newSampleRate) {
        switch (newSampleRate->whole) {
            case 44100:
                IOLog("/t-> 44.1kHz selected\n");
                
                ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_write_1(I2S_MULTICH_FORMAT,
                                (((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_read_1(I2S_MULTICH_FORMAT) &
                                 ~I2S_FMT_RATE_MASK) | I2S_FMT_RATE44);

                break;
            case 48000:
                IOLog("/t-> 48kHz selected\n");
                
                ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_write_1(I2S_MULTICH_FORMAT,
                                (((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_read_1(I2S_MULTICH_FORMAT) &
                                 ~I2S_FMT_RATE_MASK) | I2S_FMT_RATE48);
                
                break;
            default:
                // This should not be possible since we only specified 44100 and 48000 as valid sample rates
                IOLog("/t Internal Error - unknown sample rate selected.\n");
                break;
        }
    }
    
    if(newFormat) {
        switch(newFormat->fBitDepth) {
            case 16:
                ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_write_1(PLAY_FORMAT,
                                                                          (((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_read_1(PLAY_FORMAT) &
                                                                           ~MULTICH_FORMAT_MASK) | 0);
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
    unsigned int intstat;
    
    if ((intstat = ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_read_2(IRQ_STAT)) == 0) {
        return;
    } if ((intstat & CHANNEL_MULTICH)) {
        /* Acknowledge the interrupt by disabling and enabling the irq */
        ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_write_2(IRQ_MASK,
                        ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_read_2(IRQ_MASK) & ~CHANNEL_MULTICH);
        ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_write_2(IRQ_MASK,
                        ((XonarSTDeluxeAudioDevice*)audioDevice)->cmi8788_read_2(IRQ_MASK) | CHANNEL_MULTICH);
        
        takeTimeStamp();
    }

}