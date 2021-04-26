#!/usr/bin/python

# mkwinfont is copyright 2001 Simon Tatham. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import sys
import string
import struct

# Generate Windows bitmap font files from a text description.

bytestruct = struct.Struct("B")
wordstruct = struct.Struct("<H")
dwordstruct = struct.Struct("<L")

def byte(i):
    return bytestruct.pack(i & 0xFF)
def word(i):
    return wordstruct.pack(i & 0xFFFF)
def dword(i):
    return dwordstruct.pack(i & 0xFFFFFFFF)

def frombyte(s):
    return bytestruct.unpack_from(s, 0)[0]
def fromword(s):
    return wordstruct.unpack_from(s, 0)[0]
def fromdword(s):
    return dwordstruct.unpack_from(s, 0)[0]

def asciz(s):
    for length in range(len(s)):
        if frombyte(s[length:length+1]) == 0:
            break
    return s[:length].decode("ASCII")

class font:
    pass

class char:
    pass

def loadfont(file):
    "Load a font description from a text file."
    f = font()
    f.copyright = f.facename = f.height = f.ascent = None
    f.italic = f.underline = f.strikeout = 0
    f.weight = 400
    f.charset = 0
    f.pointsize = None
    f.chars = [None] * 256

    fp = open(file, "r")
    lineno = 0
    while 1:
        s = fp.readline()
        if s == "":
            break
        lineno = lineno + 1
        while s[-1:] == "\n" or s[-1:] == "\r":
            s = s[:-1]
        while s[0:1] == " ":
            s = s[1:]
        if s == "" or s[0:1] == "#":
            continue
        space = s.find(" ")
        if space == -1:
            space = len(s)
        w = s[:space]
        a = s[space+1:]
        if w == "copyright":
            if len(a) > 59:
                sys.stderr.write("Copyright too long\n")
                return None
            f.copyright = a
            continue
        if w == "height":
            f.height = int(a)
            continue
        if w == "facename":
            f.facename = a
            continue
        if w == "ascent":
            f.ascent = int(a)
            continue
        if w == "pointsize":
            f.pointsize = int(a)
            continue
        if w == "weight":
            f.weight = int(a)
            continue
        if w == "charset":
            f.charset = int(a)
            continue
        if w == "italic":
            f.italic = a == "yes"
            continue
        if w == "underline":
            f.underline = a == "yes"
            continue
        if w == "strikeout":
            f.strikeout = a == "yes"
            continue
        if w == "char":
            c = int(a)
            y = 0
            f.chars[c] = char()
            f.chars[c].width = 0
            f.chars[c].data = [0] * f.height
            continue
        if w == "width":
            f.chars[c].width = int(a)
            continue
        try:
            value = int(w, 2)
            bits = len(w)
            if bits < f.chars[c].width:
                value = value << (f.chars[c].width - bits)
            elif bits > f.chars[c].width:
                value = value >> (bits - f.chars[c].width)
            f.chars[c].data[y] = value
            y = y + 1
        except ValueError:
            sys.stderr.write("Unknown keyword "+w+" at line "+"%d"%lineno+"\n")
            return None

    if f.copyright == None:
        sys.stderr.write("No font copyright specified\n")
        return None
    if f.height == None:
        sys.stderr.write("No font height specified\n")
        return None
    if f.ascent == None:
        sys.stderr.write("No font ascent specified\n")
        return None
    if f.facename == None:
        sys.stderr.write("No font face name specified\n")
        return None
    for i in range(256):
        if f.chars[i] == None:
            sys.stderr.write("No character at position " + "%d"%i + "\n")
            return None
    if f.pointsize == None:
        f.pointsize = f.height
    return f

