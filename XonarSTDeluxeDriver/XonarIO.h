//
//  XonarIO.h
//  XonarSTDeluxeDriver
//
//  Created by Gene Chen on 2015-01-29.
//  Copyright (c) 2015 Gene Chen. All rights reserved.
//

#ifndef __XonarSTDeluxeDriver__XonarIO__
#define __XonarSTDeluxeDriver__XonarIO__

extern void pcm1796_set_left_volume(xonar_info* dev, int left);
extern void pcm1796_set_right_volume(xonar_info* dev, int right);
extern void pcm1796_set_volume(xonar_info* dev, int left, int right);
extern void pcm1796_set_mute(xonar_info* dev, int mute);

extern void cmi8788_set_output(xonar_info* dev, int which);
extern void cmi8788_toggle_sound(xonar_info* dev, int output);

extern void cmi8788_write_4(xonar_info* dev, int reg, u_int32_t data);
extern void cmi8788_write_2(xonar_info* dev, int reg, u_int16_t data);
extern void cmi8788_write_1(xonar_info* dev, int reg, u_int8_t data);

extern UInt32 cmi8788_read_4(xonar_info* dev, int reg);
extern UInt16 cmi8788_read_2(xonar_info* dev, int reg);
extern UInt8 cmi8788_read_1(xonar_info* dev, int reg);

extern int pcm1796_write_i2c(xonar_info* dev, uint8_t codec_num, uint8_t reg, uint8_t data);
extern int pcm1796_write(xonar_info* dev, uint8_t codec_num, uint8_t reg, uint8_t data);

#endif /* defined(__XonarSTDeluxeDriver__XonarIO__) */
