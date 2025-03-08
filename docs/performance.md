#### [← Back to index](index.md)

<a name="8"></a>

# 8. Performance

GFX makes a reasonable effort to use your devices efficiently, but it works best when you understand the ramifications of what you are doing with it. In many cases, GFX will choose the most efficient way to complete the operation you gave it, but in some cases you can get better performance out of it by giving it some help. Below we'll cover the various mechanisms GFX uses to push pixels around. They are listed in order of preference, except for the final entry - asynchronous drawing, which requires special consideration and must be invoked explicitly.

<a name="8.1"></a>

## 8.1 Blting

To blt means to do a direct copy of pixel data from one memory buffer to another. This is the fastest way to transfer data synchronously, and the most efficient way to do so overall. When available, GFX prefers blting. In order for blting to be the fastest possible, pixels should be byte aligned, such as `rgb_pixel<16>` but not `rgb_pixel<18>` and/or the entire source should be blted instead of part of it. No flipping, resizing, or pixel conversion can take place during a blt, since it is a raw memory transfer.

<a name="8.2"></a>

## 8.2 Copying

To copy means to transfer a rectangular window of data from one source to the other. Copying should use the most efficient means available to do the transfer based on the arguments. This means taking advantage of draw target features like batching. Copying asynchronously using DMA *can be* but is not necessarily the most efficient way to transfer data when an SPI bus or other DMA capable bus is involved. In order for a DMA transfer to occur no flipping, cropping, resizing or pixel conversion can occur, so the entire draw source must be copied to the destination.

<a name="8.3"></a>

## 8.3 Batching

With most display drivers, it is dramatically more efficient to set an "address window" which is a rectangular region, and then write pixels out to that region left to right, top to bottom until the region is full rather than sending the coordinates for each pixel individually. GFX refers to this as "batching" and on supported draw targets it can dramatically reduce bus traffic and therefore increase performance. Batching isn't necessary for targets who hold a frame buffer locally in RAM, but when the frame buffer exists externally over a bus batching is a very effective way to increase pixel throughput. You don't typically use batching directly. GFX will use it when possible - that is, any time a rectangular region can be written in full - and faster means, such as blting and copying aren't available. It should be noted that copying itself may use batching to fulfill the copy. You can use it explicitly by calling `draw::batch`.

<a name="8.4"></a>

## 8.4 RLE Transmission

In many cases, when drawing non-rectangular regions - including raster fonts with transparent backgrounds, polygons and sprites - GFX will attempt to run-length encode horizontal runs of pixels of the same color and send them out as a unit to reduce bus traffic. This happens automatically during drawing operations.

<a name="8.5"></a>

## 8.5 Asynchronous drawing

In GFX, asynchronicity is kept as simple to use as possible. While it can be much simpler than doing asynchronous operations in other libraries, it still requires the same sort of considerations in order to get any benefit out of it.

Asynchronicity is entirely dependent on the capabilities of the draw target. In practice, this usually means that only devices operating over SPI on supported platforms will actually complete an operation asynchronously. When those conditions are not met, a synchronous operation occurs instead. That way, you can call `_async` methods safely even when the hardware doesn't support it - you simply won't get the benefits.

Due to GFX simplifying asynchronous calls, and due to the different characteristics of different platforms, asynchronous behavior isn't entirely defined by GFX itself. Instead the details are often dependent on the platform. For example, under Arduino, only one asynchronous operation may be "in the air" at once per draw target on an ESP32, but under the ESP-IDF several can be pending at once, chaining one after the other. Once the maximum number of pending operations is reached further attempts will block until there is a free spot in the asynchronous queue. You can wait for all pending operations for a draw target using `draw::wait_all_async(target)`.

The primary method to benefit from this feature is `draw::bitmap_async<>()`. The other methods are provided for a couple of reasons. First, in the future it may be possible to make the other operations much more asynchronous, pending future hardware. Second, on some platforms, like the ESP-IDF switching from an asynchronous operation to a synchronous operation requires the framework to wait for all pending asynchronous operations to complete before the synchronous operation can be started. Therefore, to keep things asynchronous, it may be desirable to continue asynchronous draws so you don't end up blocking during a DMA transfer.

For your DMA transfer to be beneficial you need the amount of data you are transferring to be significant enough that it overcomes the extra upfront overhead of transferring it using DMA. Asynchronously transferring a 320x16@16-bit buffer will get you better results than a 16x16@16-bit one. As before, you cannot crop, convert, flip, or resize if you want full asynchronicity. Transfer the entire draw source to the destination unchanged. Furthermore, with most MCUs, the buffer must be in DMA capable RAM or the transfer will fail.

The other way to easily take advantage of asynchronicity is with `draw::batch_async<>()` which can do DMA transfers as you draw. This is really simple to do, as it's all automatically handled when you do asynchronous user level batching.


[→ Tools](tools.md)

[← Streams](streams.md)

