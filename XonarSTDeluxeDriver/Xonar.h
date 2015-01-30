#ifndef _XONAR_H
#define _XONAR_H

#define CMEDIA_VENDOR_ID	0x13F6
#define CMEDIA_CMI8788		0x8788

/* most DMA channels have a 16-bit counter for 32-bit words */
#define BUFFER_BYTES_MAX		((1 << 16) * 4)
/* the multichannel DMA channel has a 24-bit counter */
#define BUFFER_BYTES_MAX_MULTICH	((1 << 24) * 4)
#define PERIOD_BYTES_MIN		64

#define DEFAULT_BUFFER_BYTES		(BUFFER_BYTES_MAX / 2)
#define DEFAULT_BUFFER_BYTES_MULTICH	(1024 * 1024)

/* Device IDs */
#define ASUS_VENDOR_ID		0x1043
#define SUBID_XONAR_D2		0x8269
#define SUBID_XONAR_D2X		0x82b7
#define SUBID_XONAR_D1		0x834f
#define SUBID_XONAR_DX		0x8275
#define SUBID_XONAR_STX 	0x835c
#define SUBID_XONAR_DS		0x838e
#define SUBID_XONAR_ST		0x835d

/*
 * CM8338 registers definition
 */

#define RECA_ADDR		0x00
#define RECA_SIZE		0x04
#define RECA_FRAG		0x06
#define RECB_ADDR		0x08
#define RECB_SIZE		0x0C
#define RECB_FRAG		0x0E
#define RECC_ADDR		0x10
#define RECC_SIZE		0x14
#define RECC_FRAG		0x16
#define SPDIF_ADDR		0x18
#define SPDIF_SIZE		0x1C
#define SPDIF_FRAG		0x1E
#define MULTICH_ADDR		0x20
#define MULTICH_SIZE		0x24
#define MULTICH_FRAG		0x28
#define FPOUT_ADDR		0x30
#define FPOUT_SIZE		0x34
#define FPOUT_FRAG		0x36

#define CHANNEL_MULTICH 	0x10

#define DMA_START		0x40
#define CHAN_RESET		0x42
#define MULTICH_MODE		0x43
#define  MULTICH_MODE_CH_MASK	0x03
#define  MULTICH_MODE_2CH 	0x00
#define  MULTICH_MODE_4CH 	0x01
#define  MULTICH_MODE_6CH 	0x02
#define  MULTICH_MODE_8CH 	0x03
#define IRQ_MASK		0x44
#define IRQ_STAT		0x46
#define MISC_REG		0x48
#define  MISC_PCI_MEM_W_1_CLOCK	0x20
#define  MISC_MIDI 		0x40
#define REC_FORMAT		0x4A
#define PLAY_FORMAT		0x4B
#define  MULTICH_FORMAT_MASK 	0x0C
#define REC_MODE		0x4C
#define FUNCTION		0x50

#define  FUNCTION_RESET_CODEC 	0x02
#define  FUNCTION_2WIRE 		0x40

#define I2S_MULTICH_FORMAT	0x60
#define  I2S_MASTER 		0x0100

#define  I2S_BITS_MASK 		0x00c0
#define  I2S_FMT_I2S 		0x0000
#define  I2S_FMT_LJUST 		0x0008
#define  I2S_FMT_BITS16 	0x0000
#define  I2S_FMT_BITS24 	0x0080
#define  I2S_FMT_BITS32 	0x00c0

#define  I2S_FMT_RATE_MASK	0x0007
#define  I2S_FMT_RATE32 	0x0000
#define  I2S_FMT_RATE44 	0x0001
#define  I2S_FMT_RATE48 	0x0002
#define  I2S_FMT_RATE64 	0x0003
#define  I2S_FMT_RATE88 	0x0004
#define  I2S_FMT_RATE96 	0x0005
#define  I2S_FMT_RATE176 	0x0006
#define  I2S_FMT_RATE192 	0x0007

#define I2S_ADC1_FORMAT		0x62
#define I2S_ADC2_FORMAT		0x64
#define I2S_ADC3_FORMAT		0x66

#define SPDIF_FUNC		0x70
#define SPDIFOUT_CHAN_STAT	0x74
#define SPDIFIN_CHAN_STAT	0x78

#define I2C_ADDR		0x90
#define I2C_MAP			0x91
#define I2C_DATA		0x92
#define I2C_CTRL		0x94
#define  TWOWIRE_BUSY 		0x001
#define  TWOWIRE_SPEED_FAST 	0x100

#define SPI_CONTROL		0x98
#define SPI_DATA		0x99

#define MPU401_DATA		0xA0
#define MPU401_COMMAND		0xA1
#define MPU401_CONTROL		0xA2

