/*****************************************************************
* Stream Device Support                                          *
*                                                                *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)            *
*                                                                *
* This is the protocol parser of the Stream Device. Please refer *
* to the HTML files in ../doc/ for a detailed documentation of   *
* of the protocol file.                                          *
*                                                                *
* Please do not redistribute this file after you have changed    *
* something. If there is a bug or missing features, send me an   *
* email.                                                         *
*****************************************************************/

#include <devStreamProtocol.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

LOCAL int getLine (FILE *file, char *dest, int maxsize, int* linenr);
LOCAL int codeString (char *code, int maxbytes, char *string, char *params);
LOCAL int codeFormat (char *code, int maxbytes, char **pstring);
LOCAL int parseParameter (protocol_t *protocol, char *buffer);

/*****************************************
* parseProtocol
*
* returns OK or linenr of parse error or ERROR if pattern not found
* or -2 if out of memory
*
*/

int devStreamParseProtocol (protocol_t *protocol, FILE *file, char *protocolname)
{
    char buffer[1000];
    char code[1000];
    int linenr = 1;
    char *ptr, *params;
    int i = 0, result;
    
    protocol->codeSize = 0;
    protocol->bufferSize = 100;
    protocol->readTimeout = 100;
    protocol->replyTimeout = 1000;
    protocol->writeTimeout = 100;
    protocol->lockTimeout = 5000;
    protocol->inTerminator[0] = 0;
    protocol->outTerminator[0] = 0;
    protocol->separator[0] = 0;
    protocol->flags = 0;
    
    
    /* look for parameters in protocolname ( like: protocol(p1,p2,p3) )*/
    params = strchr (protocolname, '(');
    if (params != NULL)
    {
        ptr = params + strlen(params) - 1;
        if (*ptr != ')')
        {
            printErr ("missing ) at and of params '%s'\n", params);
            return ERROR;
        }
        *ptr = '\0';
        ptr = params;
        while (ptr != NULL)
        {
            *ptr++ = '\0';
            ptr = strchr (ptr, ',');
        }
        params++;
    }
    for (ptr = protocolname; *ptr; ptr++) *ptr = tolower (*ptr);
    while ((result = getLine (file, buffer, 999, &linenr)) != ERROR)
    {
        #ifdef DEBUG
        printErr ("parseProtocol: parsing line '%s'\n", buffer);
        #endif
        if (buffer[result-1] == '=')
        {
            /* lines like "parameter = value;" */
            getLine (file, buffer+result, 999-result, &linenr);
            if (parseParameter (protocol, buffer) == ERROR)
                return linenr;
        }
        else if (buffer[result-1] == '{')
        {
            /* lines like "protocol { definition }" */
            if (strchr (buffer, ' ') != NULL)
            {
                printErr ("unexpected whitespace in protocol name '%s'\n", buffer);
                return linenr;
            }
            buffer[result-1] = 0;
            if (strcmp (buffer, protocolname) == 0)
            {
                #ifdef DEBUG
                printErr ("parseProtocol: found protocol '%s'\n", protocolname);
                #endif
                while ((result = getLine (file, buffer, 999, &linenr)) != ERROR)
                {
                    #ifdef DEBUG
                    printErr ("parseProtocol: parsing definition '%s'\n", buffer);
                    #endif
                    if (buffer[result-1] == '=')
                    {
                        /* lines like "parameter = value;" */
                        getLine (file, buffer+result, 999-result, &linenr);
                        if (parseParameter (protocol, buffer) == ERROR) break;
                    }
                    else if (buffer[0] == '}')
                    {
/* This function terminates here: */
                        #ifdef DEBUG
                        printErr ("parseProtocol: code[%d] = NUL\n", i);
                        #endif
                        code[i++] = NUL;
                        protocol->codeSize = i;
                        if ((protocol->code = malloc(i)) == NULL)
                        {
                            printErr ("out of memory\n");
                            return -2;
                        }
                        memcpy (protocol->code, code, i);
                        return OK;
                    }
                    else if (strncmp (buffer, "in", 2) == 0)
                    {
                        #ifdef DEBUG
                        printErr ("parseProtocol: code[%d] = IN\n", i);
                        #endif
                        code[i++] = IN;
                        result = codeString (code+i, 999-i, buffer+2, params);
                        if (result == ERROR)
                        {
                            printErr ("error parsing IN argument\n");
                            break;
                        }
                        i += result;
                    }
                    else if (strncmp (buffer, "out", 3) == 0)
                    {
                        #ifdef DEBUG
                        printErr ("parseProtocol: code[%d] = OUT\n", i);
                        #endif
                        code[i++] = OUT;
                        result = codeString (code+i, 999-i, buffer+3, params);
                        if (result == ERROR)
                        {
                            printErr ("error parsing OUT argument\n");
                            break;
                        }
                        i += result;
                    }
                    else if (strncmp (buffer, "wait", 4) == 0)
                    {
                        code[i++] = WAIT;
                        result = strtol (buffer+4, &ptr, 0);
                        if (result < 0 || result > USHRT_MAX)
                        {
                            printErr ("WAIT time %d out of range\n",
                                result, protocolname);
                            break;
                        }
                        if (*ptr != NULL)
                        {
                            printErr ("extra input '%s' after WAIT\n", ptr);
                            break;
                        }
                        *(ushort_t *)(code + i) = (ushort_t) result;
                        i += sizeof (ushort_t);
                    }
                    else if (strncmp (buffer, "event", 5) == 0)
                    {
                        code[i++] = EVENT;
                        result = strtol (buffer+5, &ptr, 0);
                        if (result < 0 || result > USHRT_MAX)
                        {
                            printErr ("EVENT time %d out of range\n", result);
                            break;
                        }
                        if (*ptr != NULL)
                        {
                            printErr ("extra input '%s' after EVENT\n", ptr);
                            break;
                        }
                        *(ushort_t *)(code + i) = (ushort_t) result;
                        i += sizeof (ushort_t);
                    }
                    else
                    {
                        printErr ("unknown command '%s'\n", buffer);
                        break;
                    }
                }
                printErr ("in protocol '%s'\n", protocolname);
                return linenr;
            }
            else
            {
                #ifdef DEBUG
                printErr ("parseProtocol: skipping protocol '%s'\n", buffer);
                #endif
                while (getLine (file, buffer, 999, &linenr) != ERROR)
                {
                    #ifdef DEBUG
                    printErr ("parseProtocol: skipping '%s'\n", buffer);
                    #endif
                    if (buffer[0] == '}') break;
                }
            }
        } else {
            printErr ("parse error in '%s'\n", buffer);
            return linenr;
        }
    }
    printErr ("protocol '%s' not found\n", protocolname);
    return ERROR;
} 

