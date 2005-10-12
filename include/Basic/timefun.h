#ifndef timefun_H
#define timefun_H

/*@+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		3-5-1994
 Contents:	Time functions
 RCS:		$Id: timefun.h,v 1.6 2005-10-12 12:57:59 cvsarend Exp $
________________________________________________________________________

-*/

#include <gendefs.h>

#ifdef __cpp__
extern "C" {
#endif

int		Time_getMilliSeconds(void);	/*!< From day start */
int             Time_passed_since(int);		/*!< in millisecs */

const char*	Time_getFullDateString(void);	/*!< full date/time */
const char*	Time_getTimeString(void);	/*!< "hh::mm::ss" */

void		Time_sleep(double);		/*!< in seconds */

#ifdef __cpp__
}
#endif


#endif
