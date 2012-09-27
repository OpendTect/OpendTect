/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "genc.h"
#include "oddirs.h"
#include <string.h>
#include "envvars.h"
#include "winutils.h"
#include "debugmasks.h"
#include "file.h"
#include "filepath.h"
#include "settings.h"

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
	strcpy( result.buf(), path && *path ? path : "." );
    if ( !filename || !*filename ) return result;

    /* Remove trailing dirseps from result */
    chptr = result.buf();
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

mExternC(Basic) int SurveyNameDirty(void)
{
    return surveynamedirty;
}


mExternC(Basic) void SetSurveyNameDirty(void)
{
    surveynamedirty = 1;
}



mExternC(Basic) const char* GetSurveyFileName(void)
{
    static BufferString sfname;

    if ( sfname.isEmpty() )
    {
	const char* ptr = GetSettingsDir();
	sfname = mkFullPath(ptr,"survey");
	ptr = GetSoftwareUser();
	if ( ptr )
	{
	    sfname += ".";
	    sfname += ptr;
	}
    }

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	sprintf( dbgstrbuf, "GetSurveyFileName: '%s'", sfname.buf() );
	od_debug_message( dbgstrbuf );
    }

    return sfname;
}


mExternC(Basic) void SetSurveyName( const char* newnm )
{
    mSkipBlanks( newnm );
    strcpy( surveyname.buf(), newnm );
    removeTrailingBlanks( surveyname.buf() );
    surveynamedirty = 0;
}


mExternC(Basic) const char* GetSurveyName(void)
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

    strcpy( surveyname.buf(), ptr );
    surveynamedirty = 0;

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	sprintf( dbgstrbuf, "GetSurveyName: %s", surveyname.buf() );
	od_debug_message( dbgstrbuf );
    }

    return surveyname;
}


/*-> implementing oddirs.h */

/* 'survey data' scope */

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
    strcpy( bddir.buf(), dir );
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
    strcpy( filenamebuf.buf(), mkFullPath(basedir,survnm) );

    if ( od_debug_isOn(DBG_SETTINGS) )
    {
	sprintf( dbgstrbuf, "GetDataDir: '%s'", filenamebuf.buf() );
	od_debug_message( dbgstrbuf );
    }

    return filenamebuf;
}


const char* GetProcFileName( const char* fname )
{
    static FileNameString filenamebuf;
    strcpy( filenamebuf.buf(), mkFullPath( GetDataDir(), "Proc" ) );
    if ( fname && *fname )
	strcpy( filenamebuf.buf(), mkFullPath( filenamebuf, fname ) );
    return filenamebuf;
}


const char* GetScriptsDir( const char* subdir )
{
    static FileNameString filenamebuf;
    const char* ret = GetEnvVar( "DTECT_SCRIPTS_DIR" );
    if ( ret && *ret )
	strcpy( filenamebuf.buf(), ret );
    else
    {
	strcpy( filenamebuf.buf(), GetProcFileName(0) );
	if ( subdir && *subdir )
	    strcpy( filenamebuf.buf(), mkFullPath(filenamebuf,subdir) );
    }

    return filenamebuf;
}



const char* GetSoftwareDir( int acceptnone )
{
    static BufferString res;
    
    if ( res.isEmpty() )
    {
	res = GetEnvVar( "DTECT_APPL" );
	if ( res.isEmpty() )
	{
	    FilePath filepath = GetFullExecutablePath();
	    res = filepath.dirUpTo( filepath.nrLevels()-5 );
	}
    
	if ( res.isEmpty() )
	{
	    if ( acceptnone )
		return 0;
	    
	    fprintf( stderr, "Cannot determine OpendTect location\n" );
	    ExitProgram( 1 );
	}
	
	if ( od_debug_isOn(DBG_SETTINGS) )
	{
	    BufferString dgbstrbuf( "GetSoftwareDir: '", res.str(), "'" );
	    od_debug_message( dbgstrbuf );
	}
    }
    
    return res;
}