/*****************************************
* getLine
*
* Reads until ';', '=', '{', or '}' skipping comments, leading and terminating
* whitespaces and a terminating ';'. Whitespaces outside quoted strings are
* compressed to a single space. Converts all characters outside a string to
* lowercase. Converts all quoting types to pairs of double quotes.
* Strings can be quoted by pairs of single or double quotes or by
* pairs of parentheses:  'string1'  "string2"   (string3)
* Every special character inside quoted strings can be escaped by \
*
* returns number of bytes put into dest or ERROR if EOF or too small buffer
*/

LOCAL int getLine (FILE *file, char *dest, int maxsize, int* linenr)
{
    int c, i = 0, quoted = 0, escaped = 0, space = 0;
    
    do
    {
        c = getc (file);
        if (c == '\n') ++*linenr;
        if (c == '#')
        {
            while ((c = getc (file)) != '\n' && c != EOF);
            ++*linenr;
        }
    } while (isspace(c));
    while (c != EOF && i < maxsize)
    {
        if (c == '\n') ++*linenr;
        if (escaped)
        {
            dest[i++] = c;
            escaped = 0;
        }
        else if (quoted)
        {
            if (c == '\n')
            {
                printErr ("unterminated string\n");
                return ERROR;
            }
            if (c == '\\') escaped = 1;
            if (c == quoted)
            {
                quoted = 0;
                c = '"';
            }
            else if (c == '"') dest[i++] = '\\';
            dest[i++] = c;
        }
        else
        {
            switch (c)
            {
                case ';':
                    if (space) --i;
                    dest[i] = 0;
                    return i;
                case '{':
                case '}':
                case '=':
                    if (space) --i;
                    dest[i++] = c;
                    dest[i] = 0;
                    return i;
                case ',':
                case ' ':
                case '\n':
                case '\r':
                case '\t':
                    if (!space)
                    {
                        dest[i++] = ' ';
                        space = 1;
                    }
                    break;
                case '#':
                    while ((c = getc (file)) != '\n' && c != EOF);
                    ++*linenr;
                    if (!space)
                    {
                        dest[i++] = ' ';
                        space = 1;
                    }
                    break;
                case '(': c = ')';
                case '"':
                case '\'':
                    quoted = c;
                    dest[i++] = '"';
                    break;
                default:
                    dest[i++] = tolower(c);
                    space = 0;
            }
        }
        c = getc (file);
    }
    printErr ("unexpected end of file\n");
    return ERROR;
}


