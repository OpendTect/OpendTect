/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

static const char* rcsID = "$Id: genc.c,v 1.76 2006-01-04 18:08:44 cvsdgb Exp $";

#include "oddirs.h"
#include "genc.h"
#include "math2.h"
#include "envvars.h"
#include "filegen.h"
#include "winutils.h"
#include "timefun.h"
#include "string2.h"

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
static char dbgbuf[256];

/*-> oddirs.h */

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
    static FileNameString result;
    char* chptr;

    /* Copy path to result buf */
    if ( path != result )
	strcpy( result, path && *path ? path : "." );
    if ( !filename || !*filename ) return result;

    /* Remove trailing dirseps from result */
    chptr = result;
    while ( *chptr ) chptr++;	/* chptr on '\0' */
    chptr--;			/* chptr on last char */
    while ( *chptr == *dirsep && chptr != result-1 )
	*chptr-- = '\0';
    chptr++;			/* chptr on (first) '\0' */

    /* Add dirsep */
    *chptr++ = *dirsep; *chptr = '\0';	/* chptr on '\0' again */

    /* Add filename */
    strcpy( chptr, filename );
    return result;
}


const char* GetSoftwareDir()
{
    char* chptr1; char* chptr2;
    const char* dir = 0;

    static char* cachedDir = 0;
    if ( cachedDir ) return cachedDir;

#ifndef __win__

    dir = GetEnvVar( "DTECT_APPL" );
    if ( !dir ) dir = GetEnvVar( "dGB_APPL" );

#else

    dir = GetEnvVar( "DTECT_WINAPPL" );
    if ( !dir ) dir = GetEnvVar( "dGB_WINAPPL" );

    if ( !dir ) dir = getCleanWinPath( GetEnvVar("DTECT_APPL") );
    if ( !dir ) dir = getCleanWinPath( GetEnvVar("dGB_APPL") );

#if 0
    if ( !dir )
    {
	TCHAR szPath[_MAX_PATH];
	if ( GetModuleFileName(NULL,szPath,_MAX_PATH) ) 
//.....
// TODO  : extract DTECT_APPL from full executable path.

    }
#endif

    if ( dir && *dir && !GetEnvVar("DTECT_WINAPPL") )
	SetEnvVar( "DTECT_WINAPPL" , dir );

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
	    SetEnvVar( "DTECT_APPL" , dir );
	}
    }
#endif

#ifdef __lux__
// TODO : use /proc/self/exe symlink to find full path to current running exe
#endif

    if ( !dir )
    {
	if ( !GetEnvVar("DTECT_ARGV0") ) return 0;

	static FileNameString progname;
	strcpy( progname, GetEnvVar("DTECT_ARGV0") );
	if ( !*progname ) return 0;

	char* chptr1 = progname;
	char* chptr2 = chptr1;
	while ( chptr2 = strstr( chptr1 + 1 , "bin" ) )
	    chptr1 = chptr2;

	if ( !chptr1 ) return 0;

	*chptr1-- = '\0';

	/* Remove trailing dirseps */
	while ( chptr1 != progname-1 && *chptr1 == *dirsep )
	    *chptr1-- = '\0';

	dir = progname;
    }

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	sprintf( dbgbuf, "GetSoftwareDir: '%s'", dir );
	od_debug_message( dbgbuf );
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

    if ( remote )
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
	sprintf( dbgbuf, "GetDataFileName for '%s': '%s'",
			fname ? fname : "(null)", filenamebuf );
	od_debug_message( dbgbuf );
    }

    return filenamebuf;
}


