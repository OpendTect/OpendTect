#ifndef prog_h
#define prog_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		5-12-1995
 RCS:		$Id: prog.h,v 1.11 2004-12-15 16:45:45 bert Exp $
________________________________________________________________________

 Include this file in any executable program you make. The file is actually
 pretty empty ....

-*/

#include "plugins.h"

#ifdef __cpp__
extern "C" {
#endif
    void		od_putProgInfo(int,char**);
    			//!< One line; other info is only put if DBG::isOn()
#ifdef __cpp__
}
#endif


#endif