/*****************************************
* codeString
*
* returns number of coded bytes or ERROR
*/

LOCAL int codeString (char *code, int maxbytes, char *string, char *params)
{
    int i = 0, quoted = 0, escaped = 0;
    ulong_t value;
    int status;
    char *ptr;
    
    #ifdef DEBUG
    printf ("codeString: parsing '%s'\n", string);
    #endif
    while (*string != 0)
    {
        if (escaped)
        {
            switch (*string)
            {
                case '?':
                    code[i++] = SKIP;
                    string++;
                    break;
                case 'n':
                    code[i++] = LF;
                    string++;
                    break;
                case 'r':
                    code[i++] = CR;
                    string++;
                    break;
                case '0':
                    value = strtoul (string, &string, 8) & 0xFF;
                    code[i++] = ESC;
                    code[i++] = value;
                    break;
                case 'x':
                    value = strtoul (string + 1, &string, 16) & 0xFF;
                    code[i++] = ESC;
                    code[i++] = value;
                    break;
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    value = strtoul (string, &string, 10) & 0xFF;
                    code[i++] = ESC;
                    code[i++] = value;
                    break;
                case '$':
                    value = strtoul (string + 1, &string, 10);
                    if (params == NULL)
                    {
                        printErr ("no parameters given\n");
                        return ERROR;
                    }
                    ptr = params;
                    if (value < 1)
                    {
                        printErr ("illegal parameter number before %s\n", string);
                        return ERROR;
                    }
                    while (--value)
                    {
                        ptr += strlen (ptr) + 1;
                        if (*ptr == 0)
                        {
                            printErr ("parameter number out of range\n");
                            return ERROR;
                        }
                    }
                    if (i + strlen(ptr) >= maxbytes)
                    {
                        printErr ("code buffer too small\n");
                        return ERROR;
                    }
                    strcpy (code + i, ptr);
                    i += strlen(ptr); 
                    break;
                default:
                    code[i++] = ESC;
                    code[i++] = *string++;
            }
            escaped = 0;
        }
        else if (quoted)
        {
            if (*string == '"')
            {
                quoted = 0;
                string++;
            }
            else switch (*string)
            {
                case '\\':
                    escaped = 1;
                    string++;
                    continue;
                case '%':
                    status = codeFormat (code + i, maxbytes - i, &string);
                    if (status == ERROR)
                    {
                        printErr ("code buffer too small\n");
                        return ERROR;
                    }
                    i += status;
                    continue;
                default:
                    code[i++] = *string++;
            }
        }
        else if (*string == '"')
        {
            quoted = 1;
            string++;
            continue;
        }
        else if (*string == ' ')
        {
            string++;
            continue;
        }
        else if (*string == '$')
        {
            value = strtoul (string + 1, &string, 10);
            if (params == NULL)
            {
                printErr ("no parameters given\n");
                return ERROR;
            }
            ptr = params;
            if (value < 1)
            {
                printErr ("illegal parameter number before %s\n", string);
                return ERROR;
            }
            while (--value)
            {
                ptr += strlen (ptr) + 1;
                if (*ptr == 0)
                {
                    printErr ("parameter number out of range\n");
                    return ERROR;
                }
            }
            status = codeString (code + i, maxbytes - i, ptr, NULL);
            if (status == ERROR)
            {
                printErr ("while processing parameter #%d\n", value);
                return ERROR;
            }
            i += status;
            continue;
        }
        else if (strncmp (string, "skip", 4) == 0)
        {
            code[i++] = SKIP;
            string += 4;
        }
        else if (strncmp (string, "cr", 2) == 0)
        {
            code[i++] = CR;
            string += 2;
        }
        else if (strncmp (string, "lf", 2) == 0)
        {
            code[i++] = LF;
            string += 2;
        }
        else
        {
            value = strtoul (string, &ptr, 0);
            if (ptr == string)
            {
                printErr ("unknown token '%s'\n", ptr);
                return ERROR;
            }
            string = ptr;
            if (value > 255)
            {
                printErr ("value %ld too big for a byte\n", value);
                return ERROR;
            }
            /* escape every non quoted byte */
            code[i++] = ESC;
            if (i >= maxbytes)
            {
                printErr ("code buffer too small\n");
                return ERROR;
            }
            code[i++] = value;
        }
        if (i >= maxbytes)
        {
            printErr ("code buffer too small\n");
            return ERROR;
        }
    }
    code[i] = 0;
    #ifdef DEBUG
    printf ("codeString: coded:\n");
    for (status = 0; status < i; status++)
    {
        printf ("%02X ", (uchar_t) code[status]);
    }
    printf ("\n");
    #endif
    return i;
}