const char* GetIconFileName( const char* fname )
{
    static FileNameString filenamebuf;
    strcpy( filenamebuf, mkFullPath( GetSoftwareDir(), "data" ) );
    strcpy( filenamebuf, mkFullPath( filenamebuf, "icons" ) );
    if ( fname && *fname )
	strcpy( filenamebuf, mkFullPath( filenamebuf, fname ) );

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

    const char* nm = checkFile( GetEnvVar("OD_FILES"), "", fname );
    if ( !nm ) nm = checkFile( GetPersonalDir(), ".od", fname );
    if ( !nm ) nm = checkFile( GetSettingsDir(), "", fname );
    if ( !nm ) nm = checkFile( GetEnvVar("ALLUSERSPROFILE"), ".od", fname );
    if ( !nm ) nm = checkFile( GetSoftwareDir(), "data", fname );
    if ( !nm ) nm = checkFile( GetSoftwareDir(), "bin", fname );
    if ( !nm ) nm = checkFile( GetSoftwareDir(), "", fname );
    if ( !nm ) nm = checkFile( GetBaseDataDir(), "", fname );

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	sprintf( dbgbuf, "SearchODFile for '%s': '%s'",
			 fname ? fname : "(null)", nm ? nm : "<none>");
	od_debug_message( dbgbuf );
    }

    return nm;
}


const char* GetSoftwareUser()
{
    const char* ptr = 0;
#ifdef __win__

    ptr = GetEnvVar( "DTECT_WINUSER" );
    if ( !ptr ) ptr = GetEnvVar( "dGB_WINUSER" );

#endif

    if ( !ptr ) ptr = GetEnvVar( "DTECT_USER" );
    if ( !ptr ) ptr = GetEnvVar( "dGB_USER" );

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	sprintf( dbgbuf, "GetSoftwareUser: '%s'", ptr ? ptr : "<None>" );
	od_debug_message( dbgbuf );
    }

    return ptr;
}

const char* _GetHomeDir()
{
#ifndef __win__

    const char* ptr = GetEnvVar( "DTECT_HOME" );
    if ( !ptr ) ptr = GetEnvVar( "dGB_HOME" );
    if ( !ptr ) ptr = GetEnvVar( "HOME" );
    return ptr;

#else

    static FileNameString home = "";

    const char* ptr = GetEnvVar( "DTECT_WINHOME" );
    if ( !ptr ) ptr = GetEnvVar( "dGB_WINHOME" );

    if ( !ptr ) ptr = getCleanWinPath( GetEnvVar("DTECT_HOME") );
    				// should always at least be set
    if ( !ptr && od_debug_isOn(DBG_SETTINGS) )
    {
	sprintf( dbgbuf, "\nWarning: No DTECT_HOME nor DTECT_WINHOME set. \n"
			 " Falling back to USERPROFILE, APPDATA, etc.\n" );
	od_debug_message( dbgbuf );
    }
    
    if ( !ptr ) ptr = getCleanWinPath( GetEnvVar("dGB_HOME") );
    if ( !ptr ) ptr = getCleanWinPath( GetEnvVar("HOME") );

    if ( !ptr ) ptr = GetEnvVar( "USERPROFILE" ); // set by OS
    if ( !ptr ) ptr = GetEnvVar( "APPDATA" );     // set by OS -- but is hidden 

    if ( !ptr )
	ptr = GetEnvVar( "DTECT_USERPROFILE_DIR" ); // set by init script

    if ( !ptr && (!GetEnvVar("HOMEDRIVE") || !GetEnvVar("HOMEPATH")) ) 
    {
	if ( !ptr ) // Last resort. Is known to cause problems when used 
		    // during initialisation of statics. (0xc0000005)
	    ptr = GetSpecialFolderLocation( CSIDL_PROFILE ); // "User profile"
    }

    if ( ptr && *ptr )
    {
	strcpy( home, ptr );
	if ( !GetEnvVar("DTECT_WINHOME") )
	    SetEnvVar( "DTECT_WINHOME" , home );
	return home;
    }

    if ( !GetEnvVar("HOMEDRIVE") || !GetEnvVar("HOMEPATH") ) return 0;

    strcpy( home, GetEnvVar("HOMEDRIVE") );
    strcat( home, GetEnvVar("HOMEPATH") );

    if ( strcmp( home, "" ) && strcmp( home, "c:\\" ) && strcmp( home, "C:\\" ) 
	&& File_isDirectory( home ) ) // Apparantly, home has been set...
    {
	return home;
    }

    return 0;

#endif
}

