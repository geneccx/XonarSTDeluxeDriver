//
//  XonarSTDeluxeAudioDevice.cpp
//  XonarSTDeluxeDriver
//
//  Created by Gene Chen on 2015-01-27.
//  Copyright (c) 2015 Gene Chen. All rights reserved.
//

#include "AudioDevice.h"
#include "AudioEngine.h"

#include "Xonar.h"

#include <IOKit/audio/IOAudioControl.h>
#include <IOKit/audio/IOAudioLevelControl.h>
#include <IOKit/audio/IOAudioToggleControl.h>
#include <IOKit/audio/IOAudioSelectorControl.h>
#include <IOKit/audio/IOAudioDefines.h>

#include <IOKit/IOLib.h>

#include <IOKit/pci/IOPCIDevice.h>

#define super IOAudioDevice

OSDefineMetaClassAndStructors(XonarSTDeluxeAudioDevice, IOAudioDevice)

bool XonarSTDeluxeAudioDevice::initHardware(IOService *provider)
{
    bool result = false;
    UInt16 sVal = 0;
    UInt16 sDac = 0;
    UInt8 bVal = 0;
    int count = 0;
    
    IOLog("XonarSTDeluxeAudioDevice[%p]::initHardware(%p)\n", this, provider);
    
    if (!super::initHardware(provider)) {
        goto Done;
    }
    
    // Get the PCI device provider
    deviceInfo.pciDevice = OSDynamicCast(IOPCIDevice, provider);
    if (!deviceInfo.pciDevice) {
        goto Done;
    }
    
    // Config a map for the PCI config base registers
    // We need to keep this map around until we're done accessing the registers
    deviceInfo.deviceMap = deviceInfo.pciDevice->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if (!deviceInfo.deviceMap) {
        goto Done;
    }
    
    // Get the virtual address for the registers - mapped in the kernel address space
    /*deviceRegisters = (XonarSTDeluxeAudioDeviceRegisters *)deviceMap->getVirtualAddress();
    if (!deviceRegisters) {
        goto Done;
    }*/
    
    deviceInfo.pciDevice->setIOEnable(true);
    deviceInfo.pciDevice->setBusMasterEnable(true);
    
    // Enable the PCI memory access - the kernel will panic if this isn't done before accessing the
    // mapped registers
    deviceInfo.pciDevice->setMemoryEnable(true);
    
    /* Init CMI controller */
    sVal = cmi8788_read_2(&deviceInfo, CTRL_VERSION);
    if (!(sVal & CTRL_VERSION2)) {
        bVal = cmi8788_read_1(&deviceInfo, MISC_REG);
        bVal |= MISC_PCI_MEM_W_1_CLOCK;
        cmi8788_write_1(&deviceInfo, MISC_REG, bVal);
    }
    bVal = cmi8788_read_1(&deviceInfo, FUNCTION);
    bVal |= FUNCTION_RESET_CODEC;
    cmi8788_write_1(&deviceInfo, FUNCTION, bVal);
    
    /* set up DAC related settings */
    sDac = I2S_MASTER | I2S_FMT_RATE44 | I2S_FMT_LJUST | I2S_FMT_BITS16 | XONAR_MCLOCK_256;
    
    cmi8788_write_2(&deviceInfo, I2S_MULTICH_FORMAT, sDac);
    cmi8788_write_2(&deviceInfo, I2S_ADC1_FORMAT, sDac);
    cmi8788_write_2(&deviceInfo, I2S_ADC2_FORMAT, sDac);
    cmi8788_write_2(&deviceInfo, I2S_ADC3_FORMAT, sDac);
    
    /* setup routing regs with default values */
    cmi8788_write_2(&deviceInfo, PLAY_ROUTING, 0xE400);
    cmi8788_write_1(&deviceInfo, REC_ROUTING, 0x00);
    cmi8788_write_1(&deviceInfo, REC_MONITOR, 0x00);
    cmi8788_write_1(&deviceInfo, MONITOR_ROUTING, 0xE4);
    
    /* AC97 dances. Who needs it anyway? */
    /* Cold reset onboard AC97 */
    cmi8788_write_2(&deviceInfo, AC97_CTRL, AC97_COLD_RESET);
    count = 100;
    while ((cmi8788_read_2(&deviceInfo, AC97_CTRL) & AC97_STATUS_SUSPEND) && (count--))
    {
        cmi8788_write_2(&deviceInfo, AC97_CTRL,
                        (cmi8788_read_2(&deviceInfo, AC97_CTRL)
                         & ~AC97_STATUS_SUSPEND) | AC97_RESUME);
        
        IODelay(100);
    }
    
    if (!count)
        IOLog("XonarSTDeluxeAudioDevice[%p]::initHardware(%p) AC97 not ready\n", this, provider);
    
    sVal = cmi8788_read_2(&deviceInfo, AC97_CTRL);
    
    /* check if there's an onboard AC97 codec */
    if (sVal & AC97_CODEC0)
        IOLog("XonarSTDeluxeAudioDevice[%p]::initHardware(%p) AC97 codec0 found\n", this, provider);
    /* check if there's an front panel AC97 codec */
    if (sVal & AC97_CODEC1)
        IOLog("XonarSTDeluxeAudioDevice[%p]::initHardware(%p) AC97 codec1 found\n", this, provider);
    
    deviceInfo.anti_pop_delay = 100;
    deviceInfo.output_control_gpio = GPIO_PIN0;
    
    cmi8788_write_1(&deviceInfo, FUNCTION, cmi8788_read_1(&deviceInfo, FUNCTION) | FUNCTION_2WIRE);
    
    cmi8788_write_2(&deviceInfo, GPIO_CONTROL, cmi8788_read_2(&deviceInfo, GPIO_CONTROL) | 0x01FF);
    
    cmi8788_write_2(&deviceInfo, GPIO_DATA, cmi8788_read_2(&deviceInfo, GPIO_DATA) | GPIO_PIN0 | GPIO_PIN8);
    
    /* FIXME:
     * Confusing naming. Invokations of the following functions
     * have nothing to do with PCM1796
     */
    pcm1796_write_i2c(&deviceInfo, XONAR_ST_CLOCK, 0x5, 0x9);
    pcm1796_write_i2c(&deviceInfo, XONAR_ST_CLOCK, 0x2, 0x0);
    pcm1796_write_i2c(&deviceInfo, XONAR_ST_CLOCK, 0x3, 0x0 | (0 << 3) | 0x0 | 0x1);
    pcm1796_write_i2c(&deviceInfo, XONAR_ST_CLOCK, 0x4, (0 << 1) | 0x0);
    pcm1796_write_i2c(&deviceInfo, XONAR_ST_CLOCK, 0x06, 0x00);
    pcm1796_write_i2c(&deviceInfo, XONAR_ST_CLOCK, 0x07, 0x10);
    pcm1796_write_i2c(&deviceInfo, XONAR_ST_CLOCK, 0x08, 0x00);
    pcm1796_write_i2c(&deviceInfo, XONAR_ST_CLOCK, 0x09, 0x00);
    pcm1796_write_i2c(&deviceInfo, XONAR_ST_CLOCK, 0x16, 0x10);
    pcm1796_write_i2c(&deviceInfo, XONAR_ST_CLOCK, 0x17, 0);
    pcm1796_write_i2c(&deviceInfo, XONAR_ST_CLOCK, 0x5, 0x1);
    
    /* Init DAC */
    pcm1796_write(&deviceInfo, XONAR_ST_FRONTDAC, 20, 0);
    pcm1796_write(&deviceInfo, XONAR_ST_FRONTDAC, 18, PCM1796_FMT_24L|PCM1796_ATLD);
    pcm1796_set_volume(&deviceInfo, 75, 75);
    pcm1796_write(&deviceInfo, XONAR_ST_FRONTDAC, 19, 0);
    
    /* check if MPU401 is enabled in MISC register */
    if (cmi8788_read_1(&deviceInfo, MISC_REG) & MISC_MIDI)
        IOLog("XonarSTDeluxeAudioDevice[%p]::initHardware(%p) MPU401 found\n", this, provider);
    
    setDeviceName("Xonar Essence ST Deluxe");
    setDeviceShortName("Xonar ST+H6");
    setManufacturerName("ASUS");
        
    if (!createAudioEngine()) {
        goto Done;
    }
    
    result = true;
    
Done:
    
    if (!result) {
        if (deviceInfo.deviceMap) {
            deviceInfo.deviceMap->release();
            deviceInfo.deviceMap = NULL;
        }
    }
    
    return result;
}

