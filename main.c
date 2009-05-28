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
#define __NOLIBBASE__
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <proto/muimaster.h>
#include <proto/asl.h>
#include <proto/utility.h>
#include <MUI/BetterString_mcc.h>
#include <mui/Guigfx_mcc.h>
#include <MUI/Toolbar_mcc.h>
#include <SDI_mui.h>
#include "utils.h"
#include "main.h"
#include <stdarg.h>

/***************************************************************************/

struct UtilityBase     * UtilityBase   = NULL;
struct DosLibrary      * DOSBase       = NULL;
struct GfxBase         * GfxBase       = NULL;
struct IntuitionBase   * IntuitionBase = NULL;
struct Library         * MUIMasterBase = NULL;
struct Library         * DataTypesBase = NULL;
struct MUI_CustomClass * DropableClass = NULL;
struct MUI_CustomClass * GuiGfxClass   = NULL;

STATIC Object * app = NULL, * win = NULL, * imagesFolder = NULL, 
	* gauge = NULL, * lGroup = NULL, * GGQCyc, * PMode, * Masking,
	* Compression, * Precision, *BltMinterm, *BltMask, *Remap,
	* AslFilePath;

STATIC CONST UBYTE DefaultImagesFolder[] = "TBImages:";
STATIC CONST STRPTR ApplicationData[] = {
	"AISSTBC",					/* title / base */
	"$VER:" PROGNAME " " PROGVERZ " (" PROGDATE ")",/* Version */
	PROGNAME " " PROGCOPY,				/* Description */
	NULL
};
STATIC STRPTR GGQuality[] = {"Low","Medium","High","Best",NULL};
STATIC STRPTR PModes[] = {"«none»","PMODE_V42","PMODE_V43",NULL};
STATIC STRPTR MaskTypes[] = {"«none»","HasMask","HasTransColor","Lasso","HasAlpha",NULL};
STATIC STRPTR CmpTypes[] = {"«none»","ByteRun1",NULL};
STATIC STRPTR PrecisionTypes[] = {"«none»","Gui","Icon","Image","Exact",NULL};

STATIC struct MUIP_Toolbar_Description ToolbarBank[] =
{
	{ TDT_BUTTON, 'c', 0, "Create",	"Create ToolBar",		NULL},
	{ TDT_BUTTON, 'n', 0, "New",	"Clear current droped images",	NULL},
	{ TDT_BUTTON, 'u', 0, "Undo",	"Undo last droped image",	NULL},
	Toolbar_Space,
	{ TDT_BUTTON, 'l', 0, "Load",	"Load AISS's Project",		NULL},
	{ TDT_BUTTON, 's', 0, "Save",	"Save AISS's Project",		NULL},
	Toolbar_Space,
	{ TDT_BUTTON, 'p', 0, "Prefs",	"Settings",			NULL},
	{ TDT_BUTTON, 'a', 0, "About",	"About This Program",		NULL},
	Toolbar_End
};

GLOBAL Object * LayGroup(STRPTR ImagesFolder);
GLOBAL BOOL CreateToolBar( CTBData * data );
GLOBAL ULONG STDARGS DoSuperNew(struct IClass *cl, Object *obj, ULONG tag1, ...);
GLOBAL Object * AISSLogo ( VOID );
GLOBAL Object * EmbedToolbar(struct MUIP_Toolbar_Description * desc);
GLOBAL void InfoText( UWORD c, STRPTR fmt, ... );

#define MUIM_GUIGFX_IMAGE	(0x90000000|(1L << 1))
#define MUIM_CreateTB		(0x90000000|(1L << 2))
#define MUIM_LoadProject	(0x90000000|(1L << 3))
#define MUIM_SaveProject	(0x90000000|(1L << 4))
#define MUIM_NewProject		(0x90000000|(1L << 5))
#define MUIM_IsProjectSaved	(0x90000000|(1L << 6))
#define MUIM_UndoImage		(0x90000000|(1L << 7))

enum { TB_CREATE, TB_NEW, TB_UNDO, TB_LOAD=4, TB_SAVE, TB_PREFS=7, TB_ABOUT, TB_END };

#include "debug.h"
/***************************************************************************/

Object * GuiGfxPic( STRPTR FileName )
{
	return (Object *) NewObject(GuiGfxClass->mcc_Class, NULL,
		MUIA_Guigfx_FileName,(ULONG) FileName,
	TAG_DONE);
}

STATIC STRPTR GetString( Object * obj, STRPTR def )
{
	STRPTR str;
	
	str = (STRPTR) xget( obj, MUIA_String_Contents );
	
	if(!(str && *str))
		str = def;
	
	return( str );
}

STATIC ULONG GetStringInt( Object * obj, STRPTR def )
{
	return((ULONG) Atoi(GetString(obj,def)));
}

