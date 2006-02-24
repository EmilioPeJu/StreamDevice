/***************************************************************
* StreamDevice Support                                         *
*                                                              *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)          *
* (C) 2005 Dirk Zimoch (dirk.zimoch@psi.ch)                    *
*                                                              *
* This is the format converter base and includes the standard  *
* format converters for StreamDevice.                          *
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

#include <stdio.h>
#include "StreamFormatConverter.h"
#include "StreamFormat.h"
#include "StreamError.h"

StreamFormatConverter* StreamFormatConverter::
registered [256];

#define esc (0x1b)

void StreamFormatConverter::
provides(const char* provided)
{
    const unsigned char* p;
    for (p = reinterpret_cast<const unsigned char*>(provided);
        *p; p++)
    {
        registered[*p] = this;
    }
}

int StreamFormatConverter::
print(const StreamFormat&, StreamBuffer&, long)
{
    error("Unimplemented long value print method\n");
    return false;
}

int StreamFormatConverter::
print(const StreamFormat&, StreamBuffer&, double)
{
    error("Unimplemented double value print method\n");
    return false;
}

int StreamFormatConverter::
print(const StreamFormat&, StreamBuffer&, const char*)
{
    error("Unimplemented string value print method\n");
    return false;
}

int StreamFormatConverter::
print(const StreamFormat&, StreamBuffer&)
{
    error("Unimplemented pseudo value print method\n");
    return false;
}

int StreamFormatConverter::
scan(const StreamFormat&, const char*, long&)
{
    error("Unimplemented long value scan method\n");
    return -1;
}

int StreamFormatConverter::
scan(const StreamFormat&, const char*, double&)
{
    error("Unimplemented double value scan method\n");
    return -1;
}

int StreamFormatConverter::
scan(const StreamFormat&, const char*, char*, size_t)
{
    error("Unimplemented string value scan method\n");
    return -1;
}

int StreamFormatConverter::
scan(const StreamFormat&, StreamBuffer&, long&)
{
    error("Unimplemented pseudo value scan method\n");
    return -1;
}

static void copyFormatString(StreamBuffer& info, const char* source)
{
    const char* p = source - 1;
    while (*p != '%' && *p != ')') p--;
    info.append('%');
    while (++p != source-1) info.append(*p);
}

// Standard Long Converter for 'diouxX'

class StreamStdLongConverter : public StreamFormatConverter
{
    int parse(const StreamFormat&, StreamBuffer&, const char*&, bool);
    int print(const StreamFormat&, StreamBuffer&, long);
    int scan(const StreamFormat&, const char*, long&);
};

int StreamStdLongConverter::
parse(const StreamFormat& format, StreamBuffer& info,
    const char*& source, bool scanFormat)
{
    if (scanFormat && (format.flags & alt_flag)) return false;
    copyFormatString(info, source);
    info.append('l');
    info.append(format.conv);
    if (scanFormat) info.append("%n");
    return long_format;
}

int StreamStdLongConverter::
print(const StreamFormat& format, StreamBuffer& output, long value)
{
    output.printf(format.info(), value);
    return true;
}

int StreamStdLongConverter::
scan(const StreamFormat& format, const char* input, long& value)
{
    int length;
    if (format.flags & skip_flag)
    {
        if (sscanf(input, format.info(), &length) < 0) return -1;
	if (length < format.width) return -1;
    }
    else
    {
        if (sscanf(input, format.info(), &value, &length) < 1) return -1;
    }
    return length;
}

RegisterConverter (StreamStdLongConverter, "diouxX");

// Standard Double Converter for 'feEgG'

class StreamStdDoubleConverter : public StreamFormatConverter
{
    int parse(const StreamFormat&, StreamBuffer&, const char*&, bool);
    int print(const StreamFormat&, StreamBuffer&, double);
    int scan(const StreamFormat&, const char*, double&);
};

int StreamStdDoubleConverter::
parse(const StreamFormat& format, StreamBuffer& info,
    const char*& source, bool scanFormat)
{
    if (scanFormat && (format.flags & alt_flag))
    {
        error("Use of modifier '#' not allowed with %%%c input conversion\n",
            format.conv);
        return false;
    }
    copyFormatString(info, source);
    if (scanFormat) info.append('l');
    info.append(format.conv);
    if (scanFormat) info.append("%n");
    return double_format;
}

int StreamStdDoubleConverter::
print(const StreamFormat& format, StreamBuffer& output, double value)
{
    output.printf(format.info(), value);
    return true;
}

int StreamStdDoubleConverter::
scan(const StreamFormat& format, const char* input, double& value)
{
    int length;
    if (format.flags & skip_flag)
    {
        if (sscanf(input, format.info(), &length) < 0) return -1;
	if (length < format.width) return -1;
    }
    else
    {
        if (sscanf(input, format.info(), &value, &length) < 1) return -1;
    }
    return length;
}

RegisterConverter (StreamStdDoubleConverter, "feEgG");

// Standard String Converter for 's'

class StreamStdStringConverter : public StreamFormatConverter
{
    int parse(const StreamFormat&, StreamBuffer&, const char*&, bool);
    int print(const StreamFormat&, StreamBuffer&, const char*);
    int scan(const StreamFormat&, const char*, char*, size_t);
};

int StreamStdStringConverter::
parse(const StreamFormat& format, StreamBuffer& info,
    const char*& source, bool scanFormat)
{
    if (format.flags & (sign_flag|space_flag|zero_flag|alt_flag))
    {
        error("Use of modifiers '+', ' ', '0', '#' "
            "not allowed with %%%c conversion\n",
            format.conv);
        return false;
    }
    copyFormatString(info, source);
    info.append(format.conv);
    if (scanFormat) info.append("%n");
    return string_format;
}

int StreamStdStringConverter::
print(const StreamFormat& format, StreamBuffer& output, const char* value)
{
    output.printf(format.info(), value);
    return true;
}

int StreamStdStringConverter::
scan(const StreamFormat& format, const char* input,
    char* value, size_t maxlen)
{
    int length = 0;
    if (*input == '\0')
    {
        // match empty string
        value[0] = '\0';
        return 0;
    }
    if (format.flags & skip_flag)
    {
        if (sscanf(input, format.info(), &length) < 0) return -1;
	/* For a skip, sscanf will return 0 conversions, so check the length */
	if (length < format.width) return -1;
    }
    else
    {
        char tmpformat[10];
        const char* fmt;
        if (maxlen <= format.width || format.width == 0)
        {
            // assure not to read too much
            sprintf(tmpformat, "%%%d%c%%n", maxlen-1, format.conv);
            fmt = tmpformat;
        }
        else
        {
            fmt = format.info();
        }
        if (sscanf(input, fmt, value, &length) < 1) return -1;
        value[length] = '\0';
        debug("StreamStdStringConverter::scan: length=%d, value=%s\n",
            length, value);
    }
    return length;
}

