/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "genc.h"
#include "oddirs.h"
#include "envvars.h"
#include "winutils.h"
#include "debugmasks.h"
#include "file.h"
#include "filepath.h"
#include "settings.h"
#include "survinfo.h"
#include "od_istream.h"
#include <iostream>

#include <string.h>
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

static BufferString cur_survey_name;

#define mPrDebug(fn,val) od_debug_message( BufferString(fn,": ",val) );



const char* SurveyInfo::surveyFileName()
{
    static const char* ret = 0;

    if ( !ret )
    {
	FilePath fp( GetSettingsDir(), "survey" );
	const char* ptr = GetSoftwareUser();
	if ( ptr )
	    fp.setExtension( ptr );
	static BufferString fnm;
	fnm = fp.fullPath();
	ret = fnm.buf();

	if ( od_debug_isOn(DBG_SETTINGS) )
	    mPrDebug( "SurveyInfo::surveyFileName", ret );
    }

    return *ret ? ret : 0;
}


void SurveyInfo::setSurveyName( const char* newnm )
{
    mSkipBlanks( newnm );
    cur_survey_name = newnm;
    removeTrailingBlanks( cur_survey_name.buf() );
}


const char* SurveyInfo::curSurveyName()
{
    if ( !cur_survey_name.isEmpty() )
	return cur_survey_name.buf();

    od_istream strm( SurveyInfo::surveyFileName() );
    if ( !strm.isOK() )
	return 0;

    strm.getLine( cur_survey_name );
    char* ptr = cur_survey_name.buf();
    mTrimBlanks( ptr );
    if ( !*ptr ) return 0;

    if ( od_debug_isOn(DBG_SETTINGS) )
	mPrDebug( "SurveyInfo::curSurveyName", cur_survey_name );

    return cur_survey_name;
}


mExternC(Basic) const char* GetSurveyName()
{
    return SurveyInfo::curSurveyName();
}


/*-> implementing oddirs.h */

/* 'survey data' scope */

mExternC(Basic) const char* GetBaseDataDir()
{
    const char* dir;

#ifdef __win__

    dir = GetEnvVar( "DTECT_WINDATA" );
    if ( !dir ) dir = getCleanWinPath( GetEnvVar("DTECT_DATA") );
    if ( !dir ) dir = getCleanWinPath( GetSettingsDataDir() );

    if ( dir && *dir && !GetEnvVar("DTECT_WINDATA") )
	SetEnvVar( "DTECT_WINDATA" , dir );

#else

    dir = GetEnvVar( "DTECT_DATA" );
    if ( !dir ) dir = GetSettingsDataDir();

#endif

    if ( !dir ) return 0;

    static BufferString ret;
    ret = dir;
    return ret.buf();
}


mExternC(Basic) const char* GetDataDir()
{
    const char* basedir = GetBaseDataDir();
    if ( !basedir || !*basedir )
	return lostinspace;

    const char* survnm = GetSurveyName();
    if ( !survnm || !*survnm )
	survnm = "_no_current_survey_";

    static BufferString ret;
    ret = FilePath( basedir, survnm ).fullPath();
    if ( od_debug_isOn(DBG_SETTINGS) )
	mPrDebug( "GetDataDir", ret );
    return ret.buf();
}


mExternC(Basic) const char* GetProcFileName( const char* fname )
{
    static BufferString ret;
    ret = FilePath( GetDataDir(), "Proc", fname ).fullPath();
    return ret.buf();
}


mExternC(Basic) const char* GetScriptsDir( const char* subdir )
{
    static BufferString ret;
    const char* envval = GetEnvVar( "DTECT_SCRIPTS_DIR" );
    ret = envval && *envval ? envval : GetProcFileName( subdir );
    return ret.buf();
}