#define GPI_DATA		0xA4
#define GPI_IRQ_MASK		0xA5
#define GPIO_DATA		0xA6
#define  GPIO_PIN0 		(1<<0)
#define  GPIO_PIN1 		(1<<1)
#define  GPIO_PIN2 		(1<<2)
#define  GPIO_PIN3 		(1<<3)
#define  GPIO_PIN4 		(1<<4)
#define  GPIO_PIN5 		(1<<5)
#define  GPIO_PIN6 		(1<<6)
#define  GPIO_PIN7 		(1<<7)
#define  GPIO_PIN8 		(1<<8)
#define  GPIO_PIN9 		(1<<9)
#define GPIO_CONTROL		0xA8
#define GPIO_IRQ_MASK		0xAA
#define DEVICE_SENSE		0xAC

#define PLAY_ROUTING		0xC0

#define REC_ROUTING		0xC2
#define REC_MONITOR		0xC3
#define MONITOR_ROUTING		0xC4

#define AC97_CTRL		0xD0
#define  AC97_COLD_RESET 	0x0001
#define  AC97_STATUS_SUSPEND 	0x0002
#define  AC97_RESUME 		0x0002
#define  AC97_CODEC0 		0x0010
#define  AC97_CODEC1 		0x0020
#define AC97_INTR_MASK		0xD2
#define AC97_INTR_STAT		0xD3
#define AC97_OUT_CHAN_CONFIG	0xD4
#define AC97_IN_CHAN_CONFIG	0xD8
#define AC97_CMD_DATA		0xDC

#define CODEC_VERSION		0xE4

#define CTRL_VERSION		0xE6
#define  CTRL_VERSION2 		0x0008

/* Device IDs */
#define ASUS_VENDOR_ID		0x1043
#define SUBID_XONAR_D2		0x8269
#define SUBID_XONAR_D2X		0x82b7
#define SUBID_XONAR_D1		0x834f
#define SUBID_XONAR_DX		0x8275
#define SUBID_XONAR_STX 	0x835c
#define SUBID_XONAR_DS		0x838e
#define SUBID_XONAR_ST		0x835d

#define SUBID_GENERIC		0x0000

/* Xonar specific */
#define XONAR_DX_FRONTDAC	0x9e
#define XONAR_DX_SURRDAC	0x30
#define XONAR_STX_FRONTDAC	0x98
#define XONAR_ST_FRONTDAC	0x98
#define XONAR_ST_CLOCK		0x9c
#define XONAR_DS_FRONTDAC	0x1
#define XONAR_DS_SURRDAC	0x0
#define XONAR_MCLOCK_256	0x10
#define XONAR_MCLOCK_512	0x20

/* PCM1796 defines */
/* register 16 */
#define PCM1796_ATL 		0xff

/* register 17 */
#define PCM1796_ATR 		0xff

/* register 18 */
#define PCM1796_MUTE 		0x01
#define PCM1796_DME 		0x02
#define PCM1796_DMF 		0x0c
#define  PCM1796_DMF_DISABLE	0x00
#define  PCM1796_DMF_48 	0x04
#define  PCM1796_DMF_44 	0x08
#define  PCM1796_DMF_32 	0x0c
#define PCM1796_FMT 		0x70
#define  PCM1796_FMT_16R 	0x00
#define  PCM1796_FMT_20R 	0x10
#define  PCM1796_FMT_24R 	0x20
#define  PCM1796_FMT_24L 	0x30
#define  PCM1796_FMT_16I 	0x40
#define  PCM1796_FMT_24I 	0x50 /* default */
#define PCM1796_ATLD 		0x80

/* register 19 */
#define PCM1796_INZD 		0x01
#define PCM1796_FLT 		0x02
#define PCM1796_DFMS 		0x04
#define PCM1796_OPE 		0x10
#define PCM1796_ATS 		0x60

/* register 20 */
#define PCM1796_OS 		0x03
#define PCM1796_CHSL 		0x04
#define PCM1796_MONO 		0x08
#define PCM1796_DFTH 		0x10
#define PCM1796_DSD 		0x20
#define PCM1796_SRST 		0x40

/* register 21 */
#define PCM1796_PCMZ 		0x01
#define PCM1796_DZ 		0x6

/* register indexes in regs[] array */
#define PCM1796_REGBASE 	16
#define PCM1796_NREGS 		6
#define PCM1796_REG16 		0
#define PCM1796_REG17 		1
#define PCM1796_REG18 		2
#define PCM1796_REG19 		3
#define PCM1796_REG20 		4
#define PCM1796_REG21 		5

/* defs for AKM 4396 DAC */
#define AK4396_CTL1        0x00
#define AK4396_CTL2        0x01
#define AK4396_CTL3        0x02
#define AK4396_LchATTCtl   0x03
#define AK4396_RchATTCtl   0x04

/* defs for CS4398 DAC */
#define CS4398_CHIP_ID	  0x01
#define CS4398_MODE_CTRL  0x02
#define CS4398_MIXING	  0x03
#define CS4398_MUTE_CTRL  0x04
#define CS4398_VOLA       0x05
#define CS4398_VOLB       0x06
#define CS4398_RAMP_CTRL  0x07
#define CS4398_MISC_CTRL  0x08
#define CS4398_MISC2_CTRL 0x09

