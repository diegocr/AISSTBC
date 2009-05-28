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
#include <proto/muimaster.h>
#include <proto/alib.h>
#include <SDI_mui.h>
#include <MUI/LayGroup_mcc.h>
#include "utils.h"
#include "debug.h"
#include "main.h"

GLOBAL Object * GuiGfxPic( STRPTR FileName );
GLOBAL void InfoText( UWORD c, STRPTR fmt, ... );

Object * LayGroup(STRPTR ImagesFolder)
{
	STATIC UBYTE ErrorMsg[512];
	BPTR lock = 0;
	long err, found = 0;
	Object * lGroup = NULL, * lg;
	#if USE_LG_PICD
	UWORD width = 0, height = 0;
	UBYTE depth = 0;
	#endif /* USE_LG_PICD */
	UBYTE file[2048], *fmt = NULL;
	BOOL use_picd = TRUE;
	char __fib[sizeof(struct FileInfoBlock) + 3];
	struct FileInfoBlock *fib = 
		(struct FileInfoBlock *)(((long)__fib + 3L) & ~3L);
	
	ENTER();
	DBG_STRING(ImagesFolder);
	if(!(ImagesFolder && *ImagesFolder))
		goto done;
	
	if(!(lock = Lock( ImagesFolder, SHARED_LOCK )))
	{
		fmt = "Cannot lock images folder";
		goto done;
	}
	
	if(Examine(lock,fib)==DOSFALSE)
	{
		fmt = "Examine() failed, error %ld";
		goto done;
	}
	
	lGroup = ScrollgroupObject,
		GroupSpacing(0),
		MUIA_Scrollgroup_FreeHoriz, FALSE,
		MUIA_Scrollgroup_UseWinBorder, TRUE,
		MUIA_Scrollgroup_Contents, lg = LayGroupObject,
		End,
	End;
	
	DBG_POINTER(lGroup);
	if(!(lGroup))
		goto done;
	
	if(GetIntEnv("AISSTBC_OMITPICD"))
		use_picd = FALSE;
	
	DoMethod( lg, MUIM_Group_InitChange);
	
	while(ExNext(lock,fib)!=DOSFALSE)
	{
		Object * img;
		#if USE_LG_PICD
		UWORD w, h;
		UBYTE d;
		#endif /* USE_LG_PICD */
		
		// skip Ghosted and Selected images (ie image_G)
		if(*&fib->fib_FileName[strlen(fib->fib_FileName)-2] == '_')
			continue;
		
		*file = 0;
		AddPart( file, ImagesFolder, sizeof(file)-1);
		AddPart( file, fib->fib_FileName, sizeof(file)-1);
		
		InfoText(0,"Loading '%s' preview...", fib->fib_FileName );
		
	#if USE_LG_PICD
	  if( use_picd ) {
		if(!PictureDimension(file,&w,&h,&d))
		{
			DBG("Cannot query picture attributes..\n");
			continue;
		}
		
		if((w < 2) || (h < 2) || (d < 1))
		{
			DBG("Wrong image attributes...\n");
			continue;
		}
		
		if( width == 0 )
		{
			width = w;
			height = h;
			depth = d;
		}
		else if((w != width) || (h != height) || (d != depth))
		{
			DBG("Image attributes does not match with the previous loaded image(s)..\n");
			continue;
		}
	  }
	#endif /* USE_LG_PICD */
		
		if(!(img = GuiGfxPic( file )))
		{
			fmt = "Failed creating GuiGfx.mcc object...";
			break;
		}
		
		DoMethod( lg, OM_ADDMEMBER, img );
		found++;
	}
	
	DoMethod( lg, MUIM_Group_ExitChange);
	if(fmt != NULL)
	{
		MUI_DisposeObject( lGroup );
		lGroup = NULL;
	}
done:
	err = IoErr();
	
	if(lock)
		UnLock( lock );
	
	DBG_ASSERT(lGroup != NULL);
	if(lGroup == NULL)
	{
		if(fmt == NULL)
			fmt = "IoErr %ld Creating LayGroup Object...";
		
		SNPrintf( ErrorMsg, sizeof(ErrorMsg)-1, fmt, err );
		
		DBG_STRING(ErrorMsg);
		lGroup = VGroup, TextFrame,
			Child, TextObject,
				MUIA_Text_Contents, ErrorMsg,
			End,
			Child, HVSpace,
		End;
		
		InfoText(100,ErrorMsg);
	}
	else
	{
		InfoText(100,"Loaded %ld Images.",found);
	}
	
	RETURN(lGroup);
	return(lGroup);
}
