//
//  XonarIO.cpp
//  XonarSTDeluxeDriver
//
//  Created by Gene Chen on 2015-01-29.
//  Copyright (c) 2015 Gene Chen. All rights reserved.
//

#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>

#include "Xonar.h"

/*
 * Xonar Essence ST (Deluxe)/STX (II)
 * ----------------------------------
 *
 * CMI8788:
 *
 *   I²C <-> PCM1792A (addr 1001100)
 *       <-> CS2000 (addr 1001110) (ST only)
 *
 *   ADC1 MCLK -> REF_CLK of CS2000 (ST only)
 *
 *   GPI 0 <- external power present (STX only)
 *
 *   GPIO 0 -> enable output to speakers
 *   GPIO 1 -> route HP to front panel (0) or rear jack (1)
 *   GPIO 2 -> M0 of CS5381
 *   GPIO 3 -> M1 of CS5381
 *   GPIO 4 <- daughterboard detection
 *   GPIO 5 <- daughterboard detection
 *   GPIO 6 -> ?
 *   GPIO 7 -> route output to speaker jacks (0) or HP (1)
 *   GPIO 8 -> route input jack to line-in (0) or mic-in (1)
 *
 * PCM1792A:
 *
 *   SCK <- CLK_OUT of CS2000 (ST only)
 *
 * CM9780:
 *
 *   LINE_OUT -> input of ADC
 *
 *   AUX_IN <- aux
 *   MIC_IN <- mic
 *
 *   GPO 0 -> route line-in (0) or AC97 output (1) to CS5381 input
 *
 * H6 daughterboard
 * ----------------
 *
 * GPIO 4 <- 0
 * GPIO 5 <- 0
 */

/* C-Media CMI8788 interface */
void cmi8788_write_4(xonar_info* dev, int reg, u_int32_t data)
{
    dev->pciDevice->ioWrite32(reg, data, dev->deviceMap);
}

void cmi8788_write_2(xonar_info* dev, int reg, u_int16_t data)
{
    dev->pciDevice->ioWrite16(reg, data, dev->deviceMap);
}

void cmi8788_write_1(xonar_info* dev, int reg, u_int8_t data)
{
    dev->pciDevice->ioWrite8(reg, data, dev->deviceMap);
}

UInt32 cmi8788_read_4(xonar_info* dev, int reg)
{
    return dev->pciDevice->ioRead32(reg, dev->deviceMap);
}

UInt16 cmi8788_read_2(xonar_info* dev, int reg)
{
    return dev->pciDevice->ioRead16(reg, dev->deviceMap);
}

UInt8 cmi8788_read_1(xonar_info* dev, int reg)
{
    return dev->pciDevice->ioRead8(reg, dev->deviceMap);
}
/* Texas Instruments PCM1796 interface */
int pcm1796_write_i2c(xonar_info* dev, uint8_t codec_num, uint8_t reg, uint8_t data)
{
    int count = 50;
    
    /* Wait for it to stop being busy */
    while ((cmi8788_read_2(dev, I2C_CTRL) & TWOWIRE_BUSY) && (count > 0)) {
        IODelay(10);
        count--;
    }
    if (count == 0) {
        IOLog("XonarIO i2c timeout\n");
        return 5;
    }
    
    /* first write the Register Address into the MAP register */
    cmi8788_write_1(dev, I2C_MAP, reg);
    
    /* now write the data */
    cmi8788_write_1(dev, I2C_DATA, data);
    
    /* select the codec number to address */
    cmi8788_write_1(dev, I2C_ADDR, codec_num);
    IODelay(100);
    
    return 1;
}

int pcm1796_write(xonar_info* dev, uint8_t codec_num, uint8_t reg, uint8_t data) {
    /*
     * XXX: add support for SPI
     */
    dev->pcm1796.regs[reg - PCM1796_REGBASE] = data;
    return pcm1796_write_i2c(dev, codec_num, reg, data);
}

void pcm1796_set_mute(xonar_info* dev, int mute)
{
    uint16_t reg = dev->pcm1796.regs[PCM1796_REG18];
    
    if (mute)
        pcm1796_write(dev, XONAR_STX_FRONTDAC, 18, reg | PCM1796_MUTE);
    else
        pcm1796_write(dev, XONAR_STX_FRONTDAC, 18, reg & ~PCM1796_MUTE);
}

// Generated logarithmic value lookup table:
//
// textwrap.wrap(', '.join(['  0'] + [
//     format(math.log(x) / math.log(100) * 255, '>3.0f')
//     for x in range(1, 101)
// ]))
//
static const uint8_t vol_scale_table[] =
{
      0,   0,  38,  61,  77,  89,  99, 108, 115, 122, 128, 133, 138, 142,
    146, 150, 154, 157, 160, 163, 166, 169, 171, 174, 176, 178, 180, 182,
    185, 186, 188, 190, 192, 194, 195, 197, 198, 200, 201, 203, 204, 206,
    207, 208, 210, 211, 212, 213, 214, 216, 217, 218, 219, 220, 221, 222,
    223, 224, 225, 226, 227, 228, 229, 229, 230, 231, 232, 233, 234, 234,
    235, 236, 237, 238, 238, 239, 240, 241, 241, 242, 243, 243, 244, 245,
    245, 246, 247, 247, 248, 249, 249, 250, 250, 251, 252, 252, 253, 253,
    254, 254, 255
};

unsigned int pcm1796_vol_scale(int vol)
{
    // Apply logarithmic volume scale.
    return vol_scale_table[min(max(vol, 0), sizeof(vol_scale_table))];
}

void pcm1796_set_left_volume(xonar_info* dev, int left)
{
    pcm1796_write(dev, XONAR_STX_FRONTDAC, 16, pcm1796_vol_scale(left));
}

void pcm1796_set_right_volume(xonar_info* dev, int right)
{
    pcm1796_write(dev, XONAR_STX_FRONTDAC, 17, pcm1796_vol_scale(right));
}

void pcm1796_set_volume(xonar_info* dev, int left, int right)
{
    pcm1796_set_left_volume(dev, left);
    pcm1796_set_right_volume(dev, left);
}

void cmi8788_toggle_sound(xonar_info* dev, int output) {
    uint16_t data, ctrl;
    
    if (output) {
        ctrl = cmi8788_read_2(dev, GPIO_CONTROL);
        cmi8788_write_2(dev, GPIO_CONTROL, ctrl | dev->output_control_gpio);
        IODelay(dev->anti_pop_delay * 1000);
        data = cmi8788_read_2(dev, GPIO_DATA);
        cmi8788_write_2(dev, GPIO_DATA, data | dev->output_control_gpio);
    } else {
        /* Mute DAC before toggle GPIO to avoid another pop */
        pcm1796_set_mute(dev, 1);
        data = cmi8788_read_2(dev, GPIO_DATA);
        cmi8788_write_2(dev, GPIO_DATA,
                        data & ~dev->output_control_gpio);
        pcm1796_set_mute(dev, 0);
    }
}


void cmi8788_set_output(xonar_info* dev, int which)
{
    uint16_t val;
    
    cmi8788_toggle_sound(dev, 0);
    
    /*
     * GPIO1 - front (0) or rear (1) HP jack
     * GPIO7 - speakers (0) or HP (1)
     */
    val = cmi8788_read_2(dev, GPIO_DATA);
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
    
    cmi8788_write_2(dev, GPIO_DATA, val);
    cmi8788_toggle_sound(dev, 1);
}

