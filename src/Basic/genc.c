/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

static const char* rcsID = "$Id: genc.c,v 1.55 2004-11-29 10:57:25 bert Exp $";

#include "genc.h"
#include "filegen.h"
#include "winutils.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef __win__
# include <unistd.h>
# define sDirSep	"/"
#else
//# include <process.h>
# include <float.h>
//# include "windows.h"
# define sDirSep	"\\"
# include "string2.h"

#endif

#ifdef __mac__
# include <CoreServices/CoreServices.h>
#endif

#include "debugmasks.h"
static FileNameString surveyname;
static int surveynamedirty = YES;
static const char* dirsep = sDirSep;


int SurveyNameDirty()
{
    return surveynamedirty;
}


void SetSurveyNameDirty()
{
    surveynamedirty = 1;
}


static const char* mkFullPath( const char* path, const char* filename )
{
    static FileNameString pathbuf;
    char* chptr;
    int lastpos;

    if ( path != pathbuf )
	strcpy( pathbuf, path && *path ? path : "." );

    if ( !filename || !*filename ) return pathbuf;

    /* Remove trailing dirseps from pathbuf */
    chptr = pathbuf; while ( *chptr ) chptr++; chptr--;
    while ( chptr != pathbuf-1 && *chptr == *dirsep ) *chptr-- = '\0';

    chptr = (char*)pathbuf;
    lastpos = strlen( chptr ) - 1;
    if ( lastpos >= 0 && chptr[lastpos] != *dirsep )
	strcat( chptr, dirsep );

    strcat( chptr, filename );
    return chptr;
}


const char* GetSoftwareDir()
{
    static char* cachedDir = 0;
    if ( cachedDir ) return cachedDir;

    const char* dir = 0;

#ifndef __win__

    dir = getenv( "DTECT_APPL" );
    if ( !dir ) dir = getenv( "dGB_APPL" );

#else

    dir = getenv( "DTECT_WINAPPL" );
    if ( !dir ) dir = getenv( "dGB_WINAPPL" );

    if ( !dir ) dir = getCleanWinPath( getenv("DTECT_APPL") );
    if ( !dir ) dir = getCleanWinPath( getenv("dGB_APPL") );

#if 0
    if ( !dir )
    {
	TCHAR szPath[_MAX_PATH];
	if( GetModuleFileName(NULL,szPath,_MAX_PATH) ) 
//.....
// TODO  : extract DTECT_APPL from full executable path.

    }
#endif

    if ( dir && *dir && !getenv("DTECT_WINAPPL") )
	setEnvVar( "DTECT_WINAPPL" , dir );

#endif

#ifdef __mac__
    if ( !dir )
    {	// Get location of 'bundle'
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef url = CFBundleCopyBundleURL(mainBundle);
	CFStringRef cfStr = CFURLCopyPath(url);

	const char* bundlepath =
	    CFStringGetCStringPtr(cfStr, CFStringGetSystemEncoding());

	static FileNameString progname;
	strcpy( progname, bundlepath );

	if ( *progname )
	{
	    dir = progname;
	    setEnvVar( "DTECT_APPL" , dir );
	}
    }
#endif

#ifdef __lux__
// TODO : use /proc/self/exe symlink to find full path to current running exe
#endif

    if ( !dir )
    {
	if ( !getenv("DTECT_ARGV0") ) return 0;

	static FileNameString progname;
	strcpy( progname, getenv("DTECT_ARGV0") );

	if( !*progname ) return 0;


	char* chptr1 = progname;
	char* chptr2 = chptr1;
	while ( chptr2 = strstr( chptr1 + 1 , "bin" ) )
	    chptr1 = chptr2;

	if ( !chptr1 ) return 0;

	*chptr1-- = '\0';

	/* Remove trailing dirseps from pathbuf */
	while ( chptr1 != progname-1 && *chptr1 == *dirsep ) *chptr1-- = '\0';

	dir = progname;
    }

    if( od_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetSoftwareDir: %s\n", dir );
	od_debug_message( buf );
    }

    cachedDir = mMALLOC( strlen(dir)+ 1, char );
    strcpy( cachedDir, dir );

    return cachedDir;
}


const char* GetBinDir()
{
    static FileNameString bindir;

    strcpy( bindir, GetSoftwareDir() );
    strcpy( bindir, mkFullPath(bindir, "bin") );

    return bindir;
}


const char* GetFullPathForExec( const char* exec )
{
    static FileNameString progname;

    strcat( progname, GetBinDir() );

    if ( exec && *exec )
	strcpy( progname, mkFullPath(progname, exec) );

    return progname;
}


const char* GetExecScript( int remote )
{
    static FileNameString progname;

    strcpy( progname, "'" );

    strcat( progname, GetBinDir() );

    strcpy( progname, mkFullPath(progname, "od_exec") );

    if( remote )
	strcat( progname, "_rmt" );

    strcat( progname, "' " );
    return progname;
}


