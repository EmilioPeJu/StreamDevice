/***************************************************************
* StreamDevice Support                                         *
*                                                              *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)          *
* (C) 2005 Dirk Zimoch (dirk.zimoch@psi.ch)                    *
*                                                              *
* This is the raw format converter of StreamDevice.            *
* Please refer to the HTML files in ../doc/ for a detailed     *
* documentation.                                               *
*                                                              *
* If you do any changes in this file, you are not allowed to   *
* redistribute it any more. If there is a bug or a missing     *
* feature, send me an email and/or your patch. If I accept     *
* your changes, they will go to the next release.              *
*                                                              *
* DISCLAIMER: If this software breaks something or harms       *
* someone, it's your problem.                                  *
*                                                              *
***************************************************************/

#include "StreamFormatConverter.h"
#include "StreamError.h"

// Raw Bytes Converter %r

class StreamRawConverter : public StreamFormatConverter
{
    int parse(const StreamFormat&, StreamBuffer&, const char*&, bool);
    int print(const StreamFormat&, StreamBuffer&, long);
    int scan(const StreamFormat&, const char*, long&);
};

int StreamRawConverter::
parse(const StreamFormat& format, StreamBuffer& info,
    const char*& source, bool scanFormat)
{
    return long_format;
}

int StreamRawConverter::
print(const StreamFormat& format, StreamBuffer& output, long value)
{
    int prec = format.prec;   // number of bytes from value
    if (prec == -1) prec = 1; // default: 1 byte
    int width = prec;         // number of bytes in output
    if (format.width > width) width = format.width;
    char byte = 0;
    if (format.flags & alt_flag) // lsb first (little endian)
    {
        while (prec--)
        {
            byte = value & 0xFF;
            output.append(byte);
            value >>= 8;
            width--;
        }
        byte = (byte & 0x80) ? 0xFF : 0x00; // fill with sign
        while (width--)
        {
            output.append(byte);
        }
    }
    else // msb first (big endian)
    {
        byte = ((value >> (8 * (prec-1))) & 0x80) ? 0xFF : 0x00;
        while (width > prec) // fill with sign
        {
            output.append(byte);
            width--;
        }
        while (prec--)
        {
            output.append((value >> (8 * prec)) & 0xFF);
        }
    }
    return true;
}

int StreamRawConverter::
scan(const StreamFormat& format, const char* input, long& value)
{
    long length = 0;
    long val = 0;
    int width = format.width;
    if (width == 0) width = 1; // default: 1 byte
    if (format.flags & skip_flag)
    {
        return width; // just skip input
    }
    if (format.flags & alt_flag)
    {
        // little endian (sign extended)*/
        unsigned int shift = 0;
        while (--width && shift < sizeof(long)*8)
        {
            val |= ((unsigned char) input[length++]) << shift;
            shift += 8;
        }
        if (width == 0)
        {
            val |= ((signed char) input[length++]) << shift;
        }
        length += width; // ignore upper bytes not fitting in long
    }
    else
    {
        // big endian (sign extended)*/
        val = (signed char) input[length++];
        while (--width)
        {
            val <<= 8;
            val |= (unsigned char) input[length++];
        }
    }
    value = val;
    return length;
}

RegisterConverter (StreamRawConverter, "r");
