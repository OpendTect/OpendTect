/*+
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

static const char* rcsID = "$Id: genc.c,v 1.15 2003-03-27 12:49:59 bert Exp $";

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
static int dgb_application_code = mDgbApplCodeGDI;


int GetDgbApplicationCode()
{
    return dgb_application_code;
}


int SurveyNameDirty()
{
    return surveynamedirty;
}


void SetSurveyNameDirty()
{
    surveynamedirty = 1;
}


void SetDgbApplicationCode( int newnr )
{
    dgb_application_code = newnr;
}


const char* GetSoftwareDir()
{
    const char* dir = getenv( "dGB_APPL" );
    if ( !dir )
	dir = dgb_application_code == mDgbApplCodeDTECT ?
		    getenv( "dTECT_APPL" ) : getenv( "GDI_APPL" );
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
    const char* ptr = getenv( "dGB_USER" );
    if ( !ptr )
	ptr = dgb_application_code == mDgbApplCodeDTECT ?
	    	getenv( "dTECT_USER" ) : getenv( "GDI_USER" );
    return ptr;
}

const char* GetHomeDir()
{
#ifdef __msvc__
    static FileNameString home = "";
    if( !*home )
    {
	strcpy( home, getenv("HOMEDRIVE") );
	strcat( home, getenv("HOMEPATH") );
    }
    return home;
#else
    const char* ptr = getenv( "HOME" );
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


const char* GetBaseDataDir()
{
    const char* dir = getenv( "dGB_DATA" );
    if ( !dir )
	dir = dgb_application_code == mDgbApplCodeDTECT ?
		    getenv( "dTECT_DATA" ) : getenv( "GDI_DATA" );
    return dir;
}


const char* GetDataDir()
{
    const char* survnm;
    const char* basedir = GetBaseDataDir();
    if ( !basedir ) return 0;

    survnm = GetSurveyName();
    if ( !survnm ) return basedir;

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
#ifdef lux
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
