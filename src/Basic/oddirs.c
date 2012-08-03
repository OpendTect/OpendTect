/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

static const char* rcsID mUnusedVar = "$Id: oddirs.c,v 1.43 2012-08-03 13:01:34 cvskris Exp $";

#include "genc.h"
#include "oddirs.h"
#include "string2_c.h"
#include "envvars.h"
#include "filegen.h"
#include "winutils.h"
#include "debugmasks.h"

#include <string.h>
#include <stdio.h>
#ifdef __msvc__
# include <direct.h>
#endif
#ifndef __win__
# define sDirSep	"/"
static const char* lostinspace = "/tmp";
#else
# define sDirSep	"\\"
static const char* lostinspace = "C:\\";

#endif

#ifdef __mac__
# include <CoreServices/CoreServices.h>
#endif

static FileNameString surveyname;
static int surveynamedirty = mC_True;
static const char* dirsep = sDirSep;
static char dbgstrbuf[mMaxFilePathLength+1];


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


/* -> hidden survey functions used in survinfo.cc, ioman.cc etc. */

mGlobal( Basic ) int SurveyNameDirty(void);
int SurveyNameDirty(void)
{
    return surveynamedirty;
}


mGlobal( Basic ) void SetSurveyNameDirty(void);
void SetSurveyNameDirty(void)
{
    surveynamedirty = 1;
}


mGlobal( Basic ) const char* GetSurveyFileName(void);
const char* GetSurveyFileName(void)
{
    static FileNameString sfname;
    static int inited = mC_False;
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
	inited = mC_True;
    }

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	sprintf( dbgstrbuf, "GetSurveyFileName: '%s'", sfname );
	od_debug_message( dbgstrbuf );
    }

    return sfname;
}


mGlobal( Basic ) void SetSurveyName(const char*);
void SetSurveyName( const char* newnm )
{
    mSkipBlanks( newnm );
    strcpy( surveyname, newnm );
    C_removeTrailingBlanks( surveyname );
    surveynamedirty = 0;
}


mGlobal( Basic ) const char* GetSurveyName(void);
const char* GetSurveyName(void)
{
    FILE* fp; char* ptr;
    static char tmpbuf[mMaxFilePathLength];
    if ( !surveynamedirty ) return surveyname;

    fp = fopen( GetSurveyFileName(), "r" );
    if ( !fp ) return 0;

    ptr = tmpbuf; *ptr = '\0';
    fgets( ptr, mMaxFilePathLength, fp );
    fclose( fp );

    mTrimBlanks( ptr );
    if ( !*ptr ) return 0;

    strcpy( surveyname, ptr );
    surveynamedirty = 0;

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	sprintf( dbgstrbuf, "GetSurveyName: %s", surveyname );
	od_debug_message( dbgstrbuf );
    }

    return surveyname;
}


/*-> implementing oddirs.h */

	/* 'survey data' scope */

extern const char* GetSettingsDataDir(void);

const char* GetBaseDataDir(void)
{
    static FileNameString bddir;
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

    if ( !dir ) return 0;
    strcpy( bddir, dir );
    return bddir;
}


const char* GetDataDir(void)
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
	sprintf( dbgstrbuf, "GetDataDir: '%s'", filenamebuf );
	od_debug_message( dbgstrbuf );
    }

    return filenamebuf;
}


const char* GetProcFileName( const char* fname )
{
    static FileNameString filenamebuf;
    strcpy( filenamebuf, mkFullPath( GetDataDir(), "Proc" ) );
    if ( fname && *fname )
	strcpy( filenamebuf, mkFullPath( filenamebuf, fname ) );
    return filenamebuf;
}


const char* GetScriptsDir( const char* subdir )
{
    static FileNameString filenamebuf;
    const char* ret = GetEnvVar( "DTECT_SCRIPTS_DIR" );
    if ( ret && *ret )
	strcpy( filenamebuf, ret );
    else
    {
	strcpy( filenamebuf, GetProcFileName(0) );
	if ( subdir && *subdir )
	    strcpy( filenamebuf, mkFullPath(filenamebuf,subdir) );
    }

    return filenamebuf;
}


	/* 'sytem' scope */

#define mRetNope() { *dirnm = '\0'; return 0; }

