/*+
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

static const char* rcsID = "$Id: genc.c,v 1.18 2003-10-09 12:45:05 bert Exp $";

#include "genc.h"
#include "filegen.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef __win__
#include <unistd.h>
#else
#include <process.h>
#include <float.h>
#endif

static FileNameString filenamebuf;
static FileNameString surveyname;
static int surveynamedirty = YES;


int SurveyNameDirty()
{
    return surveynamedirty;
}


void SetSurveyNameDirty()
{
    surveynamedirty = 1;
}


const char* GetSoftwareDir()
{
    const char* dir = 0;
#ifdef __win__

    dir = getenv( "DTECT_WINAPPL" );
    if ( !dir ) dir = getenv( "dGB_WINAPPL" );

#endif

    if ( !dir ) dir = getenv( "DTECT_APPL" );
    if ( !dir ) dir = getenv( "dGB_APPL" );

    return dir;
}


const char* GetDataFileName( const char* fname )
{
    strcpy( filenamebuf, File_getFullPath( GetSoftwareDir(), "data" ) );
    if ( fname && *fname )
	strcpy( filenamebuf, File_getFullPath( filenamebuf, fname ) );
    return filenamebuf;
}


const char* GetSoftwareUser()
{
    const char* ptr = 0;
#ifdef __win__

    ptr = getenv( "DTECT_WINUSER" );
    if ( !ptr ) ptr = getenv( "dGB_WINUSER" );

#endif

    if ( !ptr ) ptr = getenv( "DTECT_USER" );
    if ( !ptr ) ptr = getenv( "dGB_USER" );

    return ptr;
}

const char* GetHomeDir()
{
#ifdef __win__

    static FileNameString home = "";

    const char* ptr = getenv( "DTECT_WINHOME" );
    if ( !ptr ) ptr = getenv( "dGB_WINHOME" );

    if ( ptr && *ptr ) return ptr;
    {
	strcpy( home, getenv("HOMEDRIVE") );
	strcat( home, getenv("HOMEPATH") );
    }
    return home;

#else

    const char* ptr = getenv( "DTECT_HOME" );
    if ( !ptr ) ptr = getenv( "dGB_HOME" );
    if ( !ptr ) ptr = getenv( "HOME" );
    return ptr;

#endif
}


const char* GetSurveyFileName()
{
    static FileNameString sfname;
    static int inited = NO;

    if ( !inited )
    {
	const char* ptr = GetHomeDir();
	if ( !ptr ) return 0;
	strcpy( sfname, File_getFullPath(ptr,".dgbSurvey") );
	ptr = GetSoftwareUser();
	if ( ptr )
	{
	    strcat( sfname, "." );
	    strcat( sfname, ptr );
	}
	inited = YES;
    }
    return sfname;
}


void SetSurveyName( const char* newnm )
{
    strcpy( surveyname, newnm );
    surveynamedirty = 0;
}


const char* GetSurveyName()
{
    int len;
    FILE* fp;
    const char* fnm;

    if ( surveynamedirty )
    {
	fnm = GetSurveyFileName();
	if ( !fnm ) return 0;

	fp = fopen( fnm, "r" );
	if ( !fp ) return 0;

	surveyname[0] = '\0';
	fgets( surveyname, PATH_LENGTH, fp );
	len = strlen( surveyname );
	if ( len == 0 ) return 0;
	if ( surveyname[len-1] == '\n' ) surveyname[len-1] = '\0';

	fclose( fp );
	surveynamedirty = 0;
    }
    return surveyname;
}


extern const char* GetSettingsDataDir();

const char* GetBaseDataDir()
{
    const char* dir = 0;
#ifdef __win__

    dir = getenv( "DTECT_WINDATA" );
    if ( !dir ) dir = getenv( "dGB_WINDATA" );

#endif

    if ( !dir ) dir = getenv( "DTECT_DATA" );
    if ( !dir ) dir = getenv( "dGB_DATA" );

    if ( !dir ) dir = GetSettingsDataDir();
    return dir;
}


const char* GetDataDir()
{
    const char* survnm;
    const char* basedir = GetBaseDataDir();
    if ( !basedir || !*basedir ) return 0;

    survnm = GetSurveyName();
    if ( !survnm || !*survnm ) return basedir;

    strcpy( filenamebuf, File_getFullPath(basedir,survnm) );
    return filenamebuf;
}


void swap_bytes( void* p, int n )
{
    int nl = 0;
    unsigned char* ptr = (unsigned char*)p;
    unsigned char c;

    if ( n < 2 ) return;
    n--;
    while ( nl < n )
    { 
	c = ptr[nl]; ptr[nl] = ptr[n]; ptr[n] = c;
	nl++; n--;
    }
}


void put_platform( unsigned char* ptr )
{
#if defined(lux) || defined(__win__)
    *ptr = 1;
#else
    *ptr = 0;
#endif
}

#ifdef __msvc__
#define getpid	_getpid
#endif

int getPID()
{
    return getpid();
}


double IntPowerOf( double x, int y )
{
    double ret = 1;
    if ( x == 0 ) return y ? 0 : 1;

    while ( y )
    {
	if ( y > 0 )
	    { ret *= x; y--; }
	else
	    { ret /= x; y++; }
    }
    return ret;
}


double PowerOf( double x, double y )
{
    int isneg = x < 0 ? 1 : 0;
    double ret;
 
    if ( x == 0 ) return y ? 0 : 1;
    if ( isneg ) x = -x;
 
    ret = exp( y * log(x) );
    return isneg ? -ret : ret;
}


#ifdef sun5
# include <ieeefp.h>
#endif

#ifdef __msvc__
# define finite	_finite
#endif

int isFinite( double v )
{
    return finite(v);
}