STRPTR AslFile( STRPTR TitleText, STRPTR InitialPattern )
{
	struct Library * AslBase;
	STRPTR name = NULL;
	
	ENTER();
	
	if((AslBase = OpenLibrary("asl.library", 0)))
	{
		struct FileRequester *freq;
		
		if((freq = AllocAslRequestTags(ASL_FileRequest, TAG_DONE)))
		{
			if(AslRequestTags(freq,
				ASLFR_TitleText, (ULONG)TitleText,
				ASLFR_InitialDrawer, (ULONG)GetString(AslFilePath,"PROGDIR:"),
				ASLFR_RejectIcons, TRUE,
				InitialPattern != NULL ? ASLFR_DoPatterns:TAG_IGNORE, TRUE,
				InitialPattern != NULL ? ASLFR_InitialPattern:TAG_IGNORE, (ULONG)InitialPattern,
				TAG_DONE))
			{
				ULONG namelen;
				
				namelen = strlen(freq->fr_File) + strlen(freq->fr_Drawer) + 32;
				
				if((name = AllocVec(namelen + 1, MEMF_PUBLIC | MEMF_CLEAR)))
				{
					AddPart(name, freq->fr_Drawer, namelen);
					AddPart(name, freq->fr_File, namelen);
					
					set( AslFilePath, MUIA_String_Contents,(ULONG)freq->fr_Drawer);
				}
			}
			
			FreeAslRequest(freq);
		}
		else ShowError("Cannot alloc AslRequest!");
		
		CloseLibrary( AslBase );
	}
	else ShowError("Cannot open %s","asl.library");
	
	DBG_STRING(name);
	
	return name;
}

/***************************************************************************/

struct UndoList
{
	struct UndoList * next;
	Object * obj;
	UWORD width;
};

struct DropableClassData
{
	ULONG width;
	Object * space, * LastAdded;
	BOOL WasProjectSaved;
	struct UndoList * UnDo;
	STRPTR files[99];
};

STATIC VOID AddUndoItem(struct DropableClassData *data,Object *obj,UWORD width)
{
	struct UndoList * new;
	
	if((new = AllocMem(sizeof(struct UndoList), MEMF_ANY)))
	{
		new->obj = obj;
		new->width = width;
		new->next = data->UnDo;
		data->UnDo = new;
	}
}

STATIC BOOL UndoImage(struct IClass *cl, Object *obj)
{
	struct DropableClassData * data = INST_DATA(cl,obj);
	struct List *childList;
	BOOL done = FALSE;
	
	if(data->UnDo == NULL)
		return FALSE;
	
	if((childList = (struct List *)xget(obj, MUIA_Group_ChildList)))
	{
		if(DoMethod( obj, MUIM_Group_InitChange ))
		{
			Object *child,*cstate = (Object *)childList->lh_Head;
			
			while((child = NextObject(&cstate)))
			{
				if(data->UnDo->obj == child)
				{
					STRPTR filename;
					
					DoMethod( child, MUIM_GUIGFX_IMAGE, &filename );
					InfoText(0,"Removed image \"%s\"", FilePart( filename ));
					
					data->width -= data->UnDo->width;
					DoMethod( obj, OM_REMMEMBER, child );
					MUI_DisposeObject( child );
					done = TRUE;
					break;
				}
			}
			
			DoMethod( obj, MUIM_Group_ExitChange );
		}
	}
	
	if( done == TRUE )
	{
		struct UndoList *n, *c = data->UnDo;
		
		n = c->next;
		
		FreeMem( c, sizeof(struct UndoList));
		data->UnDo = n;
	}
	else DisplayBeep(NULL);
	
	return(done);
}

STATIC VOID FreeUndoList(struct IClass *cl, Object *obj)
{
	struct DropableClassData * data = INST_DATA(cl,obj);
	struct UndoList * c, * n;
	
	for( c = data->UnDo ; c ; c = n )
	{
		n = c->next;
		FreeMem( c, sizeof(struct UndoList));
	}
}

