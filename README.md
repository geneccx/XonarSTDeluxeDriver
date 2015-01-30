# XonarSTDeluxeDriver
A very hacky port of the Xonar Essence ST driver for Linux to OSX.
See: https://github.com/polachok/xonar-freebsd

Working:
- Volume/mute controls
- Select output channel (speakers and rear HP)
- Audio playback

Known Issues:
- Only 2 output channels
- Output formats 96KHz and 192KHz don't work
- No support for input devices yet
- Device fails to wake on sleep. Probably need to implement power management
- The DMA buffer is a giant contiguous chunk of memory. I think this makes the latency pretty bad
- The volume scaling is non-existent - it's a straight linear value from 0-100 which makes the volume settings in OSX very difficult to use
- CS2000 clock isn't actually being used
