#ifndef timefun_H
#define timefun_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		3-5-1994
 Contents:	Time functions
 RCS:		$Id: timefun.h,v 1.1.1.2 1999-09-16 09:19:22 arend Exp $
________________________________________________________________________

@$*/

#include <gendefs.h>

#ifdef __cpp__
extern "C" {
#endif

int		Time_getMilliSeconds	Pargs( (void) ) ;
const char*	Time_getLocalString	Pargs( (void) ) ;
int		Time_get4Digits		Pargs( (void) ) ;
int		Time_get6Digits		Pargs( (void) ) ;

#ifdef __cpp__
}
#endif


/*$-*/
#endif