const char* GetSettingsDir(void)
{ // NOTE: recompile SearchODFile in spec/General when making changes here...

    int mkfullpth = YES;
    
#ifndef __win__

    const char* ptr = GetEnvVar( "DTECT_SETTINGS" );

    if ( ptr )
	mkfullpth = NO;
    else
	ptr = _GetHomeDir();

#else    

    const char* ptr = GetEnvVar( "DTECT_WINSETTINGS" );
    if( !ptr) ptr = getCleanWinPath( GetEnvVar("DTECT_SETTINGS") );

    if ( ptr )
	mkfullpth = NO;
    else
	ptr = _GetHomeDir();

    if ( !ptr )
	return 0;

    char* chptr = (char*)ptr;
    while ( chptr && *chptr++ ) { if ( *chptr == '\r' ) *chptr='\0'; }

#endif

    if ( mkfullpth ) ptr = mkFullPath( ptr, ".od" );

    if ( !File_isDirectory(ptr) )
    {
	if ( File_exists(ptr) ) ptr = 0;
	else if ( !File_createDir(ptr,0) ) ptr = 0;
    }

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	sprintf( dbgbuf, "GetSettingsDir: '%s'", ptr ? ptr : "<none>" );
	od_debug_message( dbgbuf );
    }

    return ptr;
}


const char* GetPersonalDir(void)
{ // NOTE: recompile SearchODFile in spec/General when making changes here...

    const char* ptr = GetEnvVar( "OD_PERSONAL_DIR" );

    if ( !ptr )
	ptr = _GetHomeDir();

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	sprintf( dbgbuf, "GetPersonalDir: '%s'", ptr ? ptr : "<not found>" );
	od_debug_message( dbgbuf );
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

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	sprintf( dbgbuf, "GetSurveyFileName: '%s'", sfname );
	od_debug_message( dbgbuf );
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
	    if ( od_debug_isOn(DBG_SETTINGS) )
	    {
		sprintf( dbgbuf,
		    "GetSurveyName: GetSurveyFileName returned NULL\n" );
		od_debug_message( dbgbuf );
	    }
	    return 0;
	}

	fp = fopen( fnm, "r" );
	if ( !fp )
	{
	    if ( od_debug_isOn(DBG_SETTINGS) )
	    {
		sprintf( dbgbuf,
		    "GetSurveyName: Could not open SurveyFile: '%s'", fnm );
		od_debug_message( dbgbuf );
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

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	sprintf( dbgbuf, "GetSurveyName: %s", surveyname );
	od_debug_message( dbgbuf );
    }

    return surveyname;
}


extern const char* GetSettingsDataDir();

const char* GetBaseDataDir()
{
    const char* dir = 0;

#ifdef __win__

    dir = GetEnvVar( "DTECT_WINDATA" );
    if ( !dir ) dir = GetEnvVar( "dGB_WINDATA" );

    if ( !dir ) dir = getCleanWinPath( GetEnvVar("DTECT_DATA") );
    if ( !dir ) dir = getCleanWinPath( GetEnvVar("dGB_DATA") );
    if ( !dir ) dir = getCleanWinPath( GetSettingsDataDir() );

    if ( dir && *dir && !GetEnvVar("DTECT_WINDATA") )
	SetEnvVar( "DTECT_WINDATA" , dir );

#else

    if ( !dir ) dir = GetEnvVar( "DTECT_DATA" );
    if ( !dir ) dir = GetEnvVar( "dGB_DATA" );

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

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	sprintf( dbgbuf, "GetDataDir: '%s'", filenamebuf );
	od_debug_message( dbgbuf );
    }

    return filenamebuf;
}


/*-> genc.h */

const char* GetHostName()
{
    static char ret[256];
    gethostname( ret, 256 );
    return ret;
}


void SwapBytes( void* p, int n )
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


void PutIsLittleEndian( unsigned char* ptr )
{
    *ptr = __islittle__ ? 1 : 0;
}