#define CS4398_POWER_DOWN (1<<7)	/* Obvious */
#define CS4398_CPEN	  (1<<6)	/* Control Port Enable */
#define CS4398_FREEZE	  (1<<5)	/* Freezes registers, unfreeze to
* accept changed registers */
#define CS4398_MCLKDIV2   (1<<4)	/* Divide MCLK by 2 */
#define	CS4398_MCLKDIV3   (1<<3)	/* Divive MCLK by 3 */
#define CS4398_I2S	  (1<<4)	/* Set I2S mode */

/* defs for CS4362A DAC */
#define CS4362A_MODE1_CTRL 	0x01
#define CS4362A_MODE2_CTRL	0x02
#define CS4362A_MODE3_CTRL	0x03
#define CS4362A_FILTER_CTRL	0x04
#define CS4362A_INVERT_CTRL	0x05
#define CS4362A_MIX1_CTRL	0x06
#define CS4362A_VOLA_1		0x07
#define CS4362A_VOLB_1		0x08
#define CS4362A_MIX2_CTRL	0x09
#define CS4362A_VOLA_2		0x0A
#define CS4362A_VOLB_2		0x0B
#define CS4362A_MIX3_CTRL	0x0C
#define CS4362A_VOLA_3		0x0D
#define CS4362A_VOLB_3		0x0E
#define CS4362A_CHIP_REV	0x12

/* CS4362A Reg 01h */
#define CS4362A_CPEN		(1<<7)
#define CS4362A_FREEZE		(1<<6)
#define CS4362A_MCLKDIV		(1<<5)
#define CS4362A_DAC3_ENABLE	(1<<3)
#define CS4362A_DAC2_ENABLE	(1<<2)
#define CS4362A_DAC1_ENABLE	(1<<1)
#define CS4362A_POWER_DOWN	(1)

/* CS4362A Reg 02h */
#define CS4362A_DIF_LJUST	0x00
#define CS4362A_DIF_I2S		0x10
#define CS4362A_DIF_RJUST16	0x20
#define CS4362A_DIF_RJUST24	0x30
#define CS4362A_DIF_RJUST20	0x40
#define CS4362A_DIF_RJUST18	0x50

/* CS4362A Reg 03h */
#define CS4362A_RAMP_IMMEDIATE  0x00
#define CS4362A_RAMP_ZEROCROSS	0x40
#define CS4362A_RAMP_SOFT	0x80
#define CS4362A_RAMP_SOFTZERO   0xC0
#define CS4362A_SINGLE_VOL	0x20
#define CS4362A_RAMP_ERROR	0x10
#define CS4362A_MUTEC_POL	0x08
#define CS4362A_AUTOMUTE	0x04
#define CS4362A_SIX_MUTE	0x00
#define CS4362A_ONE_MUTE	0x01
#define CS4362A_THREE_MUTE	0x03

/* CS4362A Reg 04h */
#define CS4362A_FILT_SEL	0x10
#define CS4362A_DEM_NONE	0x00
#define CS4362A_DEM_44KHZ	0x02
#define CS4362A_DEM_48KHZ	0x04
#define CS4362A_DEM_32KHZ	0x06
#define CS4362A_RAMPDOWN	0x01

/* CS4362A Reg 05h */
#define CS4362A_INV_A3 		(1<<4)
#define CS4362A_INV_B3 		(1<<5)
#define CS4362A_INV_A2		(1<<2)
#define CS4362A_INV_B2		(1<<3)
#define CS4362A_INV_A1		(1)
#define CS4362A_INV_B1		(1<<1)

/* CS4362A Reg 06h, 09h, 0Ch */
/* ATAPI crap, does anyone still use analog CD playback? */

/* CS4362A Reg 07h, 08h, 0Ah, 0Bh, 0Dh, 0Eh */
/* Volume registers */
#define CS4362A_VOL_MUTE	0x80


/* 0-100. Start at -96dB. */
#define CS4398_VOL(x) \
((x) == 0 ? 0xFF : (0xC0 - ((x)*192/100)))
/* 0-100. Start at -96dB. Bit 7 is mute. */
#define CS4362A_VOL(x) \
(char)((x) == 0 ? 0xFF : (0x60 - ((x)*96/100)))

#define UNUSED_CMI9780_CONTROLS ( \
SOUND_MASK_VOLUME | \
SOUND_MASK_PCM | \
SOUND_MASK_REARVOL | \
SOUND_MASK_CENTERVOL | \
SOUND_MASK_SIDEVOL | \
SOUND_MASK_SPEAKER | \
SOUND_MASK_ALTPCM | \
SOUND_MASK_VIDEO | \
SOUND_MASK_DEPTH | \
SOUND_MASK_MONO | \
SOUND_MASK_PHONE \
)


#define OUTPUT_LINE 0
#define OUTPUT_REAR_HP 1
#define OUTPUT_HP 2


#include <IOKit/audio/IOAudioDevice.h>

class IOPCIDevice;
class IOMemoryMap;

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
    IOPCIDevice *pciDevice;
    IOMemoryMap *deviceMap;
    
    int output;
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

#endif