const char* GetDataFileName( const char* fname )
{
    static FileNameString filenamebuf;
    strcpy( filenamebuf, mkFullPath( GetSoftwareDir(), "data" ) );
    if ( fname && *fname )
	strcpy( filenamebuf, mkFullPath( filenamebuf, fname ) );

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetDataFileName for %s: %s\n", fname ? fname : "(null)",
			filenamebuf );
	od_debug_message( buf );
    }

    return filenamebuf;
}

static const char* checkFile( const char* path, const char* subdir,
			      const char* fname )
{
    static FileNameString filenamebuf;
    if ( !path || !subdir || !fname ) return 0;

    strcpy( filenamebuf, mkFullPath( path, subdir ) );
    if ( fname && *fname )
	strcpy( filenamebuf, mkFullPath( filenamebuf, fname ) );

    if ( File_exists(filenamebuf) )
	return filenamebuf;
 
    return 0;
}

const char* SearchODFile( const char* fname )
{ // NOTE: recompile SearchODFile in spec/General when making changes here...

    const char* nm = checkFile( getenv("OD_FILES"), "", fname );
    if ( !nm ) nm = checkFile( GetPersonalDir(), ".od", fname );
    if ( !nm ) nm = checkFile( GetSettingsDir(), ".od", fname );
    if ( !nm ) nm = checkFile( getenv("ALLUSERSPROFILE"), ".od", fname );
    if ( !nm ) nm = checkFile( GetSoftwareDir(), "data", fname );
    if ( !nm ) nm = checkFile( GetSoftwareDir(), "bin", fname );
    if ( !nm ) nm = checkFile( GetSoftwareDir(), "", fname );
    if ( !nm ) nm = checkFile( GetBaseDataDir(), "", fname );

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "SearchODFile for %s: %s\n", fname ? fname : "(null)",
			nm );
	od_debug_message( buf );
    }

    return nm;
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

    if( od_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetSoftwareUser: %s\n", ptr ? ptr : "Not set" );
	od_debug_message( buf );
    }

    return ptr;
}

const char* _GetHomeDir()
{
#ifndef __win__

    const char* ptr = getenv( "DTECT_HOME" );
    if ( !ptr ) ptr = getenv( "dGB_HOME" );
    if ( !ptr ) ptr = getenv( "HOME" );
    return ptr;

#else

    static FileNameString home = "";

    const char* ptr = getenv( "DTECT_WINHOME" );
    if ( !ptr ) ptr = getenv( "dGB_WINHOME" );

    if ( !ptr ) ptr = getCleanWinPath( getenv("DTECT_HOME") );
    if ( !ptr ) ptr = getCleanWinPath( getenv("dGB_HOME") );
    if ( !ptr ) ptr = getCleanWinPath( getenv("HOME") );

    if ( ptr && *ptr )
    {
	strcpy( home, ptr );
	if ( !getenv("DTECT_WINHOME") )
	    setEnvVar( "DTECT_WINHOME" , home );
	return home;
    }

    if ( !getenv("HOMEDRIVE") || !getenv("HOMEPATH") ) return 0;

    strcpy( home, getenv("HOMEDRIVE") );
    strcat( home, getenv("HOMEPATH") );

    if( strcmp( home, "" ) && strcmp( home, "c:\\" ) && strcmp( home, "C:\\" ) 
	&& File_isDirectory( home ) ) // Apparantly, home has been set...
    {
	return home;
    }

    return 0;

#endif
}

const char* GetSettingsDir(void)
{ // NOTE: recompile SearchODFile in spec/General when making changes here...

    const char* ptr = getenv( "OD_SETTINGS_DIR" );

#ifndef __win__

    if ( !ptr )
	ptr = _GetHomeDir();
#else    

    if ( !ptr && getenv("OD_PREFER_HOME") )
	ptr = _GetHomeDir();

    if ( !ptr )
	ptr = getenv( "APPDATA" ); // should be set by OS

    if ( !ptr )
	ptr = getenv( "DTECT_APPLICATION_DATA" ); // set by init script

    if ( !ptr )
	ptr = _GetHomeDir();

    if ( !ptr ) // Last resort. Is known to cause problems when used 
                // during initialisation of statics. (0xc0000005)
	ptr = GetSpecialFolderLocation( CSIDL_APPDATA ); // "Application Data"

    if ( !ptr )
	return 0;

    char* chptr = (char*)ptr;
    while ( chptr && *chptr++ ) { if ( *chptr == '\r' ) *chptr='\0'; }

#endif

    ptr = mkFullPath( ptr, ".od" );

    if ( !File_isDirectory(ptr) )
    {
	if ( File_exists(ptr) ) ptr = 0;
	else if ( !File_createDir(ptr,0) ) ptr = 0;
    }

    if( od_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetSettingsDir: %s\n", ptr );
	od_debug_message( buf );
    }

    return ptr;
}


