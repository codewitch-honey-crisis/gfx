#### [← Back to index](index.md)

<a name="7"></a>

# 7. Streams

A stream is an abstraction used for reading and writing data to and from some kind of source - usually sequentially. Under the STL this is handled by the iostream classes, but the STL isn't always available or conformant on platforms where GFX will run. GFX therefore uses its own stream classes. Technically, streams are actually part of the [htcw_io](https://github.com/codewitch-honey-crisis/htcw_io) library that GFX relies on, but its types are imported into the GFX namespace since they are such an intrinsic part of using GFX.

Streams can be read with `read()`, written to with `write()` and seeked with `seek()` assuming the `caps()` support it. If a stream is opened for reading, `caps()` should be checked to determine that the stream can actually be read. If it can't, it was not opened successfully. There are several types of streams depending on platform availability for reading and writing to different sources. These include `file_stream` (when available), `buffer_stream`, `const_buffer_stream` and sometimes `arduino_stream` although you can make your own.

Typically, methods accept a *pointer* to a stream rather than the stream itself due to them being pure virtual. Some methods will require a stream be seekable, while others have no such requirement. The `arduino_stream` is not seekable for example, but `file_stream` is.

Different platforms initialize the file stream differently. For example, when available, `fopen()` will be used and the path and access mode can be passed in to the constructor. Under Arduino, a `File` object must be passed in.

The following is an example of streaming a truetype font off an SD card under Arduino, using a `file_stream`:
```cpp
File file = SD.open("/Maziro.ttf");
file_stream fs(file);
open_font::open(&fs,&f);
// draw stuff here
...
// free the font
fs.close();
```


[→ Performance](performance.md)

[← Positioning](positioning.md)