const char* GetApplSetupDir(void)
{
    static char* ret = 0;
    static FileNameString filenamebuf;
    const char* envstr = 0;
    if ( ret )
	return *ret ? ret : 0;

    filenamebuf[0] = '\0';
    ret = filenamebuf.buf();

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
	strcpy( dirnm.buf(), mkFullPath(GetSoftwareDir(acceptnone),"data") );
    else
    {
	appldir = GetApplSetupDir();
	if ( !appldir ) return 0;
	strcpy( dirnm.buf(), mkFullPath(appldir,"data") );
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
	strcpy( filenm.buf(),
	        mkFullPath( GetSetupDataFileDir(lt, acceptnone), fnm ) );
	return filenm;
    }

    appldir = GetSetupDataFileDir(ODSetupLoc_ApplSetupOnly, acceptnone);
    if ( !appldir )
	return lt == ODSetupLoc_ApplSetupOnly ? 0
	     : GetSetupDataFileName(ODSetupLoc_SWDirOnly,fnm, acceptnone);

    strcpy( filenm.buf(),
	    mkFullPath( GetSetupDataFileDir(lt, acceptnone), fnm ) );

    if ( (lt == ODSetupLoc_ApplSetupPref || lt == ODSetupLoc_SWDirPref)
	&& !File::exists(filenm) )
    {
	/* try 'other' file */
	GetSetupDataFileName( lt == ODSetupLoc_ApplSetupPref
		? ODSetupLoc_SWDirOnly : ODSetupLoc_ApplSetupOnly, fnm,
				acceptnone );
	if ( File::exists(filenm) )
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
    strcpy( dirnm.buf(), mkFullPath(GetSoftwareDir(0),"doc") );
    strcpy( dirnm.buf(), mkFullPath(dirnm,filedir) );
    return dirnm;
}


const char* GetPlfSubDir(void)
{
    return __plfsubdir__;
}


const char* GetBinPlfDir(void)
{
    static BufferString res;
    if ( res.isEmpty() )
    {
	FilePath path = GetFullExecutablePath();
	res = path.pathOnly();
    }
 
    return res.buf();
}


static const char* gtExecScript( const char* basedir, int remote )
{
    static FileNameString scriptnm;
    strcpy( scriptnm.buf(), mkFullPath(basedir,"bin") );
    strcpy( scriptnm.buf(), mkFullPath(scriptnm,"od_exec") );
    if ( remote ) strcat( scriptnm.buf(), "_rmt" );
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

    if ( !fnm || !File::exists(fnm) )
	fnm = gtExecScript( GetSoftwareDir(0), remote );

    strcpy( progname.buf(), "'" );
    strcat( progname.buf(), fnm );
    strcat( progname.buf(), "' " );
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
    DWORD len = 256;
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
	  && File::isDirectory(val) )
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


const char* GetBinSubDir()
{
#ifndef __debug__
# ifdef __hassymbols__
    return "RelWithDebInfo";
# else
    return "Release";
# endif
#else
    return "Debug";
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
	    strcpy( dirnm.buf(), ptr );
	else
	    getHomeDir( dirnm.buf() );

	if ( od_debug_isOn(DBG_SETTINGS) )
	{
	    sprintf( dbgstrbuf, "GetPersonalDir: '%s'", dirnm.buf() );
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
	    strcpy( dirnm.buf(), ptr );
	else
	{
	    getHomeDir( dirnm.buf() );
	    strcpy( dirnm.buf(), mkFullPath(dirnm,".od") );
	}

	if ( !File::isDirectory(dirnm) )
	{
	    if ( File::exists(dirnm) )
		File::remove( dirnm );
	    if ( !File::createDir(dirnm) )
	    {
		fprintf( stderr, "Fatal: Cannot create '.od' directory in home "
				    "directory:\n%s\n", dirnm.buf() );
		ExitProgram( 1 );
	    }
	    if ( od_debug_isOn(DBG_SETTINGS) )
	    {
		sprintf( dbgstrbuf, "Had to create SettingsDir: '%s'",
			 dirnm.buf() );
		od_debug_message( dbgstrbuf );
	    }
	}

	if ( od_debug_isOn(DBG_SETTINGS) )
	{
	    sprintf( dbgstrbuf, "GetSettingsDir: '%s'", dirnm.buf() );
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
