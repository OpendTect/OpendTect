#ifndef timefun_h
#define timefun_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		3-5-1994
 Contents:	Time functions
 RCS:		$Id: timefun.h,v 1.11 2009-07-22 16:01:14 cvsbert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"

#ifdef __cpp__
extern "C" {
#endif

mGlobal int Time_getMilliSeconds(void);		/*!< From day start */
mGlobal int Time_passedSince(int); 		/*!< in millisecs */

mGlobal const char* Time_getFullDateString(void); /*!< full date/time */
mGlobal const char* Time_getTimeString(void);  	/*!< "hh::mm::ss" */

mGlobal  void Time_sleep(double);       	/*!< in seconds */

#ifdef __cpp__
}
#endif


#endif
