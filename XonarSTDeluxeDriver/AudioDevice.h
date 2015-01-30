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
#include "Xonar.h"

struct xonar_info;

struct xonar_chinfo {
    struct snd_dbuf		*buffer;
    struct pcm_channel 	*channel;
    struct xonar_info 	*parent;
    int			dir;
    u_int32_t		fmt, spd, phys_buf, bps;
    int 			adc_type;
    int 			dac_type;
    int 			play_dma_start;
    int 			play_irq_mask;
    int 			play_chan_reset;
    int 			state;
    int 			blksz;
};

struct pcm1796_info {
    UInt16      regs[PCM1796_NREGS];
    int 		rolloff;
    int 		bypass;
    int 		deemph;
    int 		hp;
    int 		hp_gain;
};

struct xonar_info {
    int 			output;
    struct resource *reg, *irq;
    int regtype, regid, irqid;
    
    void *ih;
    
    uint16_t model;
    
    int vol[2];
    int bufmaxsz, bufsz;
    int pnum;
    struct pcm1796_info pcm1796;
    struct xonar_chinfo chan[2];
    
    int anti_pop_delay;
    int output_control_gpio;
};


class IOPCIDevice;
class IOMemoryMap;

#define XonarSTDeluxeAudioDevice com_GeneC_driver_XonarSTDeluxeDevice

class XonarSTDeluxeAudioDevice : public IOAudioDevice
{
    friend class XonarSTDeluxeAudioEngine;
    
    OSDeclareDefaultStructors(XonarSTDeluxeAudioDevice)
    
    IOPCIDevice					*pciDevice;
    IOMemoryMap					*deviceMap;
    
    struct xonar_info           deviceInfo;
    
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
    
    public:
    static unsigned int pcm1796_vol_scale(int vol);
    void pcm1796_set_volume(int left, int right);
    void pcm1796_set_mute(int mute);
    
    void cmi8788_set_output(int which);
    void cmi8788_toggle_sound(int output);
    
    void cmi8788_write_4(int reg, u_int32_t data);
    void cmi8788_write_2(int reg, u_int16_t data);
    void cmi8788_write_1(int reg, u_int8_t data);
    
    UInt32 cmi8788_read_4(int reg);
    UInt16 cmi8788_read_2(int reg);
    UInt8 cmi8788_read_1(int reg);
    
    int pcm1796_write_i2c(uint8_t codec_num, uint8_t reg, uint8_t data);
    int pcm1796_write(uint8_t codec_num, uint8_t reg, uint8_t data);
};

#endif /* _XONARSTDELUXEAUDIODEVICE_H */