/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

static const char* rcsID = "$Id: genc.c,v 1.85 2007-06-14 11:22:37 cvsbert Exp $";

#include "oddirs.h"
#include "genc.h"
#include "math2.h"
#include "envvars.h"
#include "filegen.h"
#include "winutils.h"
#include "timefun.h"
#include "string2.h"
#include "mallocdefs.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef __win__
# include <unistd.h>
# define sDirSep	"/"
static const char* lostinspace = "/tmp";
#else
# include <float.h>
# define sDirSep	"\\"
static const char* lostinspace = "C:\\";
#endif

#ifdef __mac__
# include <CoreServices/CoreServices.h>
#endif

#include "debugmasks.h"
static FileNameString surveyname;
static int surveynamedirty = YES;
static const char* dirsep = sDirSep;
static char tmpbuf[PATH_LENGTH+1];


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

#else

    dir = GetEnvVar( "DTECT_WINAPPL" );
    if ( !dir ) dir = getCleanWinPath( GetEnvVar("DTECT_APPL") );

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
	sprintf( tmpbuf, "GetSoftwareDir: '%s'", dir );
	od_debug_message( tmpbuf );
    }

    cachedDir = mMALLOC( strlen(dir)+ 1, char );
    strcpy( cachedDir, dir );

    return cachedDir;
}


const char* GetPlfSubDir()
{
#ifdef __win__
    return "win";
#else
    const char* ret = GetEnvVar( "PLFSUBDIR" );
    if ( !ret || !*ret ) ret = GetEnvVar( "binsubdir" );
    return ret && *ret ? ret : GetEnvVar( "HDIR" );
#endif
}


const char* GetSiteDataDir()
{
    static char* ret = 0;
    static FileNameString filenamebuf;
    const char* envstr;
    if ( ret )
	return *ret ? ret : 0;

    filenamebuf[0] = '\0';
    ret = filenamebuf;

    envstr = GetEnvVar( "DTECT_SITE_DATA" );
    if ( !envstr || !*envstr )
	return 0;

    strcpy( ret, envstr );
    return ret;
}



static const char* gtExecScript( const char* basedir, int remote )
{
    static FileNameString scriptnm;
    strcpy( scriptnm, mkFullPath(basedir,"bin") );
    strcpy( scriptnm, mkFullPath(scriptnm,"od_exec") );
    if ( remote ) strcat( scriptnm, "_rmt" );
    return scriptnm;
}


const char* GetExecScript( int remote )
{
    static FileNameString progname;
    const char* fnm = 0;
    const char* basedir = GetSiteDataDir();
    if ( basedir )
	fnm = gtExecScript( basedir, remote );

    if ( !fnm || !File_exists(fnm) )
	fnm = gtExecScript( GetSoftwareDir(), remote );

    strcpy( progname, "'" );
    strcat( progname, fnm );
    strcat( progname, "' " );
    return progname;
}


const char* GetDataFileDir()
{
    static FileNameString dirnmbuf;
    strcpy( dirnmbuf, mkFullPath( GetSoftwareDir(), "data" ) );
    return dirnmbuf;
}


static const char* gtDataFileName( const char* basedir, const char* fname )
{
    static FileNameString filenamebuf;
    strcpy( filenamebuf, mkFullPath( basedir, "data" ) );
    if ( fname && *fname )
	strcpy( filenamebuf, mkFullPath( filenamebuf, fname ) );
    return filenamebuf;
}


const char* GetDataFileName( const char* fname )
{
    const char* ret; const char* basedir;
    if ( !fname || !*fname )
	return GetDataFileDir();

    basedir = GetSiteDataDir();
    if ( basedir )
    {
	ret = gtDataFileName( basedir, fname );
	if ( File_exists(ret) )
	    return ret;
    }

    return gtDataFileName( GetSoftwareDir(), fname );
}


const char* GetProcFileName( const char* fname )
{
    static FileNameString filenamebuf;
    strcpy( filenamebuf, mkFullPath( GetDataDir(), "Proc" ) );
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

    const char* nm = checkFile( GetPersonalDir(), ".od", fname );
    if ( !nm ) nm = checkFile( GetSettingsDir(), "", fname );
    if ( !nm ) nm = checkFile( GetBaseDataDir(), "", fname );
    if ( !nm ) nm = checkFile( GetSiteDataDir(), "data", fname );
    if ( !nm ) nm = checkFile( GetSoftwareDir(), "data", fname );
    if ( !nm ) nm = checkFile( GetSiteDataDir(), "bin", fname );
    if ( !nm ) nm = checkFile( GetSoftwareDir(), "bin", fname );
    if ( !nm ) nm = checkFile( GetSiteDataDir(), "", fname );
    if ( !nm ) nm = checkFile( GetSoftwareDir(), "", fname );

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	sprintf( tmpbuf, "SearchODFile for '%s': '%s'",
			 fname ? fname : "(null)", nm ? nm : "<none>");
	od_debug_message( tmpbuf );
    }

    return nm;
}


