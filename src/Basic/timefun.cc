/*+
 * NAME     : %M%
 * AUTHOR   : A.H. Bril
 * DATE     : 3-5-1994
 * FUNCTION : Functions for time
-*/

static const char* rcsID = "$Id: timefun.cc,v 1.1.1.1 1999-09-03 10:11:27 dgb Exp $";

/*@+
\section{Project Time}
%====================================

This module contains functions to handle Time.

@$*/

#include "timefun.h"
#include <time.h>

#if defined(__ibm__) || defined(__sgi__) || defined (__sun__)
# define __notimeb__ 1
#endif
#ifndef __notimeb__
#include <sys/timeb.h>
#endif

/*$-*/

static struct tm* getLocal( Pnoargs ) ;

#ifdef __notimeb__
static time_t tim;
#else
static struct timeb timebstruct;
#endif
static struct tm* ptrtm;

/*@+
\subsection{The routines in detail}
%==================================

Here is a more detailed description of all the routines used in this
Time module:
@-*/


/*--------------------------------------------------@+Time_getMilliSeconds
 gets the time from day start in milliseconds.
\list{{\b Return}}the amount of millisecs
-----------------------------------------------------------------@-*/
int Time_getMilliSeconds( Pnoargs )
{
	ptrtm = getLocal();

	return
#ifdef __notimeb__
	0
#else
	timebstruct.millitm
#endif
	+ ptrtm->tm_sec	* 1000
	+ ptrtm->tm_min	* 60000
	+ ptrtm->tm_hour* 3600000;
}


/*--------------------------------------------------@+Time_getLocalString
 gets the local time as a string.
\list{{\b Return}}ptr to output string. Must be strcpy'd immediately
-----------------------------------------------------------------@-*/
const char* Time_getLocalString( Pnoargs )
{
	char *chp ;
	int lastch ;

	ptrtm = getLocal() ;
	chp = asctime( ptrtm ) ;

	lastch = strlen( chp ) - 1 ;
	if ( chp[lastch] == '\n' )
		chp[lastch] = '\0' ;

	return( chp ) ;
}


/*--------------------------------------------------@+Time_get4Digits
 and Time_get6Digits get the time in 4/6 digits (hhmm or hhmmss) as int.
\list{{\b Return}}the requested time
-----------------------------------------------------------------@-*/
int Time_get4Digits( Pnoargs ) { return( Time_get6Digits()/100 ) ; }
int Time_get6Digits( Pnoargs )
{
	ptrtm = getLocal() ;
	return( 10000*ptrtm->tm_hour + 100*ptrtm->tm_min + ptrtm->tm_sec ) ;
}


/*--------------------------------------------------@+getLocal
 (static) gets the local time from OS.
\list{{\b Return}}the time struct
\NOTE: As a side-effect, the variable timebstruct is updated.
-----------------------------------------------------------------@-*/
static struct tm* getLocal( Pnoargs )
{
#ifdef __notimeb__
	(void)time( &tim ) ;
	return localtime( &tim );
#else
	(void)ftime( &timebstruct ) ;
	return( localtime( &timebstruct.time ) ) ;
#endif
}
