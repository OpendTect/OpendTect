#ifndef timefun_H
#define timefun_H

/*@+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		3-5-1994
 Contents:	Time functions
 RCS:		$Id: timefun.h,v 1.9 2008-12-24 12:48:55 cvsranojay Exp $
________________________________________________________________________

-*/

#include <gendefs.h>

#ifdef __cpp__
extern "C" {
#endif

mGlobal int Time_getMilliSeconds(void);	/*!< From day start */
mGlobal int Time_passedSince(int); 		/*!< in millisecs */

mGlobal const char* Time_getFullDateString(void);     /*!< full date/time */
mGlobal const char* Time_getTimeString(void);  	/*!< "hh::mm::ss" */

mGlobal  void Time_sleep(double);       		/*!< in seconds */

#ifdef __cpp__
}
#endif


#endif
