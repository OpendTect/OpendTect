/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

static const char* rcsID = "$Id: genc.c,v 1.44 2004-09-27 08:08:36 dgb Exp $";

#include "genc.h"
#include "filegen.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef __win__
# include <unistd.h>
# define sDirSep	"/"
#else
# include <process.h>
# include <float.h>
# include "windows.h"
# include "getspec.h"	// GetSpecialFolderLocation()
# define sDirSep	"\\"
# include "string2.h"

// registry stuff
# include <regstr.h>
# include <ctype.h>
# include <winreg.h>

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

#ifdef __win__

const char* getCygDir()
{
    static FileNameString answer;

    if ( strcmp(answer, "") ) return answer;
    
    HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
    LPCTSTR subkey="SOFTWARE\\Cygnus Solutions\\Cygwin\\mounts v2\\/";
    LPTSTR Value="native"; 

    BYTE Value_data[80];
    DWORD Value_size = 80;

    HKEY hKeyNew=0;
    DWORD retcode=0;
    DWORD Value_type=0;

    retcode = RegOpenKeyEx ( hKeyRoot, subkey, 0, KEY_QUERY_VALUE, &hKeyNew);

    if (retcode != ERROR_SUCCESS)
    {
	hKeyRoot = HKEY_CURRENT_USER;
	subkey="Software\\Cygnus Solutions\\Cygwin\\mounts v2/";

	retcode = RegOpenKeyEx( hKeyRoot, subkey, 0, KEY_QUERY_VALUE, &hKeyNew);
	if (retcode != ERROR_SUCCESS) return 0;
    }

    retcode = RegQueryValueEx( hKeyNew, Value, NULL, &Value_type, Value_data,
                               &Value_size);

    if (retcode != ERROR_SUCCESS) return 0;

    strcpy ( answer , (const char*) Value_data );
    return answer;
}


static const char* getTmpFile()
{
    static FileNameString buffer;

    if ( getenv("TMP") )
        strcpy( buffer, getenv( "TMP" ) );
    else if ( getenv("TEMP") )
        strcpy( buffer, getenv( "TEMP" ) );
    else if ( getenv("USERPROFILE") ) // should be set by OS
	strcpy( buffer, getenv( "USERPROFILE" ) );
    else // make sure we have at least write access...
    {
	const char* specf = GetSpecialFolderLocation( CSIDL_PERSONAL );
        if( specf && *specf ) strcpy( buffer, specf );
    }

    strcat( buffer, "\\od" );

    static int counter = 0;
    int time_stamp = time( (time_t*)0 ) + counter++;
    char uniquestr[80];
    sprintf( uniquestr, "%X%X", getPID(), (int)time_stamp );

    strcat( buffer, uniquestr );

    return buffer;
}



#define mRett(msg,v)\
 { \
    if( dgb_debug_isOn(DBG_SETTINGS) && msg ) \
    { \
	char buf[4096]; \
	sprintf(buf, "convertPath: converting '%s' : %s\n", \
		frompath, msg ); \
	dgb_debug_message( buf ); \
    } \
    mFREE(cmd); return v; \
 }

#define mRet(msg,v) { fclose( cygpth ); File_remove(tempfile,NO); mRett(msg,v) }

static const char* convertPath( const char* frompath, int towin )
{
    if ( !frompath || !*frompath ) return 0;
    removeTrailingBlanks( frompath );

    static FileNameString buffer;
    const char* cygpath = towin ? "cygpath -wa \"" : "cygpath -ua \"";

    static FileNameString tempfile;
    strcpy( tempfile, getTmpFile() );

    int len = strlen( cygpath ) + strlen( frompath ) + strlen(tempfile) + 64;

    char* cmd = mMALLOC(len,char);

    strcpy( cmd, cygpath );
    strcat( cmd, frompath );
    strcat( cmd, "\" > \"" );
    strcat( cmd, tempfile );
    strcat( cmd, "\"" );

    int ret = system( cmd );
    if ( ret )
    {   // try to do our best. Does not follow links, however
	static const char* drvstr="/cygdrive/";

	char* ptr = (char*) frompath;
	skipLeadingBlanks( ptr ); removeTrailingBlanks( ptr );
	if( towin )
	{
	    if ( *(ptr+1) == ':' ) // already in windows style.
		{ strcpy( buffer, ptr ); mRett(0,buffer) }

	    char* cygdrv = strstr( ptr, drvstr );
	    if( cygdrv )
	    {
		char* drv = cygdrv + strlen( drvstr );
		*buffer = *drv; *(buffer+1) = ':'; *(buffer+2) = '\0';
		strcat( buffer, ++drv ); 

		replaceCharacter( buffer, '/', '\\' );
	    }
	    else
	    {
		strcpy( buffer, getCygDir() );
		strcat( buffer, ptr );
		replaceCharacter( buffer, '/', '\\' );

		if ( ! File_exists(buffer) )
		{
		    fprintf( stderr, 
		"\nWarning: path conversion from Unix style to Windows style:");
		    fprintf( stderr,
		"\n         Unix path     '%s'", frompath );
		    fprintf( stderr,
		"\n         converted to  '%s' does not exist.\n\n", buffer );
		    fflush( stderr );
		}
	    }


	    ret = 0;
	}
	else
	{

	    if ( *(ptr+1) != ':' ) // already in unix style.
		{ strcpy( buffer, ptr ); mRett(0,buffer) }

	    *(ptr+1) = '\0';

	    strcpy( buffer, drvstr );
	    strcat( buffer, ptr );
	    strcat( buffer, ptr+2 );

	    replaceCharacter( buffer, '\\' , '/' );

	    ret = 0;
	}
    }
    else
    {   // read result from cygpath utility

	FILE* cygpth;

	if( (cygpth = fopen( tempfile, "rt" )) == NULL )
	    mRet("could not open temporary file", 0)

	if ( feof( cygpth ) )
	    mRet("input past end on tempfile",0)

	if ( fgets( buffer, PATH_LENGTH, cygpth ) == NULL )
	    mRet("could nog read line from tempfile",0)

	fclose( cygpth );
	File_remove(tempfile,NO);
    }

    mFREE( cmd );

    char* eol = strstr( buffer , "\n" );
    if ( eol ) *eol = '\0';

    if( dgb_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "convertPath: from '%s' to '%s' \n", frompath, buffer );
	dgb_debug_message( buf );
    }

    return ret ? 0 : buffer;
}


