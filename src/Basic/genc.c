/*+
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

static const char* rcsID = "$Id: genc.c,v 1.8 2001-10-16 08:34:31 bert Exp $";

#include "genc.h"
#include "filegen.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef __win__
#include <unistd.h>
#endif

int dgb_application_code = 1; // 1 = GDI, 2 = dTect
int GetSurveyName_reRead = NO;
static FileNameString filenamebuf;
static FileNameString surveyname;
static int surveynameinited = NO;


const char* GetSoftwareDir()
{
    const char* dir = dgb_application_code == 2 ? getenv( "dTECTAPPL" ) : 0;
    if ( !dir ) dir = getenv( "dGB_APPL" );
    return dir;
}


const char* GetDataFileName( const char* fname )
{
    if ( !fname ) return 0;

    strcpy( filenamebuf, File_getFullPath( GetSoftwareDir(), "data" ) );
    strcpy( filenamebuf, File_getFullPath( filenamebuf, fname ) );
    return filenamebuf;
}


const char* GetSoftwareUser()
{
    const char* ptr = dgb_application_code == 2 ? getenv( "dTECTUSER" ) : 0;
    if ( !ptr ) ptr = getenv( "dGB_USER" );
    return ptr;
}


const char* GetSurveyFileName()
{
    static FileNameString sfname;
    static int inited = NO;
    const char* ptr;

    if ( !inited )
    {
	ptr = getenv( "HOME" );
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
    surveynameinited = 1;
    GetSurveyName_reRead = NO;
}


const char* GetSurveyName()
{
    int len;
    FILE* fp;
    const char* fnm;

    if ( GetSurveyName_reRead || !surveynameinited )
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
	surveynameinited = YES;
    }
    return surveyname;
}


const char* GetDataDir()
{
    const char* survnm;
    const char* datadir = dgb_application_code == 2 ? getenv( "dTECTDATA" ) : 0;
    if ( !datadir ) datadir = getenv( "dGB_DATA" );
    if ( !datadir ) return 0;

    survnm = GetSurveyName();
    if ( !survnm ) return datadir;

    strcpy( filenamebuf, File_getFullPath(datadir,survnm) );
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
    *ptr =

#ifdef sun5
	0;
#endif
#ifdef sgi
	2;
#else
	1;
#endif

}

#ifdef __msvc__
#define getpid	_getpid
#endif

int getPID()
{
    return
	getpid();
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
