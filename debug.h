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


#ifndef DEBUG_H
#define DEBUG_H

#include <proto/dos.h>
#include <clib/debug_protos.h>

#ifdef DEBUG
static __inline void __dEbuGpRInTF( CONST_STRPTR func_name, CONST LONG line )
{
	if(DOSBase != NULL) {
		struct DateStamp ds;
		
		DateStamp(&ds);
		
		KPrintF("[%02ld:%02ld:%02ld] ",
			ds.ds_Minute/60, ds.ds_Minute%60, ds.ds_Tick/TICKS_PER_SECOND);
	}
	else {
		KPutStr("[--:--:--] ");
	}
	KPutStr( func_name );
	KPrintF(",%ld: ", line );
}
# define DBG( fmt... ) \
({ \
	__dEbuGpRInTF(__PRETTY_FUNCTION__, __LINE__); \
	KPrintF( fmt ); \
})
# define DBG_POINTER( ptr )	DBG("%s = 0x%08lx\n", #ptr, (long) ptr )
# define DBG_VALUE( val )	DBG("%s = 0x%08lx,%ld\n", #val,val,val )
# define DBG_STRING( str )	DBG("%s = 0x%08lx,\"%s\"\n", #str,str,str)
# define DBG_ASSERT( expr )	if(!( expr )) DBG(" **** FAILED ASSERTION '%s' ****\n", #expr )
# define DBG_EXPR( expr, bool )						\
({									\
	BOOL res = (expr) ? TRUE : FALSE;				\
									\
	if(res != bool)							\
		DBG("Failed %s expression for '%s'\n", #bool, #expr );	\
									\
	res;								\
})
#else
# define DBG(fmt...)		((void)0)
# define DBG_POINTER( ptr )	((void)0)
# define DBG_VALUE( ptr )	((void)0)
# define DBG_STRING( str )	((void)0)
# define DBG_ASSERT( expr )	((void)0)
# define DBG_EXPR( expr, bool )	expr
#endif

#define DBG_TRUE( expr )	DBG_EXPR( expr, TRUE )
#define DBG_FALSE( expr )	DBG_EXPR( expr, FALSE)

#define ENTER()		DBG("--- ENTERING %s\n", __func__)
#define LEAVE()		DBG("--- LEAVING %s\n", __func__)
#define RETURN(Rc)	\
	DBG("--- LEAVING %s RC=%ld,0x%08lx\n", __func__, (LONG)(Rc),(LONG)(Rc))

#endif /* DEBUG_H */
