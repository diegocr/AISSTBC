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

#ifndef UTILS_H
#define UTILS_H

#include <stdarg.h>
#include <exec/types.h>

GLOBAL VOID ShowError(CONST_STRPTR fmt, ...);
GLOBAL void tell( CONST_STRPTR fmt, ...);
GLOBAL LONG ask( CONST_STRPTR fmt, ...);
GLOBAL LONG ShowRequestA( STRPTR gadgets, CONST_STRPTR fmt, va_list args );
GLOBAL LONG ShowRequest( STRPTR gadgets, CONST_STRPTR fmt, ... );
GLOBAL LONG VSNPrintf(STRPTR outbuf, LONG size, CONST_STRPTR fmt, va_list args);
GLOBAL LONG SNPrintf( STRPTR outbuf, LONG size, CONST_STRPTR fmt, ... );
GLOBAL STRPTR AslFile( STRPTR TitleText, STRPTR InitialPattern );
GLOBAL BOOL PictureDimension( STRPTR filename, UWORD * width, UWORD * height, UBYTE * depth );
GLOBAL ULONG GetIntEnv(STRPTR varname);
GLOBAL void bzero( void *data, ULONG fsize );
GLOBAL ULONG Atoi(CONST_STRPTR str);

/**
 * this struct should be at his own heade file, but I'll not create one 
 * just for this
 */

typedef struct
{
	UBYTE DestMode;
	UBYTE Precision;
	UBYTE Masking;
	UBYTE Compression;
	
	ULONG imgs;
	STRPTR *files;
	
	APTR scr;
	LONG BltMinterm;
	LONG BltMask;
	
} CTBData; /* CreateToolBar() Data */


#endif /* UTILS_H */