def fnt(font):
    "Generate the contents of a .FNT file, given a font description."

    # Average width is defined by Windows to be the width of 'X'.
    avgwidth = font.chars[ord('X')].width
    # Max width we calculate from the font. During this loop we also
    # check if the font is fixed-pitch.
    maxwidth = 0
    fixed = 1
    for i in range(0,256):
        if avgwidth != font.chars[i].width:
            fixed = 0
        if maxwidth < font.chars[i].width:
            maxwidth = font.chars[i].width
    # Work out how many 8-pixel wide columns we need to represent a char.
    widthbytes = (maxwidth+7) // 8
    widthbytes = (widthbytes+1) &~ 1  # round up to multiple of 2
    # widthbytes = 3 # FIXME!

    file = b''
    file = file + word(0x0300) # file version
    file = file + dword(0)     # file size (come back and fix later)
    copyright = font.copyright + ("\0" * 60)
    copyright = copyright[0:60]
    file = file + copyright.encode('ASCII')
    file = file + word(0)      # font type (raster, bits in file)
    file = file + word(font.pointsize) # nominal point size
    file = file + word(100)    # vertical resolution
    file = file + word(100)    # horizontal resolution
    file = file + word(font.ascent)   # top of font <--> baseline
    file = file + word(0)      # internal leading
    file = file + word(0)      # external leading
    file = file + byte(font.italic)
    file = file + byte(font.underline)
    file = file + byte(font.strikeout)
    file = file + word(font.weight)   # 1 to 1000; 400 is normal.
    file = file + byte(font.charset)
    if fixed:
        pixwidth = avgwidth
    else:
        pixwidth = 0
    file = file + word(pixwidth) # width, or 0 if var-width
    file = file + word(font.height) # height
    if fixed:
        pitchfamily = 0
    else:
        pitchfamily = 1
    file = file + byte(pitchfamily) # pitch and family
    file = file + word(avgwidth)
    file = file + word(maxwidth)
    file = file + byte(0)          # first char
    file = file + byte(255)        # last char
    file = file + byte(0)          # default char
    file = file + byte(32)         # break char
    file = file + word(widthbytes) # dfWidthBytes
    file = file + dword(0)         # device
    file = file + dword(0)         # face name
    file = file + dword(0)         # BitsPointer (used at load time)
    file = file + dword(0)         # pointer to bitmap data
    file = file + byte(0)          # reserved
    if fixed:
        dfFlags = 1
    else:
        dfFlags = 2
    file = file + dword(dfFlags)   # dfFlags
    file = file + word(0) + word(0) + word(0) # Aspace, Bspace, Cspace
    file = file + dword(0)         # colour pointer
    file = file + (b'\0' * 16)     # dfReserved1

    # Now the char table.
    offset_chartbl = len(file)
    offset_bitmaps = offset_chartbl + 257 * 6
    # Fix up the offset-to-bitmaps at 0x71.
    file = file[:0x71] + dword(offset_bitmaps) + file[0x71+4:]
    bitmaps = b''
    for i in range(0,257):
        if i < 256:
            width = font.chars[i].width
        else:
            width = avgwidth
        file = file + word(width)
        file = file + dword(offset_bitmaps + len(bitmaps))
        for j in range(widthbytes):
            for k in range(font.height):
                if i < 256:
                    chardata = font.chars[i].data[k]
                else:
                    chardata = 0
                chardata = chardata << (8*widthbytes - width)
                bitmaps = bitmaps + byte(chardata >> (8*(widthbytes-j-1)))

    file = file + bitmaps
    # Now the face name. Fix up the face name offset at 0x69.
    file = file[:0x69] + dword(len(file)) + file[0x69+4:]
    file = file + font.facename.encode('ASCII') + b'\0'
    # And finally fix up the file size at 0x2.
    file = file[:0x2] + dword(len(file)) + file[0x2+4:]

    # Done.
    return file

def fnt_facename(f):
    "Return the face name of a font, given the contents of the .FNT file."
    face = fromdword(f[0x69:])
    return asciz(f[face:])

def direntry(f):
    "Return the FONTDIRENTRY, given the data in a .FNT file."
    device = fromdword(f[0x65:])
    face = fromdword(f[0x69:])
    if device == 0:
        devname = ""
    else:
        devname = asciz(f[device:])
    facename = asciz(f[face:])
    return (f[0:0x71] +
            devname.encode('ASCII') + b'\0' +
            facename.encode('ASCII') + b'\0')

stubcode = [
  0xBA, 0x0E, 0x00, # mov dx,0xe
  0x0E,             # push cs
  0x1F,             # pop ds
  0xB4, 0x09,       # mov ah,0x9
  0xCD, 0x21,       # int 0x21
  0xB8, 0x01, 0x4C, # mov ax,0x4c01
  0xCD, 0x21        # int 0x21
]
stubmsg = "This is not a program!\r\nFont library created by mkwinfont.\r\n"