const char* getWinPath( const char* path )
{
    return convertPath( path, YES );
}

const char* getUnixPath( const char* path )
{
    return convertPath( path, NO );
}


#endif


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

    if ( !dir ) dir = getWinPath( getenv("DTECT_APPL") );
    if ( !dir ) dir = getWinPath( getenv("dGB_APPL") );

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

    if( dgb_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetSoftwareDir: %s\n", dir );
	dgb_debug_message( buf );
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

    if ( dgb_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetDataFileName for %s: %s\n", fname ? fname : "(null)",
			filenamebuf );
	dgb_debug_message( buf );
    }

    return filenamebuf;
}

static const char* checkFile( const char* path, const char* subdir,
			      const char* fname )
{
    static FileNameString filenamebuf;
    strcpy( filenamebuf, mkFullPath( path, subdir ) );
    if ( fname && *fname )
	strcpy( filenamebuf, mkFullPath( filenamebuf, fname ) );

    if ( File_exists(filenamebuf) )
	return filenamebuf;
 
    return 0;
}

const char* SearchConfigFile( const char* fname )
{

    const char* nm = checkFile( GetPersonalDir(), ".od", fname );
    if( !nm ) nm = checkFile( GetSettingsDir(), ".od", fname );
    if( !nm ) nm = checkFile( GetSoftwareDir(), "data", fname );

    if ( dgb_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "SearchConfigFile for %s: %s\n", fname ? fname : "(null)",
			nm );
	dgb_debug_message( buf );
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

    if( dgb_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetSoftwareUser: %s\n", ptr ? ptr : "Not set" );
	dgb_debug_message( buf );
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

    if ( !ptr ) ptr = getWinPath( getenv("DTECT_HOME") );
    if ( !ptr ) ptr = getWinPath( getenv("dGB_HOME") );
    if ( !ptr ) ptr = getWinPath( getenv("HOME") );

    if ( ptr && *ptr )
    {
	strcpy( home, ptr );
	if ( !getenv("DTECT_WINHOME") )
	    setEnvVar( "DTECT_WINHOME" , home );
	return home;
    }

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
{
    const char* ptr = _GetHomeDir();

#ifdef __win__
    if ( !ptr )
	ptr = getenv( "APPDATA" ); // should be set by OS

    if ( !ptr )
	ptr = getenv( "DTECT_APPLICATION_DATA" ); // set by init script

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

    if( dgb_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetSettingsDir: %s\n", ptr );
	dgb_debug_message( buf );
    }

    return ptr;
}


const char* GetPersonalDir(void)
{
    const char* ptr = _GetHomeDir();

#ifdef __win__

    if ( !ptr ) 
	ptr = getenv( "USERPROFILE" ); // should be set by OS

    if ( !ptr ) 
	ptr = getenv( "DTECT_USERPROFILE_DIR" );

    if ( !ptr ) 
	ptr = getenv( "DTECT_MYDOCUMENTS_DIR" );

    if ( !ptr ) // Last resort. Is known to cause problems when used 
                // during initialisation of statics. (0xc0000005)
	ptr = GetSpecialFolderLocation( CSIDL_PROFILE ); // "User Profile"

    char* chptr = (char*)ptr;
    while ( chptr && *chptr++ ) { if ( *chptr == '\r' ) *chptr='\0'; }

#endif

    if( dgb_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetPersonalDir: %s\n", ptr );
	dgb_debug_message( buf );
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

    if( dgb_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetSurveyFileName: %s\n", sfname );
	dgb_debug_message( buf );
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
	    if( dgb_debug_isOn(DBG_SETTINGS) )
	    {
		char buf[255];
		sprintf(buf,
		    "GetSurveyName: GetSurveyFileName returned NULL\n" );
		dgb_debug_message( buf );
	    }
	    return 0;
	}

	fp = fopen( fnm, "r" );
	if ( !fp )
	{
	    if( dgb_debug_isOn(DBG_SETTINGS) )
	    {
		char buf[255];
		sprintf(buf,
		    "GetSurveyName: Could not open SurveyFile: \"%s\"\n", fnm );
		dgb_debug_message( buf );
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

    if( dgb_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetSurveyName: %s\n", surveyname );
	dgb_debug_message( buf );
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

    if ( !dir ) dir = getWinPath( getenv("DTECT_DATA") );
    if ( !dir ) dir = getWinPath( getenv("dGB_DATA") );
    if ( !dir ) dir = getWinPath( GetSettingsDataDir() );

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

    if( dgb_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetDataDir: %s\n", filenamebuf );
	dgb_debug_message( buf );
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
