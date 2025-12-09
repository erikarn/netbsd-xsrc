/* 
 * Id: newport_shadow.c,v 1.3 2000/11/29 20:58:10 agx Exp $
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/newport/newport_shadow.c,v 1.2 2001/11/23 19:50:45 dawes Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "newport.h"

/*
 * Note: the shadow framebuffer code assumes that it entirely
 * controls the drawstate; it can't be called in parallel with
 * the acceleration code.
 */

void
NewportRefreshArea8(ScrnInfoPtr pScrn, int num, BoxPtr pbox)
{
	int dx, dy, x;
	CARD32 *base, *src;
	NewportPtr pNewport = NEWPORTPTR(pScrn);
	NewportRegsPtr pNewportRegs = pNewport->pNewportRegs;

#define RA8_BYTES	4	/* burst 4 pixels each time */
#define RA8_BYTE_SHIFT  2 	/* 4 Pixels on each burst, so divide ShadowPitch by 4 */
#define RA8_MASK        0xffc   /* move to 4 byte boundary   */

	NEWPORT_DPRINTF(pNewport, NEWPORT_DBG_SHADOWFB_CALLS,
	    "%s: called; num=%d\n", __func__, num);

	NewportWait(pNewportRegs);
	pNewportRegs->set.drawmode0 = (NPORT_DMODE0_DRAW | 
					NPORT_DMODE0_BLOCK | 
					NPORT_DMODE0_CHOST);
	while(num--) {
		NewportWait(pNewportRegs);
		x = pbox->x1 & RA8_MASK;  	/* move x to 4 byte boundary */
		base = pNewport->ShadowPtr 
				+ (pbox->y1 * (pNewport->ShadowPitch >> RA8_BYTE_SHIFT) ) 
				+ ( x >> RA8_BYTE_SHIFT);

		NEWPORT_DPRINTF(pNewport, NEWPORT_DBG_SHADOWFB_REGIONS,
		    "%s: --> x1=%d y1=%d x2=%d y2=%d\n",
		    __func__, pbox->x1, pbox->y1, pbox->x2, pbox->y2);

		pNewportRegs->set.xystarti = (x << 16) | pbox->y1;
		pNewportRegs->set.xyendi = ((pbox->x2-1) << 16) | (pbox->y2-1);

		for ( dy = pbox->y1; dy < pbox->y2; dy++) {

			src = base;
			for ( dx = x; dx < pbox->x2; dx += RA8_BYTES) {
				pNewportRegs->go.hostrw0 = *src;  
				src++;
			}
			base += ( pNewport->ShadowPitch >> RA8_BYTE_SHIFT );
		}
		pbox++;
	}

	NEWPORT_DPRINTF(pNewport, NEWPORT_DBG_SHADOWFB_CALLS,
	    "%s: finished\n", __func__);
}


void
NewportRefreshArea24(ScrnInfoPtr pScrn, int num, BoxPtr pbox)
{
	int dx, dy;
	CARD8 *src, *base;
#ifndef NEWPORT_USE32BPP	
	CARD32 dest;
#endif	
	NewportPtr pNewport = NEWPORTPTR(pScrn);
	NewportRegsPtr pNewportRegs = pNewport->pNewportRegs;

	NEWPORT_DPRINTF(pNewport, NEWPORT_DBG_SHADOWFB_CALLS,
	    "%s: called; num=%d\n", __func__, num);

	NewportWait(pNewportRegs);

	/* block transfers */
	pNewportRegs->set.drawmode0 = (NPORT_DMODE0_DRAW | 
					NPORT_DMODE0_BLOCK | 
					NPORT_DMODE0_CHOST);

	while(num--) {
		NEWPORT_DPRINTF(pNewport, NEWPORT_DBG_SHADOWFB_REGIONS,
		    "%s: --> x1=%d y1=%d x2=%d y2=%d\n",
		    __func__, pbox->x1, pbox->y1, pbox->x2, pbox->y2);

		base = (CARD8*)pNewport->ShadowPtr + pbox->y1 * pNewport->ShadowPitch + pbox->x1 
#ifdef NEWPORT_USE32BPP		
		* 4;
#else
		* 3;
#endif		
		pNewportRegs->set.xystarti = (pbox->x1 << 16) | pbox->y1;
		pNewportRegs->set.xyendi = ((pbox->x2-1) << 16) | (pbox->y2-1);
		
		for ( dy = pbox->y1; dy < pbox->y2; dy++) {
			src = base;
			for ( dx = pbox->x1 ;  dx < pbox->x2 ; dx++) {
				/* Removing these shifts by using 32bpp fb
				 * yields < 2% percent performance gain and wastes 25% memory 
				 */
#ifdef NEWPORT_USE32BPP
				pNewportRegs->go.hostrw0 = *(CARD32 *)src;
				src += 4;
#else
				dest = src[0] | src[1] << 8 | src[2] << 16;
				pNewportRegs->go.hostrw0 = dest;	
				src+=3;
#endif				
 			}
			base += pNewport->ShadowPitch;
 		}
 		pbox++;
 	}

	NEWPORT_DPRINTF(pNewport, NEWPORT_DBG_SHADOWFB_CALLS,
	    "%s: finished\n", __func__);
}

