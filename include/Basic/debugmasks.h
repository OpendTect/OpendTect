#ifndef debugmasks_h
#define debugmasks_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Lammertink
 Date:		Jun 2003
 RCS:		$Id: debugmasks.h,v 1.5 2004-12-16 10:34:22 bert Exp $
________________________________________________________________________

-*/

#include "debug.h"

/* 

    This is a list of reserved debug masks, on order to avoid conflicts.

    DON'T JUST THROW ANY OF THESE AWAY OR CHANGE A VALUE!!!

    Adding new masks should be OK.

*/

#define	DBG_DBG		0x0001	// general, low frequency stuff
#define	DBG_MT		0x0002	// multi-threaded stuff
#define	DBG_UI		0x0004	// ui-related stuff
#define	DBG_IO		0x0008	// general I/O stuff
#define	DBG_SOCKIO	0x0010	// socket I/O
#define	DBG_MM		0x0020	// Multi-machine batch processing
#define	DBG_SETTINGS	0x0040	// User settings
#define	DBG_PROGSTART	0x0080	// Program start and stop

#endif
