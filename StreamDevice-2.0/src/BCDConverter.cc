/***************************************************************
* StreamDevice Support                                         *
*                                                              *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)          *
* (C) 2005 Dirk Zimoch (dirk.zimoch@psi.ch)                    *
*                                                              *
* This is the BCD format converter of StreamDevice.            *
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

// packed BCD (0x00 - 0x99)

class StreamBCDConverter : public StreamFormatConverter
{
    int parse (const StreamFormat&, StreamBuffer&, const char*&, bool);
    int print(const StreamFormat&, StreamBuffer&, long);
    int scan(const StreamFormat&, const char*, long&);
};

int StreamBCDConverter::
parse(const StreamFormat&, StreamBuffer&, const char*&, bool)
{
    return long_format;
}

int StreamBCDConverter::
print(const StreamFormat& format, StreamBuffer& output, long value)
{
    unsigned char bcd[6]={0,0,0,0,0,0}; // sufficient for 2^32
    int i;
    int prec = format.prec; // number of nibbles
    if (prec == -1)
    {
        prec = 2 * sizeof (value);
    }
    int width = (prec + (format.flags & sign_flag ? 2 : 1)) / 2;
    if (format.width > width) width = format.width;
    if (format.flags & sign_flag && value < 0)
    {
        // negative BCD value, I hope "F" as "-" is OK
        bcd[5] = 0xF0;
        value = -value;
    }
    if (prec > 10) prec = 10;
    for (i = 0; i < prec; i++)
    {
        bcd[i/2] |= (value % 10) << (4 * (i & 1));
        value /= 10;
    }
    if (format.flags & alt_flag)
    {
        // least significant byte first (little endian)
        for (i = 0; i < (prec + 1) / 2; i++)
        {
            output.append(bcd[i]);
        }
        for (; i < width; i++)
        {
            output.append('\0');
        }
        output[-1] |=  bcd[5];
    }
    else
    {
        // most significant byte first (big endian)
        int firstbyte = output.length();
        for (i = 0; i < width - (prec + 1) / 2; i++)
        {
            output.append('\0');
        }
        for (i = (prec - 1) / 2; i >= 0; i--)
        {
            output.append(bcd[i]);
        }
        output[firstbyte] |= bcd[5];
    }
    return width;
}

int StreamBCDConverter::
scan(const StreamFormat& format, const char* input, long& value)
{
    int length = 0;
    int val = 0;
    unsigned char bcd1, bcd10;
    int width = format.width;
    if (width == 0) width = 1;
    if (format.flags & alt_flag)
    {
        // little endian
        int shift = 1;
        while (width--)
        {
            bcd1 = bcd10 = (unsigned char) input[length++];
            bcd1 &= 0x0F;
            bcd10 >>= 4;
            if (bcd1 > 9 || shift * bcd1 < bcd1) break;
            if (width == 0 && format.flags & sign_flag)
            {
                val += bcd1 * shift;
                if (bcd10 != 0) val = -val;
                break;
            }
            if (bcd10 > 9 || shift * bcd10 < bcd10) break;
            val += (bcd1 + 10 * bcd10) * shift;
            if (shift <= 100000000) shift *= 100;
            else shift = 0;
        }
    }
    else
    {
        // big endian
        int sign = 1;
        while (width--)
        {
            long temp;
            bcd1 = bcd10 = (unsigned char) input[length];
            bcd1 &= 0x0F;
            bcd10 >>= 4;
            if (length == 0 && format.flags & sign_flag && bcd10)
            {
                sign = -1;
                bcd10 = 0;
            }
            if (bcd1 > 9 || bcd10 > 9) break;
            temp = val * 100 + (bcd1 + 10 * bcd10);
            if (temp < val)
            {
                length = 0;
                break;
            }
            val = temp;
            length++;
        }
        val *= sign;
    }
    if (length == 0) return -1;
    return length;
}

RegisterConverter (StreamBCDConverter, "D");