STATIC BOOL DropImage(struct IClass *cl, Object *obj, STRPTR filename, long DropPosX )
{
	Object * new;
	struct DropableClassData * data = INST_DATA(cl,obj);
	
	DBG("filename=\"%s\", DroPosX = %ld\n", FilePart( filename ),DropPosX);
	
	if(filename && *filename && (new = GuiGfxPic(filename)))
	{
		UWORD width = 0;
		struct List *childList = NULL;
		BOOL JustAdd = TRUE;
		
		data->LastAdded = new;
		
		DoMethod( obj, MUIM_Group_InitChange );
		DoMethod( obj, OM_REMMEMBER, data->space );
		MUI_DisposeObject( data->space );
		
		// DropPosX is lower than 0 if we are loading a project
		if( DropPosX > 0 )
			childList = (struct List *)xget(obj, MUIA_Group_ChildList);
		
		if( childList && DropPosX < (long)data->width )
		{
			// Image droped between some already droped imgs
			int x = 0;
			Object * childs[99], * child, 
				* cstate = (Object *)childList->lh_Head;
			
			// remove image's objs to later reorganize them
			while((child = NextObject(&cstate)))
			{
			    if(data->space != child)
				DoMethod( obj, OM_REMMEMBER, childs[x++] = child );
			}
			childs[x] = NULL;
			
			if( x > 0 )
			{
				// assuming all images has the same width, which
				// condition MUST be, but we dont check it at 
				// this step, but creating the final ToolBar
				width = (x ? (data->width / x) : data->width);
				
				DBG("Sorting, objs = %ld, width = %ld\n",x,width);
				
				for( x = 0 ; childs[x] && DropPosX > width ; x++ )
				{
					DoMethod( obj, OM_ADDMEMBER, childs[x] );
					
					DropPosX -= width;
				}
				
				DoMethod( obj, OM_ADDMEMBER, new );
				
				for( ; childs[x] ; x++ )
					DoMethod( obj, OM_ADDMEMBER, childs[x] );
				
				JustAdd = FALSE;
			}
		}
		
		if( JustAdd )
			DoMethod( obj, OM_ADDMEMBER, new );
		
		DoMethod( obj, OM_ADDMEMBER, data->space = HVSpace );
		DoMethod( obj, MUIM_Group_ExitChange);
		//MUI_Redraw( obj, MADF_DRAWUPDATE );
		
		// count widths for all images, hmm, may is faster getting the
		// GuiGfxPic's picture and using PICATTR_Width on it ..?
		if(PictureDimension( filename, &width, NULL,NULL))
			data->width += width;
		
		AddUndoItem( data, new, width );
		
		data->WasProjectSaved = FALSE;
		return TRUE;
	}
	DisplayBeep(NULL);
	return FALSE;
}

STATIC ULONG DragDrop(struct IClass *cl, Object *obj, struct MUIP_DragDrop *msg)
{
	STRPTR filename = NULL;
	
	DoMethod( msg->obj, MUIM_GUIGFX_IMAGE, &filename );
	
	if(DropImage(cl,obj,filename, msg->x - _window(obj)->LeftEdge - _mleft(obj)))
	{
		InfoText(0,"Added image \"%s\"", FilePart( filename ));
	}
	else
	{
		InfoText(0,"ERROR adding \"%s\"", FilePart( filename ));
	}
	
	return(0);
}