const char* GetPersonalDir(void)
{ // NOTE: recompile SearchODFile in spec/General when making changes here...

    const char* ptr = getenv( "OD_PERSONAL_DIR" );

    if ( !ptr )
	ptr = _GetHomeDir();

#ifdef __win__

    if ( !ptr ) 
	ptr = getenv( "USERPROFILE" ); // should be set by OS

    if ( !ptr ) 
	ptr = getenv( "DTECT_USERPROFILE_DIR" );

    if ( !ptr ) 
	ptr = getenv( "DTECT_MYDOCUMENTS_DIR" );

    if ( !ptr )
	ptr = _GetHomeDir();

    if ( !ptr ) // Last resort. Is known to cause problems when used 
                // during initialisation of statics. (0xc0000005)
	ptr = GetSpecialFolderLocation( CSIDL_PROFILE ); // "User Profile"

    char* chptr = (char*)ptr;
    while ( chptr && *chptr++ ) { if ( *chptr == '\r' ) *chptr='\0'; }

#endif

    if( od_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetPersonalDir: %s\n", ptr );
	od_debug_message( buf );
    }

    return ptr;
}


const char* GetSurveyFileName()
{
    static FileNameString sfname;
    static int inited = NO;
    const char* ptr;

    if ( !inited )
    {
	ptr = GetSettingsDir();
	if ( !ptr ) return 0;
	strcpy( sfname, mkFullPath(ptr,"survey") );
	ptr = GetSoftwareUser();
	if ( ptr )
	{
	    strcat( sfname, "." );
	    strcat( sfname, ptr );
	}
	inited = YES;
    }

    if( od_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetSurveyFileName: %s\n", sfname );
	od_debug_message( buf );
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
	if ( !fnm )
	{
	    if( od_debug_isOn(DBG_SETTINGS) )
	    {
		char buf[255];
		sprintf(buf,
		    "GetSurveyName: GetSurveyFileName returned NULL\n" );
		od_debug_message( buf );
	    }
	    return 0;
	}

	fp = fopen( fnm, "r" );
	if ( !fp )
	{
	    if( od_debug_isOn(DBG_SETTINGS) )
	    {
		char buf[255];
		sprintf(buf,
		    "GetSurveyName: Could not open SurveyFile: \"%s\"\n", fnm );
		od_debug_message( buf );
	    }
	    return 0;
	}

	surveyname[0] = '\0';
	fgets( surveyname, PATH_LENGTH, fp );
	len = strlen( surveyname );
	if ( len == 0 ) return 0;
	if ( surveyname[len-1] == '\n' ) surveyname[len-1] = '\0';

	fclose( fp );
	surveynamedirty = 0;
    }

    if( od_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetSurveyName: %s\n", surveyname );
	od_debug_message( buf );
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

    if ( !dir ) dir = getCleanWinPath( getenv("DTECT_DATA") );
    if ( !dir ) dir = getCleanWinPath( getenv("dGB_DATA") );
    if ( !dir ) dir = getCleanWinPath( GetSettingsDataDir() );

    if ( dir && *dir && !getenv("DTECT_WINDATA") )
	setEnvVar( "DTECT_WINDATA" , dir );

#else

    if ( !dir ) dir = getenv( "DTECT_DATA" );
    if ( !dir ) dir = getenv( "dGB_DATA" );

    if ( !dir ) dir = GetSettingsDataDir();

#endif

    return dir;
}


const char* GetDataDir()
{
    static FileNameString filenamebuf;
    const char* survnm;
    const char* basedir = GetBaseDataDir();
    if ( !basedir || !*basedir ) return 0;

    survnm = GetSurveyName();
    if ( !survnm || !*survnm ) return basedir;

    strcpy( filenamebuf, mkFullPath(basedir,survnm) );

    if( od_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetDataDir: %s\n", filenamebuf );
	od_debug_message( buf );
    }

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


int setEnvVar( const char* env, const char* val )
{
    char* buf;
    if ( !env || !*env ) return NO;
    if ( !val ) val = "";

    buf = mMALLOC( strlen(env)+strlen(val) + 2, char );
    strcpy( buf, env );
    if ( *val ) strcat( buf, "=" );
    strcat( buf, val );

    putenv( buf );
    return YES;
}

int exitProgram( int ret )
{

#ifdef __win__

#define isBadHandle(h) ( (h) == NULL || (h) == INVALID_HANDLE_VALUE )

    // open process
    HANDLE hProcess = OpenProcess( PROCESS_TERMINATE, FALSE, getPID() );
    if ( isBadHandle( hProcess ) )
	printf( "OpenProcess() failed, err = %lu\n", GetLastError() );
    else
    {
	// kill process
	if ( ! TerminateProcess( hProcess, (DWORD) -1 ) )
	    printf( "TerminateProcess() failed, err = %lu\n", GetLastError() );

	// close handle
	CloseHandle( hProcess );
    }
#endif

    exit(ret);
    return ret;
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