const char* GetSoftwareUser()
{
    const char* ptr = 0;
    static const char* ret = 0;
    if ( !ret )
    {

#ifdef __win__

	ptr = GetEnvVar( "DTECT_WINUSER" );

#endif

	if ( !ptr ) ptr = GetEnvVar( "DTECT_USER" );

	if ( od_debug_isOn(DBG_SETTINGS) )
	{
	    sprintf( tmpbuf, "GetSoftwareUser: '%s'", ptr ? ptr : "<None>" );
	    od_debug_message( tmpbuf );
	}

	ret = ptr ? ptr : "";
    }

    return *ret ? ret : 0;
}

static const char* getHomeDir( char* val )
{
    const char* dir;

#ifndef __win__

    dir = GetEnvVar( "DTECT_HOME" );
    if ( !dir ) dir = GetEnvVar( "HOME" );

#else

    dir = GetEnvVar( "DTECT_WINHOME" );
    if ( !dir ) dir = getCleanWinPath( GetEnvVar("DTECT_HOME") );
    				// should always at least be set
    if ( !dir ) dir = getCleanWinPath( GetEnvVar("HOME") );

    if ( !dir && GetEnvVar("HOMEDRIVE") && GetEnvVar("HOMEPATH") )
    {
	/* This may be not be trustworthy */
	strcpy( val, GetEnvVar("HOMEDRIVE") );
	strcat( val, GetEnvVar("HOMEPATH") );
	if ( *val && !caseInsensitiveEqual(val,"c:\\",0)
	  && File_isDirectory(val) )
	    dir = val;
    }

    if ( !dir && od_debug_isOn(DBG_SETTINGS) )
	od_debug_message( "Problem: No DTECT_WINHOME, DTECT_HOME "
			  "or HOMEDRIVE/HOMEPATH. Result may be bad." );
    
    if ( !dir ) dir = getCleanWinPath( GetEnvVar("HOME") );
    if ( !dir ) dir = GetEnvVar( "USERPROFILE" ); // set by OS
    if ( !dir ) dir = GetEnvVar( "APPDATA" );     // set by OS -- but is hidden 
    if ( !dir ) dir = GetEnvVar( "DTECT_USERPROFILE_DIR" );// set by init script
    if ( !dir ) // Last resort. Is known to cause problems when used 
		// during initialisation of statics. (0xc0000005)
	dir = GetSpecialFolderLocation( CSIDL_PROFILE ); // "User profile"

#endif

    if ( !dir )
    {
	dir = lostinspace;
	od_debug_message( "Serious problem: Cannot find any good value for "
			  "'Personal directory'. Using:\n" );
	od_debug_message( dir );
    }

    if ( dir != val )
	strcpy( val, dir );

#ifdef __win__
    if ( !GetEnvVar("DTECT_WINHOME") )
	SetEnvVar( "DTECT_WINHOME", val );
    replaceCharacter( val, '\r', '\0' );
#endif
}


const char* GetPersonalDir( void )
{
    static FileNameString dirnm;
    static const char* ret = 0;
    const char* ptr;

    if ( !ret )
    {
	ptr = GetEnvVar( "DTECT_PERSONAL_DIR" );
	if ( ptr )
	    strcpy( dirnm, ptr );
	else
	    getHomeDir( dirnm );

	if ( od_debug_isOn(DBG_SETTINGS) )
	{
	    sprintf( tmpbuf, "GetPersonalDir: '%s'", dirnm );
	    od_debug_message( tmpbuf );
	}

	ret = dirnm;
    }

    return ret;
}