void XonarSTDeluxeAudioDevice::free()
{
    IOLog("XonarSTDeluxeAudioDevice[%p]::free()\n", this);
    
    cmi8788_set_output(&deviceInfo, OUTPUT_LINE);
    
    if (deviceInfo.deviceMap) {
        deviceInfo.deviceMap->release();
        deviceInfo.deviceMap = NULL;
    }
    
    super::free();
}

bool XonarSTDeluxeAudioDevice::createAudioEngine()
{
    bool result = false;
    XonarSTDeluxeAudioEngine *audioEngine = NULL;
    IOAudioControl *control;
    
    IOLog("XonarSTDeluxeAudioDevice[%p]::createAudioEngine()\n", this);
    
    audioEngine = new XonarSTDeluxeAudioEngine;
    if (!audioEngine) {
        goto Done;
    }
    
    // Init the new audio engine with the device registers so it can access them if necessary
    // The audio engine subclass could be defined to take any number of parameters for its
    // initialization - use it like a constructor
    if (!audioEngine->init(&deviceInfo)) {
        goto Done;
    }
    
    // Create a left & right output volume control with an int range from 0 to 65535
    // and a db range from -22.5 to 0.0
    // Once each control is added to the audio engine, they should be released
    // so that when the audio engine is done with them, they get freed properly
    control = IOAudioLevelControl::createVolumeControl(75,	// Initial value
                                                       0,		// min value
                                                       100,	// max value
                                                       (-22 << 16) + (32768),	// -22.5 in IOFixed (16.16)
                                                       0,		// max 0.0 in IOFixed
                                                       kIOAudioControlChannelIDDefaultLeft,
                                                       kIOAudioControlChannelNameLeft,
                                                       0,		// control ID - driver-defined
                                                       kIOAudioControlUsageOutput);
    if (!control) {
        goto Done;
    }
    
    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)volumeChangeHandler, this);
    audioEngine->addDefaultAudioControl(control);
    control->release();
    
    control = IOAudioLevelControl::createVolumeControl(75,	// Initial value
                                                       0,		// min value
                                                       100,	// max value
                                                       (-22 << 16) + (32768),	// min -22.5 in IOFixed (16.16)
                                                       0,		// max 0.0 in IOFixed
                                                       kIOAudioControlChannelIDDefaultRight,	// Affects right channel
                                                       kIOAudioControlChannelNameRight,
                                                       0,		// control ID - driver-defined
                                                       kIOAudioControlUsageOutput);
    if (!control) {
        goto Done;
    }
    
    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)volumeChangeHandler, this);
    audioEngine->addDefaultAudioControl(control);
    control->release();
    
    // Create an output mute control
    control = IOAudioToggleControl::createMuteControl(false,	// initial state - unmuted
                                                      kIOAudioControlChannelIDAll,	// Affects all channels
                                                      kIOAudioControlChannelNameAll,
                                                      0,		// control ID - driver-defined
                                                      kIOAudioControlUsageOutput);
    
    if (!control) {
        goto Done;
    }
    
    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)outputMuteChangeHandler, this);
    audioEngine->addDefaultAudioControl(control);
    control->release();
    
    // Create a left & right input gain control with an int range from 0 to 255
    // and a db range from 0 to 22.5
    control = IOAudioLevelControl::createVolumeControl(255,	// Initial value
                                                       0,		// min value
                                                       255,	// max value
                                                       0,		// min 0.0 in IOFixed
                                                       (22 << 16) + (32768),	// 22.5 in IOFixed (16.16)
                                                       kIOAudioControlChannelIDDefaultLeft,
                                                       kIOAudioControlChannelNameLeft,
                                                       0,		// control ID - driver-defined
                                                       kIOAudioControlUsageInput);
    if (!control) {
        goto Done;
    }
    
    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)gainChangeHandler, this);
    audioEngine->addDefaultAudioControl(control);
    control->release();
    
    
    control = IOAudioSelectorControl::createOutputSelector(OUTPUT_LINE + 1,
                                                           kIOAudioControlChannelIDAll,
                                                           kIOAudioControlChannelNameAll,
                                                           0);
    
    if(!control) {
        goto Done;
    }
    
    ((IOAudioSelectorControl*)control)->addAvailableSelection(OUTPUT_LINE + 1, "Xonar Speakers");
    ((IOAudioSelectorControl*)control)->addAvailableSelection(OUTPUT_REAR_HP + 1, "Xonar Rear HP");
    
    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)outputSelectChangeHandler, this);
    audioEngine->addDefaultAudioControl(control);
    
    
    control = IOAudioLevelControl::createVolumeControl(255,	// Initial value
                                                       0,		// min value
                                                       255,	// max value
                                                       0,		// min 0.0 in IOFixed
                                                       (22 << 16) + (32768),	// max 22.5 in IOFixed (16.16)
                                                       kIOAudioControlChannelIDDefaultRight,	// Affects right channel
                                                       kIOAudioControlChannelNameRight,
                                                       0,		// control ID - driver-defined
                                                       kIOAudioControlUsageInput);
    if (!control) {
        goto Done;
    }
    
    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)gainChangeHandler, this);
    audioEngine->addDefaultAudioControl(control);
    control->release();
    
    // Create an input mute control
    control = IOAudioToggleControl::createMuteControl(false,	// initial state - unmuted
                                                      kIOAudioControlChannelIDAll,	// Affects all channels
                                                      kIOAudioControlChannelNameAll,
                                                      0,		// control ID - driver-defined
                                                      kIOAudioControlUsageInput);
    
    if (!control) {
        goto Done;
    }
    
    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)inputMuteChangeHandler, this);
    audioEngine->addDefaultAudioControl(control);
    control->release();
    
    // Active the audio engine - this will cause the audio engine to have start() and initHardware() called on it
    // After this function returns, that audio engine should be ready to begin vending audio services to the system
    activateAudioEngine(audioEngine);
    // Once the audio engine has been activated, release it so that when the driver gets terminated,
    // it gets freed
    audioEngine->release();
    
    result = true;
    