mExternC(Basic) const char* GetSoftwareDir( int acceptnone )
{
    static BufferString res;
    
    if ( res.isEmpty() )
    {
	//Find the relinfo directory, and set sw dir to its parent
	const FilePath filepath = GetFullExecutablePath();
	for ( int idx=filepath.nrLevels()-1; idx>=0; idx-- )
	{
	    const FilePath datapath( filepath.dirUpTo(idx).buf(),"relinfo");
	    if ( File::isDirectory( datapath.fullPath()) )
	    {
		res = filepath.dirUpTo(idx);
		break;
	    }
	}
    
	if ( res.isEmpty() )
	{
	    if ( acceptnone )
		return 0;
	    
	    std::cerr << "Cannot determine OpendTect location ..." << std::endl;
	    ExitProgram( 1 );
	}
	
	if ( od_debug_isOn(DBG_SETTINGS) )
	    mPrDebug( "GetSoftwareDir", res );
    }
    
    return res.buf();
}


mExternC(Basic) const char* GetApplSetupDir()
{
    static const char* ret = 0;
    if ( !ret )
    {

	static BufferString bs;
	bs.setEmpty();
#ifdef __win__
	bs = GetEnvVar( "DTECT_WINAPPL_SETUP" );
#endif
	if ( bs.isEmpty() )
	    bs = GetEnvVar( "DTECT_APPL_SETUP" );

	ret = bs.buf();
    }
    return *ret ? ret : 0;
}


mExternC(Basic) const char* GetSetupDataFileDir( ODSetupLocType lt,
						 int acceptnone )
{
    const char* basedir;
    if ( lt > ODSetupLoc_ApplSetupPref )
	basedir = GetSoftwareDir( acceptnone );
    else
	basedir = GetApplSetupDir();
    if ( !basedir )
	return 0;

    static BufferString dirnm;
    dirnm = FilePath( basedir, "data" ).fullPath();
    return dirnm.buf();
}


mExternC(Basic) const char* GetSetupDataFileName( ODSetupLocType lt,
				const char* fnm, int acceptnone )
{
    static BufferString filenm;

    if ( lt == ODSetupLoc_SWDirOnly )
    {
	filenm = FilePath(GetSetupDataFileDir(lt,acceptnone),fnm).fullPath();
	return filenm.buf();
    }

    const char* appldir =
		GetSetupDataFileDir(ODSetupLoc_ApplSetupOnly,acceptnone);
    if ( !appldir )
	return lt == ODSetupLoc_ApplSetupOnly ? 0
	     : GetSetupDataFileName(ODSetupLoc_SWDirOnly,fnm,acceptnone);

    filenm = FilePath(GetSetupDataFileDir(lt,acceptnone),fnm).fullPath();

    if ( (lt == ODSetupLoc_ApplSetupPref || lt == ODSetupLoc_SWDirPref)
	&& !File::exists(filenm) )
    {
	/* try 'other' file */
	GetSetupDataFileName( lt == ODSetupLoc_ApplSetupPref
		? ODSetupLoc_SWDirOnly : ODSetupLoc_ApplSetupOnly, fnm,
				acceptnone );
	if ( File::exists(filenm) )
	    return filenm.buf();

	/* 'other' file also doesn't exist: revert */
	GetSetupDataFileName( lt == ODSetupLoc_ApplSetupPref
		? ODSetupLoc_ApplSetupOnly : ODSetupLoc_SWDirOnly, fnm,
				acceptnone );
    }

    return filenm.buf();
}


mExternC(Basic) const char* GetDocFileDir( const char* filedir )
{
    static BufferString dirnm;
    dirnm = FilePath(GetSoftwareDir(0),"doc",filedir).fullPath();
    return dirnm;
}


mExternC(Basic) const char* GetPlfSubDir()
{
    return __plfsubdir__;
}


mExternC(Basic) const char* GetBinPlfDir()
{
    static BufferString res;
    if ( res.isEmpty() )
	res = FilePath( GetFullExecutablePath() ).pathOnly();
    return res.buf();
}


static const char* gtExecScript( const char* basedir, int remote )
{
    static BufferString scriptnm;
    scriptnm = FilePath(basedir,"bin","od_exec").fullPath();
    if ( remote ) scriptnm.add( "_rmt" );
    return scriptnm;
}


