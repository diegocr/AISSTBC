/* ***** BEGIN LICENSE BLOCK *****
 * Version: BSD License
 * 
 * Copyright (c) 2009, Diego Casorran <dcasorran@gmail.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 ** Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  
 ** Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ***** END LICENSE BLOCK ***** */


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/datatypes.h>
#include <datatypes/pictureclass.h>
#include <proto/asl.h>
#include <SDI_compiler.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include "utils.h"
#include "debug.h"
#include "_ctype.h"

/****************************************************************************/
GLOBAL void InfoText( UWORD c, STRPTR fmt, ... );

STATIC UBYTE okm[] = "Ok";

VOID ShowError(CONST_STRPTR fmt, ...)
{
	va_list args;
	UBYTE buf[4096];
	
	va_start (args, fmt);
	VSNPrintf( buf, sizeof(buf)-1, fmt, args );
	va_end(args);
	
	InfoText(0, buf );
	
	if(((struct Process *)FindTask(NULL))->pr_WindowPtr != (APTR)-1L)
	{
		static struct IntuiText body = { 0,0,0, 15,5, NULL, NULL, NULL };
		static struct IntuiText   ok = { 0,0,0,  6,3, NULL, okm, NULL };
		
		body.IText = (UBYTE *) buf;
		AutoRequest(NULL,&body,NULL,&ok,0,0,640,72);
	}
}

void tell( CONST_STRPTR fmt, ...)
{
	ShowRequestA( okm, fmt, &fmt+1 );
}

LONG ask( CONST_STRPTR fmt, ...)
{
	return ShowRequestA( "Yes|No", fmt, &fmt+1 );
}

LONG ShowRequestA( STRPTR gadgets, CONST_STRPTR fmt, va_list args )
{
	struct EasyStruct ErrReq = {
		sizeof (struct EasyStruct), 0, NULL, NULL, NULL
	};
	
	DBG_STRING(fmt);
	
	ErrReq.es_TextFormat   = (STRPTR) fmt;
	ErrReq.es_GadgetFormat = gadgets;
	
	return EasyRequestArgs(NULL, &ErrReq, NULL, args );
}

LONG ShowRequest( STRPTR gadgets, CONST_STRPTR fmt, ... )
{
	va_list args;
	LONG result;
	
	va_start (args, fmt);
	result = ShowRequestA( gadgets, fmt, args );
	va_end( args );
	
	return result;
}

/****************************************************************************/

struct RawDoFmtStream {
	
	STRPTR Buffer;
	LONG Size;
};

static void RawDoFmtChar( REG(d0,UBYTE c), REG(a3,struct RawDoFmtStream *s))
{
	if(s->Size > 0)
	{
		*(s->Buffer)++ = c;
		 (s->Size)--;
		
		if(s->Size == 1)
		{
			*(s->Buffer)	= '\0';
			  s->Size	= 0;
		}
	}
}

LONG VSNPrintf(STRPTR outbuf, LONG size, CONST_STRPTR fmt, va_list args)
{
	long rc = 0;
	
	if((size > (long)sizeof(char)) && (outbuf != NULL))
	{
		struct RawDoFmtStream s;
		
		s.Buffer = outbuf;
		s.Size	 = size;
		
		RawDoFmt( fmt, (APTR)args, (void (*)())RawDoFmtChar, (APTR)&s);
		
		if((rc = ( size - s.Size )) != size)
			--rc;
	}
	
	return(rc);
}

LONG SNPrintf( STRPTR outbuf, LONG size, CONST_STRPTR fmt, ... )
{
	va_list args;
	long rc;
	
	va_start (args, fmt);
	rc = VSNPrintf( outbuf, size, fmt, args );
	va_end(args);
	
	return(rc);
}

/****************************************************************************/

/****************************************************************************/

BOOL PictureDimension( STRPTR filename, UWORD * width, UWORD * height, UBYTE * depth )
{
	Object * obj;
	BOOL rc = FALSE;
	
	obj = NewDTObject( filename,
		DTA_SourceType,		DTST_FILE,
		DTA_GroupID,		GID_PICTURE,
		PDTA_Remap,		FALSE,
	TAG_DONE);
	
	if(obj)
	{
		struct BitMapHeader *bmhd = NULL;
		
		GetDTAttrs( obj,
			PDTA_BitMapHeader, (ULONG) &bmhd,
		TAG_DONE);
		
		if( bmhd != NULL )
		{
			if(width)
				*width = bmhd->bmh_Width;
			if(height)
				*height = bmhd->bmh_Height;
			if(depth)
				*depth = bmhd->bmh_Depth;
			
			rc = TRUE;
		}
		
		DisposeDTObject(obj);
	}
	
	if( rc == FALSE )
	{
		if(width)
			*width = 0;
		if(height)
			*height = 0;
		if(depth)
			*depth = 0;
	}
	
	#define PA(X) ((X) ? ((long)(*(X))) : -1)
	DBG("%s: %ldx%ldx%ld %s\n", filename, PA(width),PA(height),PA(depth),
		rc ? "" : " ++ ERROR" );
	
	return rc;
}

