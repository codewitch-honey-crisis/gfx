#### [← Back to index](index.md)

<a name="9"></a>

# 9. Tools

GFX provides a couple of tools for embedding data into your code as a C++ header file. The first one is for arbitrary binary data, like JPGs or raw bitmap data, and the second one is for TTF, OTF and FON fonts.

For the web based version of these tools, [click here](https://honeythecodewitch.com/gfx/generator)

The GFX website also provides a catalog of icons to use in your projects. For that page, [click here](https://honeythecodewitch.com/gfx/iconPack)

<a name="9.1"></a>

## 9.1 Bingen

Bingen generates a header out of binary data. The header contains a flat array with the data stored as program flash.

You use it like
```
bingen <inputfile> > header.hpp
```
Note that on Windows the `#define WINDOWS` directive must be declared. Also on Windows, the console writes files in UTF-16 format, but your compiler won't like that. To fix it, open up the output file in *Notepad*, and then `Save As...` with the encoding set to UTF-8.

The header it generates will have the data contained at `header_bin` where header is the name of your binary file with illegal characters made into underscores. For example, if you used `bingen test.dat > test.hpp`, *test.hpp* would contain `test_dat_bin` which contains a `uint8_t` array.

In addition, a `const_buffer_stream` will be exposed by the name of `test_dat_stream`. This can be used anywhere a read-only, seekable stream is accepted.

<a name="9.2"></a>

## 9.2 Fontgen

Fontgen is similar to bingen, but it makes font instances for you to use instead of arrays and streams. The program works a bit differently for Win 3.1 FON files than it does for TTF and OTF files due to the different nature of each, but the concepts are the same. This turns your font into a header file that can be embedded.

```
fontgen test.fon > test.hpp
```
or
```
fontgen test.ttf > test.hpp
```
When specifying a FON file, you can specify an index into the font collection as the 2nd argument, and a range of characters as the 3rd and 4th arguments to import part of a font file. There is no such facility for Truetype.

Again, there is the caveat that on Windows the `#define WINDOWS` directive must be declared. Also on Windows, the console writes files in UTF-16 format, but your compiler won't like that. Once again, to fix it, open up the output file in *Notepad*, and then `Save As...` with the encoding set to UTF-8.

Once you've included the generated *test.hpp* file you can use it like

```cpp
const font& f = test_fon;
```
or
```cpp
const open_font& f = test_ttf;
```

[← Performance](performance.md)

