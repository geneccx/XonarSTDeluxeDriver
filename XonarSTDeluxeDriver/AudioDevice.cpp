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
#include <IOKit/audio/IOAudioDefines.h>

#include <IOKit/IOLib.h>

#include <IOKit/pci/IOPCIDevice.h>

#define super IOAudioDevice

OSDefineMetaClassAndStructors(XonarSTDeluxeAudioDevice, IOAudioDevice)

#define OUTPUT_LINE 		0
#define OUTPUT_REAR_HP 		1
#define OUTPUT_HP 		2

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
    pciDevice = OSDynamicCast(IOPCIDevice, provider);
    if (!pciDevice) {
        goto Done;
    }
    
    // Config a map for the PCI config base registers
    // We need to keep this map around until we're done accessing the registers
    deviceMap = pciDevice->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if (!deviceMap) {
        goto Done;
    }
    
    // Get the virtual address for the registers - mapped in the kernel address space
    /*deviceRegisters = (XonarSTDeluxeAudioDeviceRegisters *)deviceMap->getVirtualAddress();
    if (!deviceRegisters) {
        goto Done;
    }*/
    
    pciDevice->setIOEnable(true);
    pciDevice->setBusMasterEnable(true);
    
    // Enable the PCI memory access - the kernel will panic if this isn't done before accessing the
    // mapped registers
    pciDevice->setMemoryEnable(true);
    
    /* Init CMI controller */
    sVal = cmi8788_read_2(CTRL_VERSION);
    if (!(sVal & CTRL_VERSION2)) {
        bVal = cmi8788_read_1(MISC_REG);
        bVal |= MISC_PCI_MEM_W_1_CLOCK;
        cmi8788_write_1(MISC_REG, bVal);
    }
    bVal = cmi8788_read_1(FUNCTION);
    bVal |= FUNCTION_RESET_CODEC;
    cmi8788_write_1(FUNCTION, bVal);
    
    /* set up DAC related settings */
    sDac = I2S_MASTER | I2S_FMT_RATE44 | I2S_FMT_LJUST | I2S_FMT_BITS16 | XONAR_MCLOCK_512;
    
    cmi8788_write_2(I2S_MULTICH_FORMAT, sDac);
    cmi8788_write_2(I2S_ADC1_FORMAT, sDac);
    cmi8788_write_2(I2S_ADC2_FORMAT, sDac);
    cmi8788_write_2(I2S_ADC3_FORMAT, sDac);
    
    /* setup routing regs with default values */
    cmi8788_write_2(PLAY_ROUTING, 0xE400);
    cmi8788_write_1(REC_ROUTING, 0x00);
    cmi8788_write_1(REC_MONITOR, 0x00);
    cmi8788_write_1(MONITOR_ROUTING, 0xE4);
    
    /* AC97 dances. Who needs it anyway? */
    /* Cold reset onboard AC97 */
    cmi8788_write_2(AC97_CTRL, AC97_COLD_RESET);
    count = 100;
    while ((cmi8788_read_2(AC97_CTRL) & AC97_STATUS_SUSPEND) && (count--))
    {
        cmi8788_write_2(AC97_CTRL,
                        (cmi8788_read_2(AC97_CTRL)
                         & ~AC97_STATUS_SUSPEND) | AC97_RESUME);
        
        IODelay(100);
    }
    
    if (!count)
        IOLog("XonarSTDeluxeAudioDevice[%p]::initHardware(%p) AC97 not ready\n", this, provider);
    
    sVal = cmi8788_read_2(AC97_CTRL);
    
    /* check if there's an onboard AC97 codec */
    if (sVal & AC97_CODEC0)
        IOLog("XonarSTDeluxeAudioDevice[%p]::initHardware(%p) AC97 codec0 found\n", this, provider);
    /* check if there's an front panel AC97 codec */
    if (sVal & AC97_CODEC1)
        IOLog("XonarSTDeluxeAudioDevice[%p]::initHardware(%p) AC97 codec1 found\n", this, provider);
    
    deviceInfo.anti_pop_delay = 100;
    deviceInfo.output_control_gpio = GPIO_PIN0;
    
    cmi8788_write_1(FUNCTION, cmi8788_read_1(FUNCTION) | FUNCTION_2WIRE);
    
    cmi8788_write_2(GPIO_CONTROL, cmi8788_read_2(GPIO_CONTROL) | 0x01FF);
    
    cmi8788_write_2(GPIO_DATA, cmi8788_read_2(GPIO_DATA) | GPIO_PIN0 | GPIO_PIN8);
    
    /* FIXME:
     * Confusing naming. Invokations of the following functions
     * have nothing to do with PCM1796
     */
    pcm1796_write_i2c(XONAR_ST_CLOCK, 0x5, 0x9);
    pcm1796_write_i2c(XONAR_ST_CLOCK, 0x2, 0x0);
    pcm1796_write_i2c(XONAR_ST_CLOCK, 0x3, 0x0 | (0 << 3) | 0x0 | 0x1);
    pcm1796_write_i2c(XONAR_ST_CLOCK, 0x4, (0 << 1) | 0x0);
    pcm1796_write_i2c(XONAR_ST_CLOCK, 0x06, 0x00);
    pcm1796_write_i2c(XONAR_ST_CLOCK, 0x07, 0x10);
    pcm1796_write_i2c(XONAR_ST_CLOCK, 0x08, 0x00);
    pcm1796_write_i2c(XONAR_ST_CLOCK, 0x09, 0x00);
    pcm1796_write_i2c(XONAR_ST_CLOCK, 0x16, 0x10);
    pcm1796_write_i2c(XONAR_ST_CLOCK, 0x17, 0);
    pcm1796_write_i2c(XONAR_ST_CLOCK, 0x5, 0x1);
    
    /* Init DAC */
    pcm1796_write(XONAR_ST_FRONTDAC, 20, 0);
    pcm1796_write(XONAR_ST_FRONTDAC, 18, PCM1796_FMT_24L|PCM1796_ATLD);
    pcm1796_set_volume(75, 75);
    pcm1796_write(XONAR_ST_FRONTDAC, 19, 0);
    
    /* check if MPU401 is enabled in MISC register */
    if (cmi8788_read_1 (MISC_REG) & MISC_MIDI)
        IOLog("XonarSTDeluxeAudioDevice[%p]::initHardware(%p) MPU401 found\n", this, provider);
    
    // default attached to headphone
    cmi8788_set_output(OUTPUT_REAR_HP);
    
    setDeviceName("Xonar Essence ST Deluxe");
    setDeviceShortName("Xonar ST+H6");
    setManufacturerName("ASUS");
        
    if (!createAudioEngine()) {
        goto Done;
    }
    
    result = true;
    
