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
#include <proto/datatypes.h>
#include <proto/alib.h>
#include <proto/icon.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include "utils.h"
#include "debug.h"
#include <string.h> /* bzero() */

GLOBAL void InfoText( UWORD c, STRPTR fmt, ... );
GLOBAL struct BitMap *BltBitMask(struct BitMap *srcbm,
	UWORD width,UWORD height,UBYTE depth,BOOL backfill);

struct NewToolBar
{
	Object * dto;
	struct BitMap *BitMap;
} tb[99];

STATIC UBYTE sign[] = /* AISS ToolBar Creator's Creation */
	"\x00\xBE\xB6\xAC\xAC\xDF\xAB\x90\x90\x93\xBD\x9E\x8D\xDF\xBC\x8D"
	"\x9A\x9E\x8B\x90\x8D\xD8\x8C\xDF\xBC\x8D\x9A\x9E\x8B\x96\x90\x91";

/***************************************************************************/

#if mskHasAlpha != 4
# error wrong mskHasAlpha
#endif
#if cmpByteRun1 != 1
# error wrong cmpByteRun1
#endif

STATIC BOOL SaveDTBitMap(STRPTR filename, struct BitMap * bm, UWORD width, 
	UWORD height, UBYTE depth, UBYTE Masking, UBYTE Compression, 
	ULONG modeid, ULONG *palette, APTR scr )
{
	Object * obj;
	BOOL rc = FALSE;
	long ncols = ((depth < 9) ? (1L << depth) : 0);
	
	if( sign[0] == 0 )
	{
		register unsigned char * p = &sign[1];
		
		while( *p )
			*p++ ^= 0xff;
		
		sign[0] = 1;
	}
	
	DBG("%s: %ldx%ldx%ld, ncols=%ld, modeid=%ld scr=%lx\n",
		filename, width, height, depth, ncols,modeid,src);
	
	obj = NewDTObject( NULL,
		DTA_SourceType,	DTST_RAM,
		DTA_GroupID,	GID_PICTURE,
		PDTA_BitMap,	(ULONG) bm,
		PDTA_ModeID,	modeid,
		(ncols ? PDTA_NumColors : PDTA_DestMode),
		(ncols ? ncols : PMODE_V43),
		DTA_ObjName,(ULONG) &sign[1],
		scr ? PDTA_Remap : TAG_IGNORE, TRUE,
		scr ? PDTA_Screen: TAG_IGNORE, (ULONG)scr,
	TAG_DONE );
	
	if( obj )
	{
		BPTR fd;
		struct BitMapHeader *bmhd;
		
		//DoMethod( obj, DTM_PROCLAYOUT, NULL, 1L);
		DoDTMethod( obj, NULL,NULL,DTM_PROCLAYOUT, NULL, TRUE);
		
		GetDTAttrs( obj,
			PDTA_BitMapHeader,(ULONG) &bmhd,
		TAG_DONE );
		
		bmhd->bmh_Depth		= depth;
		bmhd->bmh_Width		= width;
		bmhd->bmh_Height	= height;
		bmhd->bmh_YAspect	= 22;
		bmhd->bmh_XAspect	= 22;
		bmhd->bmh_Masking	= Masking;
		bmhd->bmh_Compression	= Compression;
		bmhd->bmh_PageWidth	= 
			((width < 321) ? 320 : 
			((width < 641) ? 640 : 
			((width < 1025) ? 1024 : 
			((width < 1281) ? 1280 : 1600))))
		;
		bmhd->bmh_PageHeight	= bmhd->bmh_PageWidth * 3 / 4;
		
		if( ncols )
		{
			UBYTE *cmap;
			ULONG *cregs;
			int x;
			
			GetDTAttrs( obj, 
				PDTA_ColorRegisters,(ULONG)	&cmap,
				PDTA_CRegs,(ULONG)		&cregs,
			TAG_DONE );
			
			#if 1
			if((x = (3*ncols))) do {
				
				*cmap++ = ((*cregs++ = *palette++) >> 24);
			
			} while(--x > 0);
			#else
			for( x = 3*ncols-1 ; x >= 0 ; x-- )
			{
				cregs[x] = palette[x];
				cmap[x] = palette[x] >> 24;
			}
			#endif
		}
		
		if((fd = Open( filename, MODE_NEWFILE)))
		{
			rc = DoDTMethod(obj,NULL,NULL,DTM_WRITE,NULL,fd,DTWM_IFF,TAG_DONE)?1:0;
			Close( fd );
			
			if(rc == FALSE)
			{
				ShowError("DT's DTM_WRITE Method Failed!");
				
				if(depth > 8)
					ShowError("Try disabling the DT Mode...");
			}
		}
		else ShowError("Error writing to ...%s", FilePart( filename ));
		
		DisposeDTObject( obj );
	}
	else ShowError("Cannot create DT Object for ToolBar!");
	
	return(rc);
}