static int gtSoftwareDirFromArgv( char* dirnm )
{
    char* chptr1; char* chptr2;

    if ( !GetEnvVar("DTECT_ARGV0") ) mRetNope()

    strcpy( dirnm, GetEnvVar("DTECT_ARGV0") );
    if ( !*dirnm ) mRetNope()

    chptr2 = chptr1 = dirnm;
    while ( (chptr2 = strstr( chptr1 + 1, "bin" )) )
	chptr1 = chptr2;

    if ( !chptr1 ) mRetNope()

    *chptr1-- = '\0';
    /* Remove trailing dirseps */
    while ( chptr1 != dirnm-1 && *chptr1 == *dirsep )
	*chptr1-- = '\0';

    return *dirnm ? 1 : 0;
}

#ifdef __mac__
static int getBundleLocation( char* dirnm )
{
    *dirnm = 0;
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef url = CFBundleCopyBundleURL(mainBundle);
    CFStringRef cfStr = CFURLCopyPath(url);

    const char* bundlepath =
	CFStringGetCStringPtr(cfStr, CFStringGetSystemEncoding());
    if ( !bundlepath || !*bundlepath ) return 0;

    strcpy( dirnm, bundlepath );
    return 1;
}
#endif


const char* GetSoftwareDir( int acceptnone )
{
#ifdef __msvc__
    char* termchar;
#endif
    static char dirnm[1024];
    const char* dir = 0;
    static const char* ret = 0;
    if ( ret ) return ret;
    ret = dirnm;

#ifdef __cygwin__
    dir = GetEnvVar( "DTECT_WINAPPL" );
    if ( !dir ) dir = getCleanWinPath( GetEnvVar("DTECT_APPL") );
#else
    dir = GetEnvVar( "DTECT_APPL" );
#endif

#ifdef __msvc__
    if ( !dir || !*dir )
    {
	GetShortPathName(_getcwd(NULL,0),dirnm,sizeof(dirnm));
	termchar = strstr( dirnm, "\\bin\\win" ); // remove \bin\win%%
	if ( termchar )
	    *termchar = '\0';
	
	dir = dirnm;
    }
#endif

    if ( !dir || !*dir )
    {
#ifdef __mac__
       	if ( getBundleLocation(dirnm) )
	    dir = dirnm;
#endif
       	if ( !dir && gtSoftwareDirFromArgv(dirnm) )
	    dir = dirnm;
	if ( !dir || !*dir )
	{
	    if ( acceptnone )
		return 0;

	    fprintf( stderr, "Cannot determine OpendTect location\n" );
	    ExitProgram( 1 );
	}
    }

#ifdef __win__
    SetEnvVar( "DTECT_WINAPPL" , dir );
#else
    SetEnvVar( "DTECT_APPL" , dir );
#endif

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	sprintf( dbgstrbuf, "GetSoftwareDir: '%s'", dir );
	od_debug_message( dbgstrbuf );
    }

    if ( dir != dirnm )
	strcpy( dirnm, dir );

    return ret;
}


const char* GetApplSetupDir(void)
{
    static char* ret = 0;
    static FileNameString filenamebuf;
    const char* envstr = 0;
    if ( ret )
	return *ret ? ret : 0;

    filenamebuf[0] = '\0';
    ret = filenamebuf;

#ifdef __win__
    envstr = GetEnvVar( "DTECT_WINAPPL_SETUP" );
#endif
    if ( !envstr || !*envstr )
	envstr = GetEnvVar( "DTECT_APPL_SETUP" );
    if ( !envstr || !*envstr )
	return 0;

    strcpy( ret, envstr );
    return ret;
}


const char* GetSetupDataFileDir( ODSetupLocType lt, int acceptnone )
{
    static FileNameString dirnm;
    const char* appldir;
    if ( lt > ODSetupLoc_ApplSetupPref )
	strcpy( dirnm, mkFullPath(GetSoftwareDir(acceptnone),"data") );
    else
    {
	appldir = GetApplSetupDir();
	if ( !appldir ) return 0;
	strcpy( dirnm, mkFullPath(appldir,"data") );
    }
    return dirnm;
}