Done:
    
    if (!result) {
        if (deviceMap) {
            deviceMap->release();
            deviceMap = NULL;
        }
    }
    
    return result;
}

void XonarSTDeluxeAudioDevice::free()
{
    IOLog("XonarSTDeluxeAudioDevice[%p]::free()\n", this);
    
    cmi8788_set_output(OUTPUT_LINE);
    
    if (deviceMap) {
        deviceMap->release();
        deviceMap = NULL;
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
    control = IOAudioLevelControl::createVolumeControl(65535,	// Initial value
                                                       0,		// min value
                                                       65535,	// max value
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
    
    control = IOAudioLevelControl::createVolumeControl(65535,	// Initial value
                                                       0,		// min value
                                                       65535,	// max value
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
    
    // Create a left & right input gain control with an int range from 0 to 65535
    // and a db range from 0 to 22.5
    control = IOAudioLevelControl::createVolumeControl(65535,	// Initial value
                                                       0,		// min value
                                                       65535,	// max value
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
    
    control = IOAudioLevelControl::createVolumeControl(65535,	// Initial value
                                                       0,		// min value
                                                       65535,	// max value
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
    
    // Add hardware volume code change
    
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
    
    cmi8788_toggle_sound(!newValue);
    
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

void XonarSTDeluxeAudioDevice::cmi8788_set_output(int which)
{
    uint16_t val;

    cmi8788_toggle_sound(0);

    /*
     * GPIO1 - front (0) or rear (1) HP jack
     * GPIO7 - speakers (0) or HP (1)
     */
    val = cmi8788_read_2(GPIO_DATA);
    switch (which) {
        case OUTPUT_LINE:
            val &= ~(GPIO_PIN7|GPIO_PIN1);
            break;
        case OUTPUT_REAR_HP:
            val |= (GPIO_PIN7|GPIO_PIN1);
            break;
        case OUTPUT_HP:
            val |= GPIO_PIN7;
            val &= ~GPIO_PIN1;
            break;
    }
    
    cmi8788_write_2(GPIO_DATA, val);
    cmi8788_toggle_sound(1);
}


void XonarSTDeluxeAudioDevice::pcm1796_set_mute(int mute)
{
    uint16_t reg = deviceInfo.pcm1796.regs[PCM1796_REG18];
    
    if (mute)
        pcm1796_write(XONAR_STX_FRONTDAC, 18, reg | PCM1796_MUTE);
    else
        pcm1796_write(XONAR_STX_FRONTDAC, 18, reg & ~PCM1796_MUTE);
}

void XonarSTDeluxeAudioDevice::cmi8788_toggle_sound(int output) {
    uint16_t data, ctrl;
    
    if (output) {
        ctrl = cmi8788_read_2(GPIO_CONTROL);
        cmi8788_write_2(GPIO_CONTROL, ctrl | deviceInfo.output_control_gpio);
        IODelay(deviceInfo.anti_pop_delay * 1000);
        data = cmi8788_read_2(GPIO_DATA);
        cmi8788_write_2(GPIO_DATA, data | deviceInfo.output_control_gpio);
    } else {
        /* Mute DAC before toggle GPIO to avoid another pop */
        pcm1796_set_mute(1);
        data = cmi8788_read_2(GPIO_DATA);
        cmi8788_write_2(GPIO_DATA,
                        data & ~deviceInfo.output_control_gpio);
        pcm1796_set_mute(0);
    }
}

unsigned int XonarSTDeluxeAudioDevice::pcm1796_vol_scale(int vol)
{
    /* 0-14 - mute, 255 - max */
    return (vol * 241)/100;
}

void XonarSTDeluxeAudioDevice::pcm1796_set_volume(int left, int right)
{
    pcm1796_write(XONAR_STX_FRONTDAC, 16, pcm1796_vol_scale(left));
    pcm1796_write(XONAR_STX_FRONTDAC, 17, pcm1796_vol_scale(right));
}

/* C-Media CMI8788 interface */
void XonarSTDeluxeAudioDevice::cmi8788_write_4(int reg, u_int32_t data)
{
    pciDevice->ioWrite32(reg, data, deviceMap);
}

void XonarSTDeluxeAudioDevice::cmi8788_write_2(int reg, u_int16_t data)
{
    pciDevice->ioWrite16(reg, data, deviceMap);
}

void XonarSTDeluxeAudioDevice::cmi8788_write_1(int reg, u_int8_t data)
{
    pciDevice->ioWrite8(reg, data, deviceMap);
}

UInt32 XonarSTDeluxeAudioDevice::cmi8788_read_4(int reg)
{
    return pciDevice->ioRead32(reg, deviceMap);
}

UInt16 XonarSTDeluxeAudioDevice::cmi8788_read_2(int reg)
{
    return pciDevice->ioRead16(reg, deviceMap);
}

UInt8 XonarSTDeluxeAudioDevice::cmi8788_read_1(int reg)
{
    return pciDevice->ioRead8(reg, deviceMap);
}
/* Texas Instruments PCM1796 interface */
int XonarSTDeluxeAudioDevice::pcm1796_write_i2c(uint8_t codec_num, uint8_t reg, uint8_t data)
{
    int count = 50;
    
    /* Wait for it to stop being busy */
    while ((cmi8788_read_2(I2C_CTRL) & TWOWIRE_BUSY) && (count > 0)) {
        IODelay(10);
        count--;
    }
    if (count == 0) {
        IOLog("XonarSTDeluxeAudioDevice[%p] i2c timeout\n", this);
        return 5;
    }
    
    /* first write the Register Address into the MAP register */
    cmi8788_write_1(I2C_MAP, reg);
    
    /* now write the data */
    cmi8788_write_1(I2C_DATA, data);
    
    /* select the codec number to address */
    cmi8788_write_1(I2C_ADDR, codec_num);
    IODelay(100);
    
    return 1;
}

int XonarSTDeluxeAudioDevice::pcm1796_write(uint8_t codec_num, uint8_t reg, uint8_t data) {
    /*
     * XXX: add support for SPI
     */
    deviceInfo.pcm1796.regs[reg - PCM1796_REGBASE] = data;
    return pcm1796_write_i2c(codec_num, reg, data);
}