/***************************************************************************/

VOID ScaleBitMap(struct BitMap *src, UWORD srcw, UWORD srch, 
		 struct BitMap *dest, UWORD destw, UWORD desth )
{
	struct BitScaleArgs bsa;
	
	bsa.bsa_SrcX=0;
	bsa.bsa_SrcY=0;
	bsa.bsa_SrcWidth=srcw;
	bsa.bsa_SrcHeight=srch;
	bsa.bsa_XSrcFactor=srcw;
	bsa.bsa_YSrcFactor=srch;
	bsa.bsa_DestX=0;
	bsa.bsa_DestY=0;
	bsa.bsa_DestWidth=destw;
	bsa.bsa_DestHeight=desth;
	bsa.bsa_XDestFactor=destw;
	bsa.bsa_YDestFactor=desth;
	bsa.bsa_SrcBitMap=src;
	bsa.bsa_DestBitMap=dest;
	bsa.bsa_Flags=0;
	bsa.bsa_XDDA=0;
	bsa.bsa_YDDA=0;
	bsa.bsa_Reserved1=0;
	bsa.bsa_Reserved2=0;
	
	BitMapScale(&bsa);
}

/***************************************************************************/

STATIC ULONG GetRightPrecision(UBYTE val)
{
	// Convert the values of a MUI Cycle to the Right precision number
	
	return	((val == 1) ? PRECISION_GUI : 
		((val == 2) ? PRECISION_ICON : 
		((val == 3) ? PRECISION_IMAGE : PRECISION_EXACT )));
}