def stub():
    "Create a small MZ executable."
    file = b''
    file = file + b'MZ' + word(0) + word(0)
    file = file + word(0) # no relocations
    file = file + word(4) # 4-para header
    file = file + word(0x10) # 16 extra para for stack
    file = file + word(0xFFFF) # maximum extra paras: LOTS
    file = file + word(0) + word(0x100) # SS:SP = 0000:0100
    file = file + word(0) # no checksum
    file = file + word(0) + word(0) # CS:IP = 0:0, start at beginning
    file = file + word(0x40) # reloc table beyond hdr
    file = file + word(0) # overlay number
    file = file + 4 * word(0) # reserved
    file = file + word(0) + word(0) # OEM id and OEM info
    file = file + 10 * word(0) # reserved
    file = file + dword(0) # offset to NE header
    assert len(file) == 0x40
    for i in stubcode: file = file + byte(i)
    file = file + (stubmsg + "$").encode('ASCII')
    n = len(file)
    pages = (n+511) // 512
    lastpage = n - (pages-1) * 512
    file = file[:2] + word(lastpage) + word(pages) + file[6:]
    # Now assume there will be a NE header. Create it and fix up the
    # offset to it.
    while len(file) % 16: file = file + b'\0'
    file = file[:0x3C] + dword(len(file)) + file[0x40:]
    return file

def fon(name, fonts):
    "Create a .FON font library, given a bunch of .FNT file contents."

    # Construct the FONTDIR.
    fontdir = word(len(fonts))
    for i in range(len(fonts)):
        fontdir = fontdir + word(i+1)
        fontdir = fontdir + direntry(fonts[i])

    # The MZ stub.
    stubdata = stub()
    # Non-resident name table should contain a FONTRES line.
    nonres = "FONTRES 100,96,96 : " + name
    nonres = byte(len(nonres)) + nonres.encode('ASCII') + b'\0\0\0'
    # Resident name table should just contain a module name.
    mname = ""
    for c in name:
        if c in "0123546789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz":
            mname = mname + c
    res = byte(len(mname)) + mname.encode('ASCII') + b'\0\0\0'
    # Entry table / imported names table should contain a zero word.
    entry = word(0)

    # Compute length of resource table.
    # 12 (2 for the shift count, plus 2 for end-of-table, plus 8 for the
    #    "FONTDIR" resource name), plus
    # 20 for FONTDIR (TYPEINFO and NAMEINFO), plus
    # 8 for font entry TYPEINFO, plus
    # 12 for each font's NAMEINFO 

    # Resources are currently one FONTDIR plus n fonts.
    # TODO: a VERSIONINFO resource would be nice too.
    resrcsize = 12 + 20 + 8 + 12 * len(fonts)
    resrcpad = ((resrcsize + 15) &~ 15) - resrcsize

    # Now position all of this after the NE header.
    p = 0x40        # NE header size
    off_segtable = off_restable = p
    p = p + resrcsize + resrcpad
    off_res = p
    p = p + len(res)
    off_modref = off_import = off_entry = p
    p = p + len(entry)
    off_nonres = p
    p = p + len(nonres)

    pad = ((p+15) &~ 15) - p
    p = p + pad
    q = p + len(stubdata)

    # Now q is file offset where the real resources begin. So we can
    # construct the actual resource table, and the resource data too.
    restable = word(4) # shift count
    resdata = b''
    # The FONTDIR resource.
    restable = restable + word(0x8007) + word(1) + dword(0)
    restable = restable + word((q+len(resdata)) >> 4)
    start = len(resdata)
    resdata = resdata + fontdir
    while len(resdata) % 16: resdata = resdata + b'\0'
    restable = restable + word((len(resdata)-start) >> 4)
    restable = restable + word(0x0C50) + word(resrcsize-8) + dword(0)
    # The font resources.
    restable = restable + word(0x8008) + word(len(fonts)) + dword(0)
    for i in range(len(fonts)):
        restable = restable + word((q+len(resdata)) >> 4)
        start = len(resdata)
        resdata = resdata + fonts[i]
        while len(resdata) % 16: resdata = resdata + b'\0'
        restable = restable + word((len(resdata)-start) >> 4)
        restable = restable + word(0x1C30) + word(0x8001 + i) + dword(0)
    # The zero word.
    restable = restable + word(0)
    assert len(restable) == resrcsize - 8
    restable = restable + b'\007FONTDIR'
    restable = restable + b'\0' * resrcpad

    file = stubdata + b'NE' + byte(5) + byte(10)
    file = file + word(off_entry) + word(len(entry))
    file = file + dword(0) # no CRC
    file = file + word(0x8308) # the Mysterious Flags
    file = file + word(0) + word(0) + word(0) # no autodata, no heap, no stk
    file = file + dword(0) + dword(0) # CS:IP == SS:SP == 0
    file = file + word(0) + word(0) # segment table len, modreftable len
    file = file + word(len(nonres))
    file = file + word(off_segtable) + word(off_restable)
    file = file + word(off_res) + word(off_modref) + word(off_import)
    file = file + dword(len(stubdata) + off_nonres)
    file = file + word(0) # no movable entries
    file = file + word(4) # seg align shift count
    file = file + word(0) # no resource segments
    file = file + byte(2) + byte(8) # target OS and more Mysterious Flags
    file = file + word(0) + word(0) + word(0) + word(0x300)

    # Now add in all the other stuff.
    file = file + restable + res + entry + nonres + b'\0' * pad + resdata

    return file

