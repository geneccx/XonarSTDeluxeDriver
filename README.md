# XonarSTDeluxeDriver
A very hacky port of the Xonar Essence ST/STX driver for Linux to macOS.
See: https://github.com/polachok/xonar-freebsd

**Working:**
- Volume/mute controls
- Select output channel (speakers and rear HP)
- Audio playback

**Known Issues:**
- Only 2 output channels
- Output format 192KHz doesn't work
- No support for input devices yet
- Device fails to wake on sleep. Probably need to implement power management
- The DMA buffer is a giant contiguous chunk of memory. I think this makes the latency pretty bad

**Troubleshooting:**  
If you only hear static output when trying to play sound:  
Try using a different system definition  

Working definitions:
- `iMac 14,2`
- `MacPro6,1`

NOT working definitions:
- `iMac17,1`

If the audio sounds sped up/slowed down:  
Try setting a different output format in "Audio Midi Setup.app"