STATIC BOOL CreateToolBarSet( CTBData * data, STRPTR BaseName, UBYTE TBMode )
{
	ULONG *palette;
	ULONG modeid=0;
	BOOL rc = FALSE;
	UBYTE file[1024];
	struct BitMapHeader *bmhd;
	struct BitMap * TBBM = NULL;
	ULONG x, width = 0, height=0, depth=0, total_width = 0, blt_width = 0;
	
	if((int)data->imgs > (int)(sizeof(tb)/sizeof(tb[0]))-1)
		goto done; // this shouldnt happens...
	
	bzero((APTR)tb, sizeof(tb)-1);
	
	for( x = 0 ; x < data->imgs ; x++ )
	{
		SNPrintf( file, sizeof(file)-1, "%s%s", data->files[x],
			((TBMode == 1) ? "_G":((TBMode == 2) ? "_S":"")));
		
		InfoText( x*100/data->imgs, "Loading %s...", FilePart( file ));
		
		#if PMODE_V43 != 1
		# error PMODE_V43 is wrong !?
		#endif
		
		tb[x].dto = NewDTObject( file,
			DTA_SourceType,		DTST_FILE,
			DTA_GroupID,		GID_PICTURE,
			data->Precision ? OBP_Precision:TAG_IGNORE,
			GetRightPrecision(data->Precision),
			PDTA_Remap,		1,//FALSE,
			data->DestMode ? PDTA_DestMode:TAG_IGNORE, data->DestMode-1,
		//	PDTA_FreeSourceBitMap, 	TRUE,
		//	PDTA_UseFriendBitMap, 	TRUE,
		//	PDTA_DitherQuality,	2, <- causes lots of datatypes hits...
		//	PDTA_ScaleQuality,	0,
		TAG_DONE);
		
		if(!tb[x].dto)
		{
			ShowError("Error loading \"%s\"", file );
			goto done;
		}
		
		if(!DoMethod( tb[x].dto, DTM_PROCLAYOUT, NULL, 1L))
		{
			ShowError("DTM_PROCLAYOUT Failed!");
			goto done;
		}
		
		GetDTAttrs( tb[x].dto,
			PDTA_BitMapHeader, (ULONG) &bmhd,
			PDTA_DestBitMap,   (ULONG) &tb[x].BitMap,
		TAG_DONE);
		
		if(!tb[x].BitMap)
			GetDTAttrs(tb[x].dto, PDTA_BitMap, (ULONG) &tb[x].BitMap, TAG_DONE);
		
		if(!tb[x].BitMap || !bmhd || !bmhd->bmh_Depth)
		{
			ShowError("Got wrong BitMap or BitMapHeader");
			goto done;
		}
		
		if(!width)
		{
			width = bmhd->bmh_Width;
			height = bmhd->bmh_Height;
			depth = bmhd->bmh_Depth;
			
			GetDTAttrs(tb[x].dto,PDTA_CRegs,(ULONG)&palette,PDTA_ModeID,(ULONG)&modeid,TAG_DONE);
		}
		
		DBG(" --- %s: %ldx%ldx%ld mask=%ld cmp=%ld\n", FilePart(file),
			bmhd->bmh_Width, bmhd->bmh_Height, bmhd->bmh_Depth, 
				bmhd->bmh_Masking, bmhd->bmh_Compression );
		
		if((width != bmhd->bmh_Width)
			|| (height != bmhd->bmh_Height)
				|| (depth != bmhd->bmh_Depth))
		{
			ShowError("ALL Images MUST have the same width, height, and depth");
			goto done;
		}
		
		// total width, plus 1 for 1pixel spacing
		total_width += (bmhd->bmh_Width + 1);
	}
	
	// minus unneeded last 1pixel space
	--total_width;
	
	DBG("Loaded %ld images, total width = %ld, h=%ld,d=%ld\n", x, total_width, height, depth);
	
	//if(depth < 8)
	//	depth = 8;
	
	TBBM = AllocBitMap(total_width, height, depth, BMF_CLEAR /*|BMF_STANDARD|BMF_MINPLANES*/, tb[0].BitMap );
	if(!TBBM)
	{
		ShowError("Cannot create %ldx%ldx%ld bitmap", total_width,height,depth);
		goto done;
	}
	
	for( x = 0 ; x < data->imgs ; x++ )
	{
	/*	DBG("Bliting %ld...\n", x );
		InfoText( x*100/data->imgs, "Bliting ToolBar BitMap...");
	*/	
		BltBitMap( tb[x].BitMap, 0, 0, TBBM, blt_width, 0, width, height, data->BltMinterm /*ABC|ABNC*/, data->BltMask /*0xff*/,NULL);
		blt_width += width + 1;
	}
	
	// save the bitmap's toolbar to disk
	DBG("Saving ToolBar...\n");
	InfoText( 100, "Saving ToolBar...");
	
	SNPrintf( file, sizeof(file)-1, "%s%s.ToolBar", BaseName,
		((TBMode == 1) ? "_G":((TBMode == 2) ? "_S":"")));
	
	/*TBBM =*/// BltBitMask(TBBM,total_width,height,depth,TRUE);
	
	
	if(SaveDTBitMap( file, TBBM, total_width, height, depth, data->Masking, data->Compression, modeid, palette, data->scr))
		rc = TRUE;
done:
	DBG("Exiting... rc = %s\n", rc ? "ALL FINE":"ERROR");
	
	for( x = 0 ; x < data->imgs ; x++ )
	{
		if(tb[x].dto)
			DisposeDTObject(tb[x].dto);
	}
	
	return(rc);
}

/***************************************************************************/

BOOL CreateToolBar( CTBData * data )
{
	BOOL rc = FALSE;
	STRPTR name;
	
	if((name = AslFile("Select ToolBar filename base",NULL)))
	{
		int x;
		
		for( x = 0 ; x < 3 ; x++ )
		{
			rc = CreateToolBarSet( data, name, x );
			
			if(!rc)
			{
				if( x )
				{
					ShowError("Failed creating '%s' ToolBar",
						x == 1 ? "Ghosted":"Selected");
				}
				break;
			}
		}
		
		if( rc )
			InfoText(0,"ToolBar Created!");
		
		FreeVec( name );
	}
	
	if( rc == FALSE )
		DisplayBeep(NULL);
	
	return rc;
}