def main():
    outfile = None
    facename = None
    fntload = 0
    fonmode = 1
    infiles = []
    a = sys.argv[1:]
    options = 1
    if len(a) == 0:
        print ("usage: mkwinfont [-fnt | -fon] [-o outfile]"
               " [-facename name] files")
        sys.exit(0)
    while len(a) > 0:
        if a[0] == "--":
            options = 0
            a = a[1:]
        elif options and a[0][0:1] == "-":
            if a[0] == "-o":
                try:
                    outfile = a[1]
                    a = a[2:]
                except IndexError:
                    sys.stderr.write("option -o requires an argument\n")
                    sys.exit(1)
            elif a[0] == "-facename":
                try:
                    facename = a[1]
                    a = a[2:]
                except IndexError:
                    sys.stderr.write("option -facename requires an argument\n")
                    sys.exit(1)
            elif a[0] == "-fnt":
                fonmode = 0
                a = a[1:]
            elif a[0] == "-fon":
                fonmode = 1
                a = a[1:]
            elif a[0] == "-fnt2fon":
                fonmode = 1
                fntload = 1
                a = a[1:]
            else:
                sys.stderr.write("ignoring unrecognised option "+a[0]+"\n")
                a = a[1:]
        else:
            infiles = infiles + [a[0]]
            a = a[1:]

    if len(infiles) < 0:
        sys.stderr.write("no input files specified\n")
        sys.exit(1)

    if outfile == None:
        sys.stderr.write("no output file specified\n")
        sys.exit(1)

    if fonmode == 0 and len(infiles) > 1:
        sys.stderr.write("FNT mode can only process one font\n")
        sys.exit(1)

    fnts = []
    fds = []
    for fname in infiles:
        if fntload:
            with open(fname) as fp:
                fntdata = fp.read()
            f = font()
            f.facename = fnt_facename(fntdata)
            fds.append(f)
            fnts.append(fntdata)
        else:
            f = loadfont(fname)
            if f == None:
                sys.stderr.write("unable to load font description "+fname+"\n")
                sys.exit(1)
            if facename != None:
                f.facename = facename
            fds = fds + [f]
            fnts = fnts + [fnt(f)]

    if fonmode == 0:
        outfp = open(outfile, "wb")
        outfp.write(fnts[0])
        outfp.close()
    else:
        # If all supplied fonts have the same face name, use that.
        # Otherwise, require that one be input.
        autoname = fds[0].facename
        for f in fds[1:]:
            if autoname != f.facename:
                autoname = None
        if facename == None:
            facename = autoname
        if facename == None:
            sys.stderr.write("fonts disagree on face name; "+\
            "specify one with -facename\n")
            sys.exit(1)
        outfp = open(outfile, "wb")
        outfp.write(fon(facename, fnts))
        outfp.close()

if __name__ == "__main__":
    main()