DISPATCHERPROTO(DropableClass_Dispatcher)
{
	switch(msg->MethodID)
	{
		case OM_NEW:
		{
			struct DropableClassData * data;
			struct opSet * xmsg = (struct opSet *) msg;
			Object * space;
			
			obj = (Object *)DoSuperNew(cl,obj,
				ReadListFrame,
				MUIA_Dropable, TRUE,
				MUIA_Group_Horiz, TRUE,
				MUIA_Background, MUII_ReadListBack,
				GroupSpacing(1),
				Child, space = HGroup,
					Child, TextObject,
						MUIA_Text_Contents,(ULONG) "\n Drop images on me!\n",
					End,
					Child, HVSpace,
				End,
			TAG_MORE, xmsg->ops_AttrList);
			
			if(!obj || !(data = INST_DATA(cl,obj)))
				return 0;
			
			data->width = 0;
			data->space = space;
			data->WasProjectSaved = TRUE;
			data->UnDo = NULL;
			
		}	return((ULONG)obj);
		
		case OM_DISPOSE:
		{
			FreeUndoList(cl,obj);
		}	break;
		
		case MUIM_DragQuery:
			return MUIV_DragQuery_Accept;
		case MUIM_DragDrop:
			return(DragDrop(cl,obj,(struct MUIP_DragDrop *) msg));
		
		case MUIM_CreateTB:
		{
			struct List *childList = (struct List *)xget(obj, MUIA_Group_ChildList);
			struct DropableClassData * data = INST_DATA(cl,obj);
			BOOL done = FALSE;
			
			DBG(" -- MUIM_CreateTB --\n");
			
			set( app, MUIA_Application_Sleep, TRUE );
			
			DBG_ASSERT(childList != NULL);
			if(childList)
			{
				Object *cstate = (Object *)childList->lh_Head;
				Object *child;
				long imgs = 0;
				
				while((child = NextObject(&cstate)))
				{
					STRPTR filename = NULL;
					
					DoMethod( child, MUIM_GUIGFX_IMAGE, &filename );
					
					if(filename && *filename)
					{
						data->files[imgs++] = filename;
						data->files[ imgs ] = NULL;
						
						DBG("%03ld: \"%s\"\n",imgs-1,filename);
					}
				}
				
				if(imgs > 0)
				{
					STATIC CTBData ctbdata;
					
					bzero( &ctbdata, sizeof(ctbdata));
					
					ctbdata.DestMode	= (UBYTE) xget(PMode,MUIA_Cycle_Active);
					ctbdata.Precision	= (UBYTE) xget(Precision,MUIA_Cycle_Active);
					ctbdata.Masking		= (UBYTE) xget(Masking,MUIA_Cycle_Active);
					ctbdata.Compression	= (UBYTE) xget(Compression,MUIA_Cycle_Active);
					ctbdata.BltMinterm	= GetStringInt(BltMinterm,"$C0");
					ctbdata.BltMask		= GetStringInt(BltMask,"$FF");
					ctbdata.imgs = imgs;
					ctbdata.files = data->files;
					
					if(xget( Remap, MUIA_Selected ))
						ctbdata.scr = (APTR) xget( win, MUIA_Window_Screen );
					
					CreateToolBar( &ctbdata );
					
					done = TRUE;
				}
			}
			
			if( done == FALSE )
				DisplayBeep(NULL);
			
			set( app, MUIA_Application_Sleep, FALSE );
			DBG_VALUE(done);
			
		}	return TRUE;
		
		case MUIM_IsProjectSaved:
		{
			struct DropableClassData * data = INST_DATA(cl,obj);
			
			if(data->WasProjectSaved)
				return TRUE;
			
			if(!ask("Current Project isn't saved,\ndo you want to continue?"))
				return FALSE;
			
			return data->WasProjectSaved = TRUE;
		}	break;
		
		case MUIM_NewProject:
		{
			struct DropableClassData * data = INST_DATA(cl,obj);
			struct List *childList;
			Object *cstate, *child;
			
			if(!DoMethod( obj, MUIM_IsProjectSaved))
				return FALSE;
			
			if(!(childList = (struct List *)xget(obj, MUIA_Group_ChildList)))
				return FALSE;
			
			cstate = (Object *)childList->lh_Head;
			
			DoMethod( obj, MUIM_Group_InitChange );
			
			while((child = NextObject(&cstate)))
			{
				DoMethod( obj, OM_REMMEMBER, child );
				MUI_DisposeObject( child );
			}
			
			DoMethod( obj, OM_ADDMEMBER, data->space = HGroup,
					Child, TextObject,
						MUIA_Text_Contents,(ULONG) "\n Drop images on me!\n",
					End,
					Child, HVSpace,
				End );
			DoMethod( obj, MUIM_Group_ExitChange );
			data->width = 0;
			
			InfoText(100,"New Project Ready.");
			
		}	return TRUE;
		
		case MUIM_LoadProject:
		case MUIM_SaveProject:
		{
			struct DropableClassData * data = INST_DATA(cl,obj);
			STRPTR filename;
			BOOL saveMode, error = TRUE;
			
			if(!(saveMode = (msg->MethodID == MUIM_SaveProject)))
			{
				if(!DoMethod( obj, MUIM_IsProjectSaved))
					return FALSE;
				
				DoMethod( obj, MUIM_NewProject );
			}
			set( app, MUIA_Application_Sleep, TRUE );
			
			if((filename = AslFile(saveMode ? "Save Project To...":"Load Project From...","#?.tbp")))
			{
				BPTR fd;
				#define IDENT	160
				
				if(saveMode)
				{
					// AslFile allocates plus 32 bytes
					
					int x = strlen( filename );
					
					filename[x++] = '.';
					filename[x++] = 't';
					filename[x++] = 'b';
					filename[x++] = 'p';
					filename[ x ] =  0 ;
				}
				
				if((fd = Open( filename, saveMode ? MODE_NEWFILE:MODE_OLDFILE)))
				{
					if(saveMode)
					{
						struct List *childList = (struct List *)xget(obj, MUIA_Group_ChildList);
						Object *cstate = (Object *)childList->lh_Head, * child;
						
						FPutC( fd, IDENT ); // identifier
						
						while((child = NextObject(&cstate)))
						{
							STRPTR file = NULL;
							
							DoMethod( child, MUIM_GUIGFX_IMAGE, &file );
							
							if(file && *file)
								FPrintf( fd, "%s\n",(long) file );
						}
						
						error = FALSE;
					}
					else if(FGetC( fd ) != IDENT)
					{
						ShowError("This file seems not a project created by me...");
					}
					else
					{
						UBYTE line[4096];
						
						error = FALSE;
						while(FGets( fd, line, sizeof(line)-1))
						{
							if(!*line)
								continue;
							
							*&line[strlen(line)-1] = 0;
							
							if(!DropImage(cl,obj,line,-1))
							{
								ShowError("Cannot drop image \"%s\"!", line );
								error = TRUE;
							}
						}
					}
					
					Close( fd );
				}
				else ShowError("Cannot open \"%s\"!", filename );
				
				FreeVec( filename );
			}
			
			data->WasProjectSaved = TRUE;
			set( app, MUIA_Application_Sleep, FALSE );
			
			if( error )
			{
				InfoText(100,"ERROR %s project.",
					saveMode ? "saving":"loading");
			}
			else
			{
				InfoText(100,"Project %s successfully.",
					saveMode ? "saved":"loaded");
			}
			
		}	return TRUE;
		
		case MUIM_UndoImage:
		{
			UndoImage( cl, obj );
		}	return TRUE;
	}
	return(DoSuperMethodA(cl, obj, msg));
}


/***************************************************************************/

struct GuiGfxClassData
{
	STRPTR filename;
};

#define SCALEMODEMASK(u,d,p,s) (((u)?NISMF_SCALEUP:0)|((d)?NISMF_SCALEDOWN:0)|((p)?NISMF_KEEPASPECT_PICTURE:0)|((s)?NISMF_KEEPASPECT_SCREEN:0))
#define ScaleUp		TRUE
#define ScaleDown	TRUE
#define PicAspect	FALSE
#define ScreenAspect	FALSE
//#define TransMask	FALSE
//#define TransColor	FALSE

