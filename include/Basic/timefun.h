#ifndef timefun_H
#define timefun_H

/*@+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		3-5-1994
 Contents:	Time functions
 RCS:		$Id: timefun.h,v 1.4 2003-11-07 12:21:51 bert Exp $
________________________________________________________________________

-*/

#include <gendefs.h>

#ifdef __cpp__
extern "C" {
#endif

int		Time_getMilliSeconds(void);	/*!< From day start */
const char*	Time_getLocalString(void);	/*!< local time in static buf */
void		Time_sleep(double);		/*!< in seconds */

#ifdef __cpp__
}
#endif


#endif