RegisterConverter (StreamStdStringConverter, "s");

// Standard Characters Converter for 'c'

class StreamStdCharsConverter : public StreamStdStringConverter
{
    int parse(const StreamFormat&, StreamBuffer&, const char*&, bool);
    int print(const StreamFormat&, StreamBuffer&, long);
    // scan is inherited from %s format
};

int StreamStdCharsConverter::
parse(const StreamFormat& format, StreamBuffer& info,
    const char*& source, bool scanFormat)
{
    if (format.flags & (sign_flag|space_flag|zero_flag|alt_flag))
    {
        error("Use of modifiers '+', ' ', '0', '#' "
            "not allowed with %%%c conversion\n",
            format.conv);
        return false;
    }
    copyFormatString(info, source);
    info.append(format.conv);
    if (scanFormat)
    {   info.append("%n");
        return string_format;
    }
    return long_format;
}

int StreamStdCharsConverter::
print(const StreamFormat& format, StreamBuffer& output, long value)
{
    output.printf(format.info(), value);
    return true;
}

RegisterConverter (StreamStdCharsConverter, "c");

// Standard Charset Converter for '[' (input only)

class StreamStdCharsetConverter : public StreamFormatConverter
{
    int parse(const StreamFormat&, StreamBuffer&, const char*&, bool);
    int scan(const StreamFormat&, const char*, char*, size_t);
};

int StreamStdCharsetConverter::
parse(const StreamFormat& format, StreamBuffer& info,
    const char*& source, bool scanFormat)
{
    if (!scanFormat)
    {
        error("Format conversion %%[ is only allowed in input formats\n");
        return false;
    }
    if (format.flags & (left_flag|sign_flag|space_flag|zero_flag|alt_flag))
    {
        error("Use of modifiers '-', '+', ' ', '0', '#'"
            "not allowed with %%%c conversion\n",
            format.conv);
        return false;
    }
    info.printf("%%%d[", format.width);
    while (*source || *source != ']')
    {
        if (*source == esc) source++;
        info.append(*source++);
    }
    if (!*source) {
        error("Missing ']' after %%[ format conversion\n");
        return false;
    }
    info.append("]%n");
    return string_format;
}

int StreamStdCharsetConverter::
scan(const StreamFormat& format, const char* input,
    char* value, size_t maxlen)
{
    size_t length;
    if (format.flags & skip_flag)
    {
        if (sscanf (input, format.info(), &length) < 0) return -1;
	if (length < format.width) return -1;
    }
    else
    {
        char tmpformat[256];
        const char* fmt;
        if (maxlen <= format.width || format.width == 0)
        {
            char *p = strchr (format.info(), '[');
            // assure not to read too much
            sprintf(tmpformat, "%%%d%s%%n", maxlen-1, p);
            fmt = tmpformat;
        }
        else
        {
            fmt = format.info();
        }
        if (sscanf(input, fmt, value, &length) < 1) return -1;
        value[length] = '\0';
        debug("StreamStdCharsetConverter::scan: length=%d, value=%s\n",
            length, value);
    }
    return length;
}

RegisterConverter (StreamStdCharsetConverter, "[");