mExternC(Basic) const char* GetExecScript( int remote )
{

#ifdef __msvc__
    return "";
#else

    const char* basedir = GetApplSetupDir();
    const char* fnm = 0;
    if ( basedir )
	fnm = gtExecScript( basedir, remote );

    if ( !fnm || !File::exists(fnm) )
	fnm = gtExecScript( GetSoftwareDir(0), remote );

    static BufferString progname;
    progname.set( "'" ).add( fnm ).add( "' " );
    return progname.buf();
#endif
}


mExternC(Basic) const char* GetSoftwareUser()
{
    static const char* ret = 0;
    if ( !ret )
    {
	const char* envval = 0;
#ifdef __win__
	envval = GetEnvVar( "DTECT_WINUSER" );
#endif
	if ( !envval || !*envval )
	    envval = GetEnvVar( "DTECT_USER" );
	if ( od_debug_isOn(DBG_SETTINGS) )
	    mPrDebug( "GetSoftwareUser", envval ? envval : "<None>" );

	static BufferString bs;
	bs = envval;
	ret = bs.buf();
    }

    return *ret ? ret : 0;
}


mExternC(Basic) const char* GetUserNm()
{
#ifdef __win__
    static char usernm[256];
    DWORD len = 256;
    GetUserName( usernm, &len );
    return usernm;
#else
    static BufferString ret;
    ret = GetEnvVar( "USER" );
    return ret.isEmpty() ? 0 : ret.buf();
#endif
}


static void getHomeDir( BufferString& homedir )
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
	homedir.set( GetEnvVar("HOMEDRIVE") ).add( GetEnvVar("HOMEPATH") );
	if ( !homedir.isEmpty() && !caseInsensitiveEqual(homedir.buf(),"c:\\",0)
	  && File::isDirectory(homedir) )
	    dir = homedir.buf();
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

    if ( dir != homedir.buf() )
	homedir = dir;

#ifdef __win__
    if ( !GetEnvVar("DTECT_WINHOME") )
	SetEnvVar( "DTECT_WINHOME", homedir.buf() );
    replaceCharacter( homedir.buf(), '\r', '\0' );
#endif
}


mExternC(Basic) const char* GetBinSubDir()
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

mExternC(Basic) const char* GetPersonalDir()
{
    static const char* ret = 0;

    if ( !ret )
    {
	static BufferString dirnm;
	const char* ptr = GetEnvVar( "DTECT_PERSONAL_DIR" );
	if ( ptr )
	    dirnm = ptr;
	else
	    getHomeDir( dirnm );

	if ( od_debug_isOn(DBG_SETTINGS) )
	    mPrDebug( "GetPersonalDir", dirnm );

	ret = dirnm.buf();
    }

    return ret;
}


mExternC(Basic) const char* GetSettingsDir()
{
    static const char* ret = 0;

    if ( !ret )
    {
	const char* ptr = 0;
#ifdef __win__
	ptr = GetEnvVar( "DTECT_WINSETTINGS" );
	if( !ptr ) ptr = getCleanWinPath( GetEnvVar("DTECT_SETTINGS") );
#else
	ptr = GetEnvVar( "DTECT_SETTINGS" );
#endif

	static BufferString dirnm;
	if ( ptr )
	    dirnm = ptr;
	else
	{
	    getHomeDir( dirnm );
	    dirnm = FilePath( dirnm, ".od" ).fullPath();
	}

	if ( !File::isDirectory(dirnm) )
	{
	    if ( File::exists(dirnm) )
		File::remove( dirnm );
	    if ( !File::createDir(dirnm) )
	    {
		std::cerr << "Fatal: Cannot create '.od' directory in home "
		    "directory:\n" << dirnm.buf() << std::endl;
		ExitProgram( 1 );
	    }
	    if ( od_debug_isOn(DBG_SETTINGS) )
		mPrDebug( "Had to create SettingsDir", dirnm );
	}

	if ( od_debug_isOn(DBG_SETTINGS) )
	    mPrDebug( "GetSettingsDir", dirnm );

	ret = dirnm.buf();
    }

    return ret;
}


mExternC(Basic) const char* GetSettingsFileName( const char* fnm )
{
    static BufferString ret;
    ret = FilePath( GetSettingsDir(), fnm ).fullPath();
    return ret;
}