#ifdef __msvc__
#define getpid	_getpid
#endif

int GetPID()
{
    return getpid();
}


int ExitProgram( int ret )
{
    if ( od_debug_isOn(DBG_PROGSTART) )
	printf( "\nExitProgram (PID: %d) at %s\n",
		GetPID(), Time_getFullDateString() );

#ifdef __win__

#define isBadHandle(h) ( (h) == NULL || (h) == INVALID_HANDLE_VALUE )

    // open process
    HANDLE hProcess = OpenProcess( PROCESS_TERMINATE, FALSE, GetPID() );
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


/*-> envvar.h */

char* GetOSEnvVar( const char* env )
{
    return getenv( env );
}


#define mMaxNrEnvEntries 1024
typedef struct _GetEnvVarEntry
{
    char	varname[128];
    char	value[1024];
} GetEnvVarEntry;


static void loadEntries( const char* fnm, int* pnrentries,
    			 GetEnvVarEntry* entries[] )
{
    static FILE* fp;
    static char linebuf[1024];
    static char* ptr;
    static const char* varptr;

    fp = fopen( fnm, "r" );
    if ( !fp ) return;

    while ( fgets(linebuf,1024,fp) )
    {
	ptr = linebuf;
	skipLeadingBlanks(ptr);
	varptr = ptr;
	if ( *varptr == '#' || !*varptr ) continue;

	while ( *ptr && !isspace(*ptr) ) ptr++;
	if ( !*ptr ) continue;
	*ptr++ = '\0';
	skipLeadingBlanks(ptr);
	removeTrailingBlanks(ptr);
	if ( !*ptr ) continue;

	entries[*pnrentries] = mMALLOC(1,GetEnvVarEntry);
	strcpy( entries[*pnrentries]->varname, varptr );
	strcpy( entries[*pnrentries]->value, ptr );
	(*pnrentries)++;
    }
    fclose( fp );
}


const char* GetEnvVar( const char* env )
{
    static int filesread = 0;
    static int nrentries = 0;
    static GetEnvVarEntry* entries[mMaxNrEnvEntries];
    int idx;

    if ( !env || !*env ) return 0;

    if ( !filesread )
    {
	filesread = 1;
	loadEntries( mkFullPath(GetSettingsDir(),"envvars"),
		     &nrentries, entries );
	loadEntries( GetDataFileName("EnvVars"), &nrentries, entries );
    }

    for ( idx=0; idx<nrentries; idx++ )
    {
	if ( !strcmp( entries[idx]->varname, env ) )
	    return entries[idx]->value;
    }

    return GetOSEnvVar( env );
}


int GetEnvVarYN( const char* env )
{
    const char* s = GetEnvVar( env );
    return !s || *s == '0' || *s == 'n' || *s == 'N' ? 0 : 1;
}


int GetEnvVarIVal( const char* env, int defltval )
{
    const char* s = GetEnvVar( env );
    return s ? atoi(s) : defltval;
}


double GetEnvVarDVal( const char* env, double defltval )
{
    const char* s = GetEnvVar( env );
    return s ? atof(s) : defltval;
}


int SetEnvVar( const char* env, const char* val )
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


/*-> math2.h */

#ifdef sun5
# include <ieeefp.h>
#endif

int IsNormalNumber( double x )
{
#ifdef __msvc__
    return _finite( x );
#else
    return finite( x );
#endif
}

double IntPowerOf( double x, int y )
{
    double ret = 1;
    if ( x == 0 ) return y ? 0 : 1;
    if ( x > 1.5 || x < -1.5 )
    {
	if ( y > 150 ) return mUndefValue;
	if ( y < -150 ) return 0;
	if ( x > 1.99 || x < -1.99 )
	{
	    if ( y > 100 ) return mUndefValue;
	    if ( y < -100 ) return 0;
	}
    }
    else if ( x < 0.5 && x > -0.5 )
    {
	if ( y > 100 ) return 0;
	if ( y < -100 ) return 1;
    }

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