DISPATCHERPROTO(GuiGfxClass_Dispatcher)
{
	switch(msg->MethodID)
	{
		case OM_NEW:
		{
			STRPTR filename;
			struct GuiGfxClassData * data;
			struct opSet * xmsg = (struct opSet *) msg;
			long quality;
			
		#if !USE_LG_PICD
			BOOL TransMask = FALSE;
			BOOL TransColor = FALSE;
			
			if(xget(Masking,MUIA_Cycle_Active)==1)
				TransMask = TRUE;
			
			if(xget(Masking,MUIA_Cycle_Active)==2)
				TransColor = TRUE;
		#endif /* USE_LG_PICD */
			
			quality = xget(GGQCyc,MUIA_Cycle_Active);
			
			obj = (Object *)DoSuperNew(cl,obj,
				InnerSpacing(0,0),
				GroupSpacing( 0 ),
				MUIA_FixWidth, 32,
				MUIA_FixHeight, 32,
				MUIA_Draggable, TRUE,
				MUIA_Guigfx_Quality, quality,
			#if !USE_LG_PICD
				MUIA_Guigfx_ScaleMode,SCALEMODEMASK(ScaleUp,ScaleDown,PicAspect,ScreenAspect),
				MUIA_Guigfx_Transparency,(TransMask ? NITRF_MASK:0) | (TransColor ? NITRF_RGB : 0),
			#endif
			TAG_MORE, xmsg->ops_AttrList);
			
			if(!obj || !(data = INST_DATA(cl,obj)))
				return 0;
			
			filename = (STRPTR)GetTagData(MUIA_Guigfx_FileName,0,xmsg->ops_AttrList);
			
			if(filename && *filename)
			{
				long len = strlen(filename);
				
				if((data->filename = AllocVec(len+1,MEMF_ANY)))
				{
					CopyMem( filename, data->filename, len );
					data->filename[len] = 0;
					
					set( obj, MUIA_ShortHelp,(ULONG)FilePart(data->filename));
				}
			}
		}	return((ULONG) obj );
		
		case OM_DISPOSE:
		{
			struct GuiGfxClassData * data = INST_DATA(cl,obj);
			if(data->filename)
				FreeVec(data->filename);
		}	break;
		
		case MUIM_GUIGFX_IMAGE:
		{
			struct GuiGfxClassData * data = INST_DATA(cl,obj);
			ULONG * store = MARG1;
			*store = (ULONG) data->filename;
		}	return TRUE;
	}
	return(DoSuperMethodA(cl, obj, msg));
}

/***************************************************************************/

void InfoText( UWORD c, STRPTR fmt, ... )
{
	STATIC UBYTE text[4096];
	
	if(fmt != NULL)
	{
		if(*fmt != 0)
		{
			va_list args;
			
			va_start (args, fmt);
			VSNPrintf( text, sizeof(text)-1, fmt, args );
			va_end(args);
		}
		else *text = 0;
	}
	
	DBG_STRING(text);
	SetAttrs( gauge, 
		MUIA_Gauge_InfoText,(ULONG) text,
		MUIA_Gauge_Current, (ULONG) c,
	TAG_DONE);
}

/***************************************************************************/

#define ID_CREATETB	0x78995
#define ID_REFRESHLG	0x78996

STATIC BOOL RefreshLayGroup( VOID )
{
	Object * parent;
	STRPTR folder;
	BOOL rc = FALSE;
	
	ENTER();
	DBG_POINTER(lGroup);
	
	parent = (Object *) xget( lGroup, MUIA_Parent );
	DBG_POINTER(parent);
	DBG_ASSERT(parent != NULL);
	
	if( parent != NULL )
	{
		set( app, MUIA_Application_Sleep, TRUE );
		
		DoMethod( parent, MUIM_Group_InitChange);
		DoMethod( parent, OM_REMMEMBER, lGroup );
		MUI_DisposeObject( lGroup );
		
		InfoText(0,"Loading Images, please wait...");
		
		folder = (STRPTR)xget(imagesFolder,MUIA_String_Contents);
		if(!(folder && *folder))
		{
			DBG("Using default folder...\n");
			folder = (STRPTR) DefaultImagesFolder;
			set(imagesFolder,MUIA_String_Contents,(ULONG)folder);
		}
		
		lGroup = LayGroup(folder);
		DBG_ASSERT(lGroup != NULL);
		DBG_POINTER(lGroup);
		
		if( lGroup == NULL )
			lGroup = HVSpace;
		
		DoMethod( parent, OM_ADDMEMBER, lGroup );
		DoMethod( parent, MUIM_Group_ExitChange);
		
		//InfoText(0,"");
		set( app, MUIA_Application_Sleep, FALSE );
		rc = TRUE;
	}
	
	RETURN(rc);
	return(rc);
}

/***************************************************************************/

