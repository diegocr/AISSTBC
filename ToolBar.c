/* ***** BEGIN LICENSE BLOCK *****
 * Version: BSD License
 * 
 * Copyright (c) 2006, Diego Casorran <dcasorran@gmail.com>
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


#include <proto/dos.h>
#include <proto/muimaster.h>
#include <MUI/Toolbar_mcc.h>
#include "ToolBar.h"
#include "debug.h"

STATIC VOID ArrayToFile( STRPTR filename, CONST_STRPTR data, ULONG size )
{
	BPTR file;
	
	if(!(file = Lock( filename, SHARED_LOCK )))
	{
		if((file = Open( filename, MODE_NEWFILE)))
		{
			Write( file,(STRPTR) data, size );
			
			Close( file );
		}
	}
	else UnLock( file );
}

Object * EmbedToolbar(struct MUIP_Toolbar_Description * desc)
{
	BPTR t;
	
	if((t = Lock( "T:", SHARED_LOCK )))
	{
		BPTR ProgDir;
		
		ProgDir = CurrentDir( t );
		
		ArrayToFile("AISS.Toolbar",aiss_toolbar_normal,sizeof(aiss_toolbar_normal));
		ArrayToFile("AISS_S.Toolbar",aiss_toolbar_selected,sizeof(aiss_toolbar_selected));
		
		CurrentDir( ProgDir );
		UnLock( t );
	}
	
	return ToolbarObject,
		MUIA_Toolbar_ImageType, 	MUIV_Toolbar_ImageType_File,
		MUIA_Toolbar_ImageNormal,	"AISS.Toolbar",
		MUIA_Toolbar_ImageSelect,	"AISS_S.Toolbar",
		//MUIA_Toolbar_ImageGhost,	"AISS_G.Toolbar",
		MUIA_Toolbar_Path, "T:",
		MUIA_Toolbar_Description,	desc,
		MUIA_Font,	MUIV_Font_Tiny,
		MUIA_ShortHelp,	TRUE,
		MUIA_Draggable,	FALSE,
	End;
}

