/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 3-5-1994
 * FUNCTION : Functions for time
-*/

static const char* rcsID = "$Id: timefun.cc,v 1.17 2009-07-22 16:03:40 cvsbert Exp $";

#include "timefun.h"

#include <time.h>
#include <string.h>

#if defined (sun5) || defined(mac)
# define __notimeb__ 1
#endif

#ifndef __notimeb__
# ifdef __win__
#  include <windows.h>
# endif
# include <sys/timeb.h>
#endif

#ifdef __notimeb__
static time_t tim;
#else
# ifdef __win__
static struct _timeb timebstruct;
# else
static struct timeb timebstruct;
# endif
#endif
static struct tm* ptrtm;


static struct tm* getLocal( void )
{
#ifdef __notimeb__

    (void)time( &tim ) ;
    return localtime( &tim );

#else

# ifdef __win__

    (void)_ftime( &timebstruct ) ;

# else

    (void)ftime( &timebstruct ) ;

# endif

    return localtime( &timebstruct.time );

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
    + ptrtm->tm_hour	* 3600000;

}


int Time_passedSince( int timestamp )
{
    int elapsed = timestamp > 0 ? Time_getMilliSeconds() - timestamp : -1;
    if ( elapsed < 0 && timestamp > 0 && (elapsed + 86486400 < 86400000) )
	elapsed += 86486400;

    return elapsed;
} 


const char* Time_getFullDateString( void )
{
    char *chp ;
    int lastch ;

    ptrtm = getLocal() ;
    chp = asctime( ptrtm ) ;

    lastch = strlen( chp ) - 1 ;
    if ( chp[lastch] == '\n' ) chp[lastch] = '\0' ;

    return chp;
}


const char* Time_getTimeString( void )
{
    static char buf[32];
    char* ptrbuf = buf;
    const char* ptrfull;

    ptrfull = strchr( Time_getFullDateString(), ':' );
    while ( !isspace(*(ptrfull-1)) )
	ptrfull--;
    while ( !isspace(*ptrfull) )
	*ptrbuf++ = *ptrfull++;

    return buf;
}


void Time_sleep( double s )
{
#ifdef __notimeb__

    if ( s > 0 ) sleep( mNINT(s) );

#else

# ifdef __win__

    double ss = s*1000;
    Sleep( (DWORD)mNINT(ss) );

# else

    struct timespec ts;
    if ( s <= 0 ) return;

    ts.tv_sec = (time_t)s;
    ts.tv_nsec = (long)((((double)s - ts.tv_sec) * 1000000000L) + .5);

    nanosleep( &ts, &ts );

# endif

#endif
}