STATIC Object * FloatText(UBYTE left, CONST_STRPTR text)
{
	return ListviewObject,
		MUIA_Weight, left ? 100:180,
		//left ? MUIA_FixWidth:TAG_IGNORE,1,
		MUIA_FixHeight, 1,
		MUIA_Listview_Input, FALSE,
		MUIA_Listview_ScrollerPos, MUIV_Listview_ScrollerPos_None,
		MUIA_Listview_List, FloattextObject, 
			MUIA_Frame, MUIV_Frame_None, //MUIV_Frame_ReadList,
			//MUIA_Background, MUII_ReadListBack,
			MUIA_Background,(ULONG)"2:ffffffff,ffffffff,ffffffff",
			MUIA_Floattext_Text,(ULONG) text,
			MUIA_Floattext_TabSize, 4,
			//MUIA_Floattext_Justify, TRUE,
		End,
	End;
}

STATIC Object * MyCheckmark( ULONG ObjectID, STRPTR ShortHelp )
{
	Object *obj;
	
	if((obj = MUI_MakeObject(MUIO_Checkmark, NULL)))
	{
		SetAttrs(obj,
			MUIA_CycleChain	, TRUE,
			MUIA_ShortHelp	, (ULONG)ShortHelp,
			MUIA_ObjectID	, ObjectID,
		TAG_DONE);
	}
	
	return(obj);
}

STATIC Object *MyCycle( ULONG ObjectID, STRPTR *array, STRPTR ShortHelp )
{
	Object *obj;
	
	if((obj = MUI_MakeObject(MUIO_Cycle, NULL, array)))
	{
		SetAttrs(obj,
			MUIA_CycleChain	, TRUE,
			MUIA_ShortHelp	, (ULONG) ShortHelp,
			MUIA_ObjectID	, ObjectID,
		TAG_DONE);
	}
	
	return(obj);
}

STATIC Object * MyString( STRPTR ShortHelp, ULONG MaxLen, ULONG ObjectID )
{
	Object * obj;
	
	if((obj = MUI_MakeObject(MUIO_String, NULL, MaxLen)))
	{
		SetAttrs(obj,
			MUIA_CycleChain		, TRUE,
			MUIA_String_AdvanceOnCR	, TRUE,
			MUIA_ShortHelp		, (ULONG) ShortHelp,
			MUIA_ObjectID		, ObjectID,
		TAG_DONE);
	}
	
	return(obj);
}

/***************************************************************************/

int main( VOID )
{
	int rc = 1;
	Object * DropObj, * Toolbar, * PrefsWIN, * AboutWIN;
	
	if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME,MUIMASTER_VMIN)))
	{
		ShowError("Cannot open %s v%ld",MUIMASTER_NAME,MUIMASTER_VMIN);
		goto done;
	}
	
	if(!(DropableClass = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct DropableClassData), ENTRY(DropableClass_Dispatcher))))
	{
		DBG("Cannot Create DropableCustomClass..\n");
		goto done;
	}
	
	UtilityBase   = (APTR)DropableClass->mcc_UtilityBase;
	DOSBase       = (APTR)DropableClass->mcc_DOSBase;
	GfxBase       = (APTR)DropableClass->mcc_GfxBase;
	IntuitionBase = (APTR)DropableClass->mcc_IntuitionBase;
	
	if(!(DataTypesBase = OpenLibrary("datatypes.library", 43)))
	{
		ShowError("Cannot open %s v%ld","datatypes.library", 43);
		goto done;
	}
	
	if(!(GuiGfxClass = MUI_CreateCustomClass(NULL, MUIC_Guigfx, NULL, sizeof(struct GuiGfxClassData), ENTRY(GuiGfxClass_Dispatcher))))
	{
		ShowError("cannot create GuiGfx's CustomClass");
		goto done;
	}
	
	app = ApplicationObject,
		MUIA_Application_Title      , ApplicationData[0],
		MUIA_Application_Version    , ApplicationData[1],
		MUIA_Application_Copyright  , ApplicationData[2] + 21,
		MUIA_Application_Author     , ApplicationData[2] + 29,
		MUIA_Application_Description, ApplicationData[2],
		MUIA_Application_Base       , ApplicationData[0],
		
		SubWindow, win = WindowObject,
			MUIA_Window_Title, ApplicationData[2],
			MUIA_Window_ID, MAKE_ID('A','I','S','S'),
			MUIA_Window_UseRightBorderScroller, TRUE,
			WindowContents, VGroup,
				MUIA_Group_SameWidth, TRUE,
				MUIA_Background, MUII_WindowBack,
				Child, HGroup,
					Child, Toolbar = EmbedToolbar(ToolbarBank),
					Child, HVSpace,
				End,
				Child, ColGroup(2), GroupFrame,
					MUIA_Background, MUII_GroupBack,
					Child, Label2("TBImages:"),
					Child, imagesFolder = PopaslObject,
						MUIA_Popstring_String, BetterStringObject, StringFrame,
							MUIA_String_AdvanceOnCR		, TRUE,
							MUIA_String_MaxLen		, 1024,
							MUIA_String_Contents		, (ULONG) DefaultImagesFolder,
							MUIA_ObjectID			, MAKE_ID('I','M','G','S'),
						End,
						MUIA_Popstring_Button, PopButton(MUII_PopDrawer),
						ASLFR_TitleText, "Select a folder with some Toolbar Images",
						ASLFR_InitialDrawer, (ULONG)DefaultImagesFolder,
						ASLFR_DrawersOnly, TRUE,
					End,
				End,
				Child, gauge = GaugeObject,
					GaugeFrame, 
					MUIA_FixHeightTxt, "XYZ",
					MUIA_Gauge_Horiz, TRUE,
				End,
				Child, DropObj = NewObject(DropableClass->mcc_Class, NULL,TAG_DONE),
				Child, lGroup = HVSpace,
			End,
		End,
		SubWindow, PrefsWIN = WindowObject,
			MUIA_Window_Title, "Settings",
			MUIA_Window_ID, MAKE_ID('P','W','I','N'),
			WindowContents, VGroup,
				MUIA_Background, MUII_WindowBack,
				Child, ColGroup(2),
					GroupFrameT("Datatypes"),
					Child, Label1("Preview _Quality:"),
					Child, GGQCyc = MyCycle(MAKE_ID('Q','C','Y','C'),GGQuality,"Defines the quality for MainWindow's GuiGfx.mcc preview images"),
					Child, Label1("_DestMode:"),
					Child, PMode = MyCycle(MAKE_ID('M','O','D','E'),PModes,"Which PDTA_DestMode do you want to use to \nload the images (at toolbar creation time)"),
					Child, Label1("_Precision:"),
					Child, Precision = MyCycle(MAKE_ID('P','R','E','C'),PrecisionTypes,"Which OBP_Precision, on the same way as PDTA_DestMode"),
					Child, Label1("_Masking:"),
					Child, Masking = MyCycle(MAKE_ID('M','A','S','K'),MaskTypes,"Masking technique to save the final toolbar"),
					Child, Label1("_Compression:"),
					Child, Compression = MyCycle(MAKE_ID('C','O','M','P'),CmpTypes,"Compression technique to the final toolbar"),
					Child, HVSpace,
					Child, HGroup,
						Child, Remap = MyCheckmark(MAKE_ID('R','M','A','P'),"Remap the toolbar to the curent screen's colors"),
						Child, Label1("_Remap"),
						Child, HVSpace,
					End,
				End,
				Child, VSpace(2),
				Child, ColGroup(2),
					GroupFrameT("BltBitMap"),
					Child, Label1("Minterm"),
					Child, BltMinterm = MyString("The logic function to apply to the rectangle",9,MAKE_ID('M','T','R','M')),
					Child, Label1("Mask"),
					Child, BltMask = MyString("The write mask to apply.\nTypically this is set to $ff",9,MAKE_ID('B','M','S','K')),
				End,
				
				Child, AslFilePath = StringObject,
					MUIA_ShowMe, FALSE,
					MUIA_ObjectID, MAKE_ID('A','S','L','P'),
				End,
			End,
		End,
		SubWindow, AboutWIN = WindowObject,
			MUIA_Window_Title, "About This Program",
			MUIA_Window_ID, MAKE_ID('A','W','I','N'),
			WindowContents, VGroup,
				MUIA_Background,(ULONG)"2:ffffffff,ffffffff,ffffffff",
				InnerSpacing(0,0),
				Child, AISSLogo ( ),
				Child, TextObject,
					MUIA_Text_Contents, ApplicationData[1] + 5,
					MUIA_Text_PreParse, "\033b\033c",
					MUIA_Font, MUIV_Font_Big,
				End,
				Child, TextObject,
					MUIA_Text_Contents, ApplicationData[2] + 21,
					MUIA_Text_PreParse, "\033c",
				End,
				Child, TextObject,
					MUIA_Text_Contents, "All Rights Reserved",
					MUIA_Text_PreParse, "\033c",
				End,
				Child, TextObject,
					MUIA_Text_Contents, "\n   Special Thanks To   ",
					MUIA_Text_PreParse, "\033c\033u",
					MUIA_Font, MUIV_Font_Inherit,
				End,
				Child, ColGroup(2),
					InnerSpacing(8,8),
					GroupSpacing( 2 ),
					Child, FloatText(1,"Stefan Stuntz"),
					Child, FloatText(0,"Author of the Magic User Interface used to create this program."),
					Child, FloatText(1,"Martin Merz"),
					Child, FloatText(0,"By creating AppMI and AISS, The best graphic icons available for AmigaOS."),
					Child, FloatText(1,"Alessandro Zummo"),
					Child, FloatText(0,"For the nice LayGroup.mcc, used to display the grouped preview icons."),
					Child, FloatText(1,"Matthias Bethke and TEK"),
					Child, FloatText(0,"Who created GuiGfx.mcc and guigfx.library respectively."),
					Child, FloatText(1,"Benny Kjær Nielsen and Alfonso Ranieri"),
					Child, FloatText(0,"For ToolBar.mcc and TheBar.mcc respectively, The MUI classes this program is mainly helpful to."),
					//Child, FloatText(1,"girlfriend"),
					//Child, FloatText(0,"By leaving out allowing me to waste more time on coding =)"),
				End,
			End,
		End,
	End;
	
	if( ! app ) {
		ShowError("Failed creating MUI Application!");
		goto done;
	}
	
	DoMethod(win, MUIM_Notify,MUIA_Window_CloseRequest, TRUE,
		app,2,MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);
	
	DoMethod(PrefsWIN, MUIM_Notify,MUIA_Window_CloseRequest, TRUE,
		PrefsWIN, 3, MUIM_Set, MUIA_Window_Open, FALSE );
	DoMethod(AboutWIN, MUIM_Notify,MUIA_Window_CloseRequest, TRUE,
		AboutWIN, 3, MUIM_Set, MUIA_Window_Open, FALSE );
	
	DoMethod( GGQCyc, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
		app, 2, MUIM_Application_ReturnID, ID_REFRESHLG );
	
	DoMethod( Toolbar, MUIM_Toolbar_Notify, TB_CREATE,	MUIV_Toolbar_Notify_Pressed, FALSE, DropObj, 1, MUIM_CreateTB );
	DoMethod( Toolbar, MUIM_Toolbar_Notify, TB_NEW,		MUIV_Toolbar_Notify_Pressed, FALSE, DropObj, 1, MUIM_NewProject );
	DoMethod( Toolbar, MUIM_Toolbar_Notify, TB_UNDO,	MUIV_Toolbar_Notify_Pressed, FALSE, DropObj, 1, MUIM_UndoImage );
	DoMethod( Toolbar, MUIM_Toolbar_Notify, TB_LOAD,	MUIV_Toolbar_Notify_Pressed, FALSE, DropObj, 1, MUIM_LoadProject );
	DoMethod( Toolbar, MUIM_Toolbar_Notify, TB_SAVE,	MUIV_Toolbar_Notify_Pressed, FALSE, DropObj, 1, MUIM_SaveProject );
	DoMethod( Toolbar, MUIM_Toolbar_Notify, TB_PREFS,	MUIV_Toolbar_Notify_Pressed, FALSE, PrefsWIN, 3, MUIM_Set, MUIA_Window_Open, TRUE );
	DoMethod( Toolbar, MUIM_Toolbar_Notify, TB_ABOUT,	MUIV_Toolbar_Notify_Pressed, FALSE, AboutWIN, 3, MUIM_Set, MUIA_Window_Open, TRUE );
	
	DoMethod( imagesFolder, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		app, 2, MUIM_Application_ReturnID, ID_REFRESHLG );
	
	set( BltMinterm, MUIA_String_Contents,(ULONG) "$C0");
	set( BltMask, MUIA_String_Contents,(ULONG) "$FF");
	
	DoMethod( app, MUIM_Application_Load, MUIV_Application_Load_ENVARC );
	
	SetAttrs(win,MUIA_Window_Open,TRUE,TAG_DONE);
	
	if(xget( win, MUIA_Window_Open))
	{
		ULONG sigs = 0, mid;
		struct Window * window;
		BOOL stop = FALSE;
		
		if((window = (struct Window *)xget(win, MUIA_Window_Window)))
			((struct Process *)FindTask(0))->pr_WindowPtr = window;
		
		if(!xget(GGQCyc,MUIA_Cycle_Active))
			DoMethod( app, MUIM_Application_ReturnID, ID_REFRESHLG );
		
		do {
			mid = DoMethod(app,MUIM_Application_NewInput,&sigs);
			
			switch( mid )
			{
				case MUIV_Application_ReturnID_Quit:
					if(DoMethod( DropObj, MUIM_IsProjectSaved))
						stop = TRUE;
					break;
				
				case ID_REFRESHLG:
					RefreshLayGroup ( ) ;
					break;
				
				default:
					break;
			}
			if(sigs && !stop)
			{
				sigs = Wait(sigs | SIGBREAKF_CTRL_C);
				if( sigs & SIGBREAKF_CTRL_C )
				{
					DBG(" +++ Signal Interrupt\n");
					break;
				}
			}
		} while(!stop);
		
		SetAttrs(win,MUIA_Window_Open,FALSE,TAG_DONE);
		DoMethod( app, MUIM_Application_Save, MUIV_Application_Save_ENVARC );
		
		rc = 0;
		
		DBG("Exiting Application...\n");
	}
	else ShowError("Failed opening MUI Window!");
	
done:
	if(app)
		MUI_DisposeObject(app);
	if(DropableClass)
		MUI_DeleteCustomClass(DropableClass);
	if(DataTypesBase)
		CloseLibrary(DataTypesBase);
	if(GuiGfxClass)
		MUI_DeleteCustomClass(GuiGfxClass);
	if (MUIMasterBase)
		CloseLibrary(MUIMasterBase);
	
	return(rc);
}

/***************************************************************************/

