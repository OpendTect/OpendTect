/*+
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

static const char* rcsID = "$Id: genc.c,v 1.6 2001-03-30 10:27:21 bert Exp $";

#include "genc.h"
#include "filegen.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef __win__
#include <unistd.h>
#endif

int GetSurveyName_reRead = NO;
static FileNameString filenamebuf;
static FileNameString surveyname;
static int surveynameinited = NO;


const char* GetSoftwareDir()
{
    return getenv( "dGB_APPL" );
}


const char* GetDataFileName( const char* fname )
{
    if ( !fname ) return 0;

    strcpy( filenamebuf, File_getFullPath( GetSoftwareDir(), "data" ) );
    strcpy( filenamebuf, File_getFullPath( filenamebuf, fname ) );
    return filenamebuf;
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
	ptr = getenv( "dGB_USER" );
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
    const char* ptr;

    if ( GetSurveyName_reRead || !surveynameinited )
    {
	ptr = GetSurveyFileName();
	if ( !ptr ) return 0;

	fp = fopen( ptr, "r" );
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
    const char* ptr;
    if ( !getenv( "dGB_DATA" ) ) return 0;
    ptr = GetSurveyName();
    if ( !ptr ) return getenv( "dGB_DATA" );

    strcpy( filenamebuf, File_getFullPath(getenv("dGB_DATA"),ptr) );
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


int getPID()
{
    return
#ifdef __win__
	getpid_();
#else
	getpid();
#endif
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

int isFinite( double v )
{
    return finite(v);
}
