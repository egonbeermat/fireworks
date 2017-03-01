# fireworks for TinyScreen+

Small sketch displaying firework-like particles on TinyScreen+

Requires FastLED library, available here: https://github.com/FastLED/FastLED/releases

Version: 1.0

Date: 02-27-2017

Devices Supported:
* TinyScreen+

Quick overview:
A double fullscreen buffer is used in this sketch because, well, we've got the RAM for it :) Drawing to a buffer and transferring the buffer to the screen, vs. writing directly to the screen, improves performance and allows for effects to be done against the buffer (such as dimming existing pixels to create 'trails'). Even using a buffer to improve performance and remove flicker, the main bottleneck is now in the buffer transfer - around 4500 micros for fullscreen 8 bit color and 9000 micros for fullscreen 16 bit color. Using DMA to transfer the buffer frees up the processor to continue execution, so a second buffer can be drawn to whilst the first is in transit. Even so, 30-50% of the code execution time is still spent waiting for the previous DMA buffer transfer to complete. 

Using fast but approximate sin8 and cos8 libraries from FastLED improves execution speed significantly over standard sin and cos libraries, without any noticable visual difference for this use case.

Finally, the buffer in transit is copied to the alternative buffer, to preserve the drawing data, and memcpy from Daniel Vik improved performance ~8 fold for this operation, vs. the standard library.

Depending on the number of particles chosen, the sketch achieves a frame rate of around 120fps for 16 bit color and 240fps for 8 bit color!