/****************************************************************************/

ULONG GetIntEnv(STRPTR varname)
{
	UBYTE buf[64];
	LONG rc = 0;
	
	DBG("Getting ENVAR \"%s\"...\n", varname );
	DBG_ASSERT(DOSBase != NULL);
	if( DOSBase == NULL )
		return 0;
	
	buf[0] = '\0';
	GetVar( varname,buf,sizeof(buf), GVF_BINARY_VAR);
	
	StrToLong( buf, &rc );
	
	RETURN(rc);
	return(rc);
}

/****************************************************************************/
/**
 * Atoi function from the clib2 library by Olaf
 */
ULONG Atoi(CONST_STRPTR str)
{
	size_t num_digits_converted = 0;
	BOOL is_negative;
	long result = 0;
	register int base = 0;
	register long new_sum;
	register long sum;
	char c;
	
	if(!(str && *str))
		return 0;
	
	/* Skip all leading blanks. */
	while((c = (*str)) != '\0')
	{
		if(! isspace(c))
			break;

		str++;
	}

	/* The first character may be a sign. */
	if(c == '-')
	{
		/* It's a negative number. */
		is_negative = TRUE;

		str++;
	}
	else
	{
		/* It's not going to be negative. */
		is_negative = FALSE;

		/* But there may be a sign we will choose to
		   ignore. */
		if(c == '+')
			str++;
	}

	c = (*str);

	/* There may be a leading '0x' to indicate that what
	   follows is a hexadecimal number. */
	if(base == 0 || base == 16)
	{
		if((c == '0') && (str[1] == 'x' || str[1] == 'X'))
		{
			base = 16;

			str += 2;

			c = (*str);
		}
		
		if((c == '$') && isxdigit(str[1]))
		{
			base = 16;
			
			c = *(++str);
		}
	}

	/* If we still don't know what base to use and the
	   next letter to follow is a zero then this is
	   probably a number in octal notation. */
	if(base == 0)
	{
		if(c == '0')
			base = 8;
		else
			base = 10;
	}

	sum = 0;

	if(1 <= base && base <= 36)
	{
		while(c != '\0')
		{
			if('0' <= c && c <= '9')
				c -= '0';
			else if ('a' <= c)
				c -= 'a' - 10;
			else if ('A' <= c)
				c -= 'A' - 10;
			else
				break;

			/* Ignore invalid numbers. */
			if(c >= base)
				break;

			new_sum = base * sum + c;
			if(new_sum < sum) /* overflow? */
			{
				SetIoErr(ERROR_LINE_TOO_LONG);

				if(is_negative)
					result = LONG_MIN;
				else
					result = LONG_MAX;

				goto out;
			}

			sum = new_sum;

			str++;

			c = (*str);

			num_digits_converted++;
		}
	}

	/* Did we convert anything? */
	if(num_digits_converted == 0)
		goto out;

	if(is_negative)
		result = (-sum);
	else
		result = sum;

 out:

	return(result);
}

/****************************************************************************/

void bcopy(const void *src,void *dest,size_t len)
{
	CopyMem((APTR) src, dest, len );
}

/****************************************************************************/

void bzero( void *data, size_t fsize )
{
	register unsigned long * uptr = (unsigned long *) data;
	register unsigned char * sptr;
	long size = (long) fsize;
	
	// first blocks of 32 bits
	while(size >= (long)sizeof(ULONG))
	{
		*uptr++ = 0;
		size -= sizeof(ULONG);
	}
	
	sptr = (unsigned char *) uptr;
	
	// now any pending bytes
	while(size-- > 0)
		*sptr++ = 0;
}

/****************************************************************************/

size_t strlen(const char *string)
{ const char *s=string;
  
  if(!(string && *string))
  	return 0;
  
  do;while(*s++); return ~(string-s);
}

/****************************************************************************/

#if 0
char *strchr(const char *s,int c)
{
  while (*s!=(char)c)
    if (!*s++)
      { s = (char *)0; break; }
  return (char *)s;
}
#endif

/****************************************************************************/