Done:
    
    if (!result && (audioEngine != NULL)) {
        audioEngine->release();
    }
    
    return result;
}

IOReturn XonarSTDeluxeAudioDevice::volumeChangeHandler(IOService *target, IOAudioControl *volumeControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn result = kIOReturnBadArgument;
    XonarSTDeluxeAudioDevice *audioDevice;
    
    audioDevice = (XonarSTDeluxeAudioDevice *)target;
    if (audioDevice) {
        result = audioDevice->volumeChanged(volumeControl, oldValue, newValue);
    }
    
    return result;
}

IOReturn XonarSTDeluxeAudioDevice::volumeChanged(IOAudioControl *volumeControl, SInt32 oldValue, SInt32 newValue)
{
    IOLog("XonarSTDeluxeAudioDevice[%p]::volumeChanged(%p, %d, %d)\n", this, volumeControl, (int)oldValue, (int)newValue);
    
    if (volumeControl) {
        IOLog("\t-> Channel %u\n", (unsigned int)(volumeControl->getChannelID()));
    }
    
    switch(volumeControl->getChannelID()) {
        case 1:
            // left channel
            pcm1796_set_left_volume(&deviceInfo, newValue);
            break;
        case 2:
            // right channel
            pcm1796_set_right_volume(&deviceInfo, newValue);
            break;
    }
    
    return kIOReturnSuccess;
}