/*****************************************
* codeFormat
*
* returns number of coded bytes or ERROR
* updates pointer to string
* An n bytes long format string like "%[flags][width][.prec]<conv>"
* will be coded to: 
* byte 0  1   2     3     4       5       6      7...                 n
* FORMAT  n 0xFF <conv><flags><prec|-1><width><formatstring> '%' 'n'  0
* Restictions: n < 128, width < 128, prec < 128
* Flags bits
*   7 6 5 4 3 2 1 0
*  |   | | | | | \__ FORMAT_FLAG_LEFT  (leading '-')
*  |   | | | | \____ FORMAT_FLAG_SIGN  (leading '+')
*  |   | | | \______ FORMAT_FLAG_SPACE (leading ' ')
*  |   | | \________ FORMAT_FLAG_ALT   (leading '#')
*  |   | \__________ FORMAT_FLAG_ZERO  (leading '0')
*  |   \____________ FORMAT_FLAG_SKIP  (leading '*')
*  \________________ unused
*/

LOCAL int codeFormat (char *code, int maxbytes, char **pstring)
{
    int loop = 1;
    ulong_t size;
    char *ptr = *pstring;
    format_t *format = (format_t *) (code + 2);
    
    if (maxbytes < 8 + sizeof (format_t)) return ERROR;
    code[0] = FORMAT;
    /* optional flags */
    format->type = 0xFF;
    format->flags = 0;
    while (loop)
    {
        switch (*++ptr)
        {
            case '-':
                format->flags |= FORMAT_FLAG_LEFT;
                break;
            case '+':
                format->flags |= FORMAT_FLAG_SIGN;
                break;
            case ' ':
                format->flags |= FORMAT_FLAG_SPACE;
                break;
            case '#':
                format->flags |= FORMAT_FLAG_ALT;
                break;
            case '0':
                format->flags |= FORMAT_FLAG_ZERO;
                break;
            case '*':
                format->flags |= FORMAT_FLAG_SKIP;
                break;
            default:
                loop = 0;
        }
    }
    /* minimal fieldwidth */
    size = strtoul(ptr, &ptr, 10);
    if (size >= 128) return ERROR;
    format->width = size;
    /* optional precision */
    if (*ptr == '.')
    {
        size = strtol(++ptr, &ptr, 10);
        if (size >= 128) return ERROR;
        format->prec = size;
    }
    else
    {
        format->prec = -1;
    }
    /* conversion character */
    format->conversion = *ptr++;
    if (format->conversion == '[')
    {
        /* scan char set */
        if (*ptr == '^') ++ptr;
        ptr = strchr (++ptr, ']');
        if (ptr == NULL) return ERROR;
        ++ptr;
        size = ptr - *pstring;
        if (maxbytes < size + 8 + sizeof (format_t) || size >= 128) return ERROR;
        memcpy (format->string, *pstring, size);
    }
    else
    {
        /* copy the format string and insert an 'l' (we always have long or double) */
        size = ptr - *pstring;
        if (maxbytes < size + 8 + sizeof (format_t) || size >= 128) return ERROR;
        memcpy (format->string, *pstring, size - 1);
        format->string[size-1] = 'l';
        format->string[size++] = format->conversion;
    }
    code[1] = size + 3 + sizeof (format_t);
    /* add "%n" to the format string */
    strcpy (format->string + size, "%n");
    /* update the pstring pointer */
    *pstring = ptr;
    return size + 4 + sizeof (format_t);
}

/*****************************************
* parseParameter
*
* returns OK or ERROR
*/