const char* GetSetupDataFileName( ODSetupLocType lt, const char* fnm,
				  int acceptnone )
{
    static FileNameString filenm;
    const char* appldir;

    if ( lt == ODSetupLoc_SWDirOnly )
    {
	strcpy( filenm, mkFullPath( GetSetupDataFileDir(lt, acceptnone), fnm ) );
	return filenm;
    }

    appldir = GetSetupDataFileDir(ODSetupLoc_ApplSetupOnly, acceptnone);
    if ( !appldir )
	return lt == ODSetupLoc_ApplSetupOnly ? 0
	     : GetSetupDataFileName(ODSetupLoc_SWDirOnly,fnm, acceptnone);

    strcpy( filenm, mkFullPath( GetSetupDataFileDir(lt, acceptnone), fnm ) );

    if ( (lt == ODSetupLoc_ApplSetupPref || lt == ODSetupLoc_SWDirPref)
	&& !File_exists(filenm) )
    {
	/* try 'other' file */
	GetSetupDataFileName( lt == ODSetupLoc_ApplSetupPref
		? ODSetupLoc_SWDirOnly : ODSetupLoc_ApplSetupOnly, fnm,
				acceptnone );
	if ( File_exists(filenm) )
	    return filenm;

	/* 'other' file also doesn't exist: revert */
	GetSetupDataFileName( lt == ODSetupLoc_ApplSetupPref
		? ODSetupLoc_ApplSetupOnly : ODSetupLoc_SWDirOnly, fnm,
				acceptnone );
    }

    return filenm;
}


const char* GetDocFileDir( const char* filedir )
{
    static FileNameString dirnm;
    strcpy( dirnm, mkFullPath(GetSoftwareDir(0),"doc") );
    strcpy( dirnm, mkFullPath(dirnm,filedir) );
    return dirnm;
}


const char* GetPlfSubDir(void)
{
    return __plfsubdir__;
}


const char* GetBinPlfDir(void)
{
    static FileNameString dirnm;
    strcpy( dirnm, mkFullPath(GetSoftwareDir(0),"bin") );
    strcpy( dirnm, mkFullPath(dirnm,GetPlfSubDir()) );
    return dirnm;
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

#ifdef __msvc__
    return "";
#else

    static FileNameString progname;
    const char* fnm = 0;
    const char* basedir = GetApplSetupDir();
    if ( basedir )
	fnm = gtExecScript( basedir, remote );

    if ( !fnm || !File_exists(fnm) )
	fnm = gtExecScript( GetSoftwareDir(0), remote );

    strcpy( progname, "'" ); strcat( progname, fnm ); strcat( progname, "' " );
    return progname;
#endif
}


const char* GetSoftwareUser(void)
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
	    sprintf( dbgstrbuf, "GetSoftwareUser: '%s'", ptr ? ptr : "<None>" );
	    od_debug_message( dbgstrbuf );
	}
	ret = ptr ? ptr : "";
    }

    return *ret ? ret : 0;
}


const char* GetUserNm(void)
{
#ifdef __win__
    static char usernm[256];
    int len = 256;
    GetUserName( usernm, &len );
    return usernm;
#else
    const char* ret = GetEnvVar( "USER" );
    return ret;
#endif
}


static void getHomeDir( char* val )
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
	if ( *val && !C_caseInsensitiveEqual(val,"c:\\",0)
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
    C_replaceCharacter( val, '\r', '\0' );
#endif
}

#define mDoRet( ret ) return #ret

const char* GetBinSubDir()
{
    mDoRet( __binsubdir__ );
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
	    sprintf( dbgstrbuf, "GetPersonalDir: '%s'", dirnm );
	    od_debug_message( dbgstrbuf );
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
		File_remove( dirnm, mFile_NotRecursive );
	    if ( !File_createDir(dirnm,0) )
	    {
		fprintf( stderr, "Fatal: Cannot create '.od' directory in home "
				    "directory:\n%s\n", dirnm );
		ExitProgram( 1 );
	    }
	    if ( od_debug_isOn(DBG_SETTINGS) )
	    {
		sprintf( dbgstrbuf, "Had to create SettingsDir: '%s'", dirnm );
		od_debug_message( dbgstrbuf );
	    }
	}

	if ( od_debug_isOn(DBG_SETTINGS) )
	{
	    sprintf( dbgstrbuf, "GetSettingsDir: '%s'", dirnm );
	    od_debug_message( dbgstrbuf );
	}

	ret = dirnm;
    }

    return ret;
}


const char* GetSettingsFileName( const char* fnm )
{
    return mkFullPath( GetSettingsDir(), fnm );
}