IOReturn XonarSTDeluxeAudioDevice::outputMuteChangeHandler(IOService *target, IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn result = kIOReturnBadArgument;
    XonarSTDeluxeAudioDevice *audioDevice;
    
    audioDevice = (XonarSTDeluxeAudioDevice *)target;
    if (audioDevice) {
        result = audioDevice->outputMuteChanged(muteControl, oldValue, newValue);
    }
    
    return result;
}

IOReturn XonarSTDeluxeAudioDevice::outputMuteChanged(IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue)
{
    IOLog("XonarSTDeluxeAudioDevice[%p]::outputMuteChanged(%p, %d, %d)\n", this, muteControl, (int)oldValue, (int)newValue);
    
    cmi8788_toggle_sound(&deviceInfo, newValue);
    
    return kIOReturnSuccess;
}

IOReturn XonarSTDeluxeAudioDevice::outputSelectChangeHandler(IOService *target, IOAudioControl *outputSelectControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn result = kIOReturnBadArgument;
    XonarSTDeluxeAudioDevice *audioDevice;
    
    audioDevice = (XonarSTDeluxeAudioDevice *)target;
    if (audioDevice) {
        result = audioDevice->outputSelectChanged(outputSelectControl, oldValue, newValue);
    }
    
    return result;
}