const char* GetSettingsDir(void)
{
    static FileNameString dirnm;
    static const char* ret = 0;
    const char* ptr = 0;

    if ( !ret )
    {
#ifdef __win__
	ptr = GetEnvVar( "DTECT_WINSETTINGS" );
	if( !ptr ) ptr = getCleanWinPath( GetEnvVar("DTECT_SETTINGS") );
#else
	ptr = GetEnvVar( "DTECT_SETTINGS" );
#endif

	if ( ptr )
	    strcpy( dirnm, ptr );
	else
	{
	    getHomeDir( dirnm );
	    strcpy( dirnm, mkFullPath(dirnm,".od") );
	}

	if ( !File_isDirectory(dirnm) )
	{
	    if ( File_exists(dirnm) )
		File_remove( dirnm, NO );
	    if ( !File_createDir(dirnm,0) )
	    {
		fprintf( stderr, "Fatal: Cannot create '.od' directory in home "
				    "directory:\n%s\n", dirnm );
		ExitProgram( 1 );
	    }
	    if ( od_debug_isOn(DBG_SETTINGS) )
	    {
		sprintf( tmpbuf, "Had to create SettingsDir: '%s'", dirnm );
		od_debug_message( tmpbuf );
	    }
	}

	if ( od_debug_isOn(DBG_SETTINGS) )
	{
	    sprintf( tmpbuf, "GetSettingsDir: '%s'", dirnm );
	    od_debug_message( tmpbuf );
	}

	ret = dirnm;
    }

    return ret;
}


const char* GetSurveyFileName()
{
    static FileNameString sfname;
    static int inited = NO;
    const char* ptr;

    if ( !inited )
    {
	ptr = GetSettingsDir();
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
	sprintf( tmpbuf, "GetSurveyFileName: '%s'", sfname );
	od_debug_message( tmpbuf );
    }

    return sfname;
}


void SetSurveyName( const char* newnm )
{
    skipLeadingBlanks( newnm );
    strcpy( surveyname, newnm );
    removeTrailingBlanks( surveyname );
    surveynamedirty = 0;
}


const char* GetSurveyName()
{
    FILE* fp; char* ptr;
    if ( !surveynamedirty ) return surveyname;

    fp = fopen( GetSurveyFileName(), "r" );
    if ( !fp ) return 0;

    ptr = tmpbuf; *ptr = '\0';		/* Don't use tmpbuf between here ... */
    fgets( ptr, PATH_LENGTH, fp );
    fclose( fp );

    skipLeadingBlanks( ptr );
    removeTrailingBlanks( ptr );
    if ( !*ptr ) return 0;

    strcpy( surveyname, ptr );		/* ... and here */
    surveynamedirty = 0;

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	sprintf( tmpbuf, "GetSurveyName: %s", surveyname );
	od_debug_message( tmpbuf );
    }

    return surveyname;
}


extern const char* GetSettingsDataDir();

const char* GetBaseDataDir()
{
    const char* dir = 0;

#ifdef __win__

    dir = GetEnvVar( "DTECT_WINDATA" );

    if ( !dir ) dir = getCleanWinPath( GetEnvVar("DTECT_DATA") );
    if ( !dir ) dir = getCleanWinPath( GetSettingsDataDir() );

    if ( dir && *dir && !GetEnvVar("DTECT_WINDATA") )
	SetEnvVar( "DTECT_WINDATA" , dir );

#else

    if ( !dir ) dir = GetEnvVar( "DTECT_DATA" );
    if ( !dir ) dir = GetSettingsDataDir();

#endif

    return dir;
}


const char* GetDataDir()
{
    static FileNameString filenamebuf;
    const char* survnm;
    const char* basedir = GetBaseDataDir();
    if ( !basedir || !*basedir ) return lostinspace;

    survnm = GetSurveyName();
    if ( !survnm || !*survnm ) survnm = "_no_current_survey_";
    strcpy( filenamebuf, mkFullPath(basedir,survnm) );

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	sprintf( tmpbuf, "GetDataDir: '%s'", filenamebuf );
	od_debug_message( tmpbuf );
    }

    return filenamebuf;
}


/*-> genc.h */

const char* GetLocalHostName()
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
#ifdef __little__
    *ptr = 1;
#else
    *ptr = 0;
#endif
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

// On Mac OpendTect crashes when calling the usual exit and shows error message:
// dyld: odmain bad address of lazy symbol pointer passed to stub_binding_helper
// _Exit does not call registered exit functions and prevents crash
#ifdef __mac__
    _Exit(0);
    return 0;
#endif

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
    if ( mcIsUndefined(x) ) return mcUndefValue;

    double ret = 1;
    if ( x == 0 )
	return y ? 0 : 1;

    if ( x > 1.5 || x < -1.5 )
    {
	if ( y > 150 ) return mcUndefValue;
	if ( y < -150 ) return 0;
	if ( x > 1.99 || x < -1.99 )
	{
	    if ( y > 100 ) return mcUndefValue;
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


double ACos( double c )
{
    if ( c>=1 ) return 0;
    if ( c<=-1 ) return M_PI;
    return acos( c );
}


double ASin( double s )
{
    if ( s>=1 ) return M_PI_2;
    if ( s<=-1 ) return -M_PI_2;
    return asin( s );
}
