/*+
 * NAME     : %M%
 * AUTHOR   : A.H. Bril
 * DATE     : 3-5-1994
 * FUNCTION : Functions for time
-*/

static const char* rcsID = "$Id: timefun.cc,v 1.3 2000-03-03 12:05:20 bert Exp $";

#include "timefun.h"
#include <time.h>

#if defined(__ibm__) || defined(__sgi__) || defined (__sun__)
# define __notimeb__ 1
#endif
#ifndef __notimeb__
#include <sys/timeb.h>
#endif

#ifdef __notimeb__
static time_t tim;
#else
static struct timeb timebstruct;
#endif
static struct tm* ptrtm;


static struct tm* getLocal( void )
{
#ifdef __notimeb__
    (void)time( &tim ) ;
    return localtime( &tim );
#else
    (void)ftime( &timebstruct ) ;
    return( localtime( &timebstruct.time ) ) ;
#endif
}


int Time_getMilliSeconds( void )
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


const char* Time_getLocalString( void )
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


void Time_sleep( double s )
{
#ifdef __notimeb__

    if ( s > 0 ) sleep( mNINT(s) );

#else

    struct timespec ts;
    if ( s <= 0 ) return;

    ts.tv_sec = (time_t)s;
    ts.tv_nsec = (long)((((double)s - ts.tv_sec) * 1000000000L) + .5);

    nanosleep( &ts, &ts );

#endif
}