IOReturn XonarSTDeluxeAudioDevice::outputSelectChanged(IOAudioControl *outputSelectControl, SInt32 oldValue, SInt32 newValue)
{
    IOLog("XonarSTDeluxeAudioDevice[%p]::outputSelectChanged(%p, %d, %d)\n", this, outputSelectControl, (int)oldValue, (int)newValue);
    
    cmi8788_set_output(&deviceInfo, newValue - 1);
    
    return kIOReturnSuccess;
}

IOReturn XonarSTDeluxeAudioDevice::gainChangeHandler(IOService *target, IOAudioControl *gainControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn result = kIOReturnBadArgument;
    XonarSTDeluxeAudioDevice *audioDevice;
    
    audioDevice = (XonarSTDeluxeAudioDevice *)target;
    if (audioDevice) {
        result = audioDevice->gainChanged(gainControl, oldValue, newValue);
    }
    
    return result;
}

IOReturn XonarSTDeluxeAudioDevice::gainChanged(IOAudioControl *gainControl, SInt32 oldValue, SInt32 newValue)
{
    IOLog("XonarSTDeluxeAudioDevice[%p]::gainChanged(%p, %d, %d)\n", this, gainControl, (int)oldValue, (int)newValue);
    
    if (gainControl) {
        IOLog("\t-> Channel %u\n", (unsigned int)gainControl->getChannelID());
    }
    
    // Add hardware gain change code here
    
    return kIOReturnSuccess;
}

IOReturn XonarSTDeluxeAudioDevice::inputMuteChangeHandler(IOService *target, IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn result = kIOReturnBadArgument;
    XonarSTDeluxeAudioDevice *audioDevice;
    
    audioDevice = (XonarSTDeluxeAudioDevice *)target;
    if (audioDevice) {
        result = audioDevice->inputMuteChanged(muteControl, oldValue, newValue);
    }
    
    return result;
}

IOReturn XonarSTDeluxeAudioDevice::inputMuteChanged(IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue)
{
    IOLog("XonarSTDeluxeAudioDevice[%p]::inputMuteChanged(%p, %d, %d)\n", this, muteControl, (int)oldValue, (int)newValue);
    
    // Add input mute change code here
    
    return kIOReturnSuccess;
}