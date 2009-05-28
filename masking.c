
// Thanks to Thomas Rapp for this code!

#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/datatypes.h>
#include <datatypes/pictureclass.h>
#include <cybergraphx/cybergraphics.h>

/*

parameters:

  srcbm   - bitmap to create mask from
  w,h     - size of picture
  x,y     - position in the picture where to fetch transparent color from (usually 0,0)

result:

  maskbm  - bitmap containing mask

*/
STATIC struct BitMap *create_mask (struct BitMap *srcbm,long w,long h,long bgx,long bgy)
{
	struct BitMap *maskbm;
	long d,x,y;
	long bpr;
	struct RastPort rp;
	struct TmpRas tmpras;
	UBYTE *tmpbuf;
	ULONG rassize;
	struct RastPort temprp;
	struct BitMap *tempbm;
	UBYTE *array = NULL,*yp,*xp,*p;
	long tpen;
	ULONG trgb;
	ULONG rgb;

	if((maskbm = AllocBitMap (w,h,2,BMF_CLEAR,NULL))) /* depth 2 because we create two masks (see backfill code below) */
	{
		InitRastPort (&rp);
		rp.BitMap = srcbm;

		if((tempbm = AllocBitMap (w,h,1,0,NULL)))
		{
			struct Library *CyberGfxBase;
			
			temprp = rp;                 /* for Read/WritePixelArray8 as documented in autodocs */
			temprp.Layer = NULL;
			temprp.BitMap = tempbm;

			bpr = ((w + 15) >> 4) << 4;    /* bytes per row for Read/WritePixelArray8 */

			d = GetBitMapAttr(srcbm,BMA_DEPTH);
			if (d <= 8)
			{
				if((array = AllocVec (bpr * h,0)))
				{
					ReadPixelArray8 (&rp,0,0,w-1,h-1,array,&temprp);

					tpen = array[bgy * bpr + bgx];   /* fetch transparent pen */

					yp = array;
					for (y = 0; y < h; y++)
					{
						xp = yp;
						for (x = 0; x < w; x++)
						{
							*xp++ = (*xp == tpen ? 0 : 1); /* set transparent pixels to 0, others to 1 */
						}
						yp += bpr;
					}
				}
			}
			else if ((CyberGfxBase = OpenLibrary ("cybergraphics.library",0)))
			{
				if((array = AllocVec (w * h * 3,0)))
				{
					ReadPixelArray (array,0,0,w*3,&rp,0,0,w,h,RECTFMT_RGB);

					p = &array[(bgy * w + bgx) * 3];            /* pointer to background color */
					trgb = (*p++ << 16) | (*p++ << 8) | (*p++); /* get transparent RGB value */

					p = array;
					yp = array;

				/* note that array is used in two different ways at the same
				   time here: three bytes per pixel (RGB) as taken from
				   ReadPixelArray are read and one byte per pixel (pen number)
				   is written to prepare WritePixelArray8. This can be done
				   because the read pointer is faster than the write pointer,
				   so the writes do not destroy data not yet read.
   				*/
					for (y = 0; y < h; y++)
					{
						xp = yp;
						for (x = 0; x < w; x++)
							{
							rgb = (*p++ << 16) | (*p++ << 8) | (*p++);
							*xp++ = (rgb == trgb ? 0 : 1);
							}
						yp += bpr;
					}
				}
				
				CloseLibrary(CyberGfxBase);
			}

			if (array)
			{
				rp.BitMap = maskbm;

				WritePixelArray8 (&rp,0,0,w-1,h-1,array,&temprp); /* write 0/1 pixels into maskbm => bitplane 0 contains mask */

				FreeVec (array);

		/* the following is only needed for backfill */

				rassize = RASSIZE(w,h);
				if((tmpbuf = AllocVec (rassize,MEMF_CHIP|MEMF_CLEAR|MEMF_PUBLIC)))
				{
					InitTmpRas (&tmpras,tmpbuf,rassize);
					rp.TmpRas = &tmpras;
				}

				SetAPen (&rp,2);
				Flood (&rp,1,bgx,bgy);  /* fill area around mask with pen 2 => bitplane 1 contains filled background */

				BltBitMap (maskbm,0,0,maskbm,0,0,w,h,ABNC|ANBNC,0x02,NULL);   /* invert bitplane 1 => bp 1 contains mask without holes in it */

				FreeVec (tmpbuf);

		/* end of backfill code */

			}

			FreeBitMap (tempbm);
		}
	}

	return (maskbm);
}

static void free_mask (struct BitMap *bm)
{
	FreeBitMap (bm);
}


struct BitMap *BltBitMask(struct BitMap *srcbm,UWORD width,UWORD height,UBYTE depth,BOOL backfill)
{
	struct BitMap *maskbm;
	struct RastPort temprp;
	struct BitMap *destbm = NULL;
	
	InitRastPort (&temprp);
	temprp.BitMap = srcbm;
	
	if((maskbm = create_mask(srcbm,width,height,0,0)))
	{
	#define destbm maskbm
	//	BltBitMapRastPort(maskbm,0,0,&temprp,0,0,width,height,ABC|ABNC);
		BltMaskBitMapRastPort(destbm,0,0,&temprp,0,0,width,height,ABC|ABNC|ANBC,maskbm->Planes[!backfill ? 1 : 0]);
		//BltBitMap( maskbm, 0, 0, srcbm, width, 0, width, height, ABC|ABNC|ANBC, maskbm->Planes[backfill ? 1 : 0], NULL);
		
		free_mask (maskbm);
	}
	
	#undef destbm
	destbm = temprp.BitMap;
	
	return(destbm);
}