LOCAL int parseParameter (protocol_t *protocol, char *source)
{
    char *ptr;
    long value;
    int i,j;
    char buffer [8];

    #ifdef DEBUG
    printErr ("parseParameter: parsing '%s'\n", source);
    #endif
    ptr = strchr (source, '=');
    if (ptr == NULL)
    {
        printErr ("missing = in '%s'\n", source);
        return ERROR;
    }
    /* lines like "token = value" */
    *ptr = 0;
    if (ptr[-1] == ' ') ptr[-1] = 0;
    if (*++ptr == ' ') ++ptr;
    if (*ptr == 0)
    {
        printErr ("missing value after parameter '%s'\n", source);
    }
    if (strcmp (source, "buffersize") == 0)
    {
        value = strtol (ptr, &ptr, 0);
        if (value < 0 || value > USHRT_MAX)
        {
            printErr ("value %d out of range\n", value);
            return ERROR;
        }
        protocol->bufferSize = value;
    }
    else if (strcmp (source, "terminator") == 0)
    {
        if (codeString (buffer, 8, ptr, NULL) == ERROR)
        {
            printErr ("error parsing terminator\n");
            return ERROR;
        }
        for (i = 0, j = 1; buffer[i];)
        {
            if (buffer[i] == ESC) i++;
            protocol->outTerminator[j] = buffer[i];
            protocol->inTerminator[j++] = buffer[i++];
        }
        if (--j > 4)
        {
            printErr ("terminator too long\n");
            return ERROR;
        }
        protocol->inTerminator[0] = j;
        protocol->outTerminator[0] = j;
        return OK;
    }
    else if (strcmp (source, "interminator") == 0)
    {
        if (codeString (buffer, 8, ptr, NULL) == ERROR)
        {
            printErr ("error parsing terminator\n");
            return ERROR;
        }
        for (i = 0, j = 1; buffer[i];)
        {
            if (buffer[i] == ESC) i++;
            protocol->inTerminator[j++] = buffer[i++];
        }
        if (--j > 4)
        {
            printErr ("terminator too long\n");
            return ERROR;
        }
        protocol->inTerminator[0] = j;
        return OK;
    }
    else if (strcmp (source, "outterminator") == 0)
    {
        if (codeString (buffer, 8, ptr, NULL) == ERROR)
        {
            printErr ("error parsing terminator\n");
            return ERROR;
        }
        for (i = 0, j = 1; buffer[i];)
        {
            if (buffer[i] == ESC) i++;
            protocol->outTerminator[j++] = buffer[i++];
        }
        if (--j > 4)
        {
            printErr ("terminator too long\n");
            return ERROR;
        }
        protocol->outTerminator[0] = j;
        return OK;
    }
    else if (strcmp (source, "separator") == 0)
    {
        if (codeString ((char *) protocol->separator, 6, ptr, NULL) == ERROR)
        {
            printErr ("error parsing separator\n");
            return ERROR;
        }
        return OK;
    }
    else if (strcmp (source, "readtimeout") == 0)
    {
        value = strtol (ptr, &ptr, 0);
        if (value < 0 || value > USHRT_MAX)
        {
            printErr ("value %d out of range\n", value);
            return ERROR;
        }
        protocol->readTimeout = value;
    }
    else if (strcmp (source, "replytimeout") == 0)
    {
        value = strtol (ptr, &ptr, 0);
        if (value < 0 || value > USHRT_MAX)
        {
            printErr ("value %d out of range\n", value);
            return ERROR;
        }
        protocol->replyTimeout = value;
    }
    else if (strcmp (source, "writetimeout") == 0)
    {
        value = strtol (ptr, &ptr, 0);
        if (value < 0 || value > USHRT_MAX)
        {
            printErr ("value %d out of range\n", value);
            return ERROR;
        }
        protocol->writeTimeout = value;
    }
    else if (strcmp (source, "locktimeout") == 0)
    {
        value = strtol (ptr, &ptr, 0);
        if (value < 0 || value > USHRT_MAX)
        {
            printErr ("value %d out of range\n", value);
            return ERROR;
        }
        protocol->lockTimeout = value;
    }
    else if (strcmp (source, "extrainput") == 0)
    {
        i = strlen (ptr);
        if (strncmp (ptr, "ignore", i) == 0)
        {
            protocol->flags |= FLAG_IGNORE_EXTRA_INPUT;
            return OK;
        }
        if (strncmp (ptr, "error", i) == 0)
        {
            protocol->flags &= ~FLAG_IGNORE_EXTRA_INPUT;
            return OK;
        }
        printErr ("value '%s' should be 'ignore' or 'error'\n", ptr);
        return ERROR;
    }
    else
    {
        printErr ("unknown parameter '%s'\n", source);
        return ERROR;
    }
    if (*ptr != 0)
    {
        printErr ("parseParameter: extra text '%s' after value\n", ptr);
        return ERROR;
    }
    return OK;
}

