/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "oddirs.h"

#include "applicationdata.h"
#include "debug.h"
#include "dirlist.h"
#include "envvars.h"
#include "genc.h"
#include "file.h"
#include "filepath.h"
#include "od_istream.h"
#include "plugins.h"
#include "pythonaccess.h"
#include "settings.h"
#include "survinfo.h"
#include "thread.h"
#include "winutils.h"

#include <iostream>
#include <QStandardPaths>

#ifdef __msvc__
# include <direct.h>
# include <Lmcons.h>
#endif
#ifdef __win__
#include "windows.h"
# define sDirSep	"\\"
static const char* lostinspace = "C:\\";
#else
# include <dlfcn.h>
# define sDirSep	"/"
static const char* lostinspace = "/tmp";
#endif

#ifdef __mac__
# include <CoreServices/CoreServices.h>
#endif

static BufferString cur_survey_name;

#define mPrDebug(fn,val) od_debug_message( BufferString(fn,": ",val) );

static const char* sShare = "share";
static const char* sDoc = "doc";
static const char* sResources = "Resources";

static BufferString sExportToDir;
static BufferString sImportFromDir;
static BufferString sPicturesDir;


const char* SurveyInfo::surveyFileName()
{
    mDeclStaticString( fnm );

    if ( fnm.isEmpty() )
    {
	FilePath fp( GetSettingsDir(), "survey" );
	const char* ptr = GetSoftwareUser();
	if ( ptr )
	    fp.setExtension( ptr );


	fnm = fp.fullPath();

	if ( od_debug_isOn(DBG_SETTINGS) )
	    mPrDebug( "SurveyInfo::surveyFileName", fnm.buf() );
    }

    return fnm.str();
}


Notifier<SurveyInfo>& SurveyInfo::rootDirChanged()
{
    static PtrMan<Notifier<SurveyInfo> > thenotif =
					new Notifier<SurveyInfo>( nullptr );
    return *thenotif.ptr();
}


static Threads::Lock& GetSurvChangeLock()
{
    static PtrMan<Threads::Lock> lock = new Threads::Lock();
    return *lock.ptr();
}


void SurveyInfo::setSurveyName( const char* newnm )
{
    Threads::Locker lock( GetSurvChangeLock() );
    if ( StringView(newnm) != cur_survey_name )
	rootDirChanged().trigger();

    cur_survey_name = newnm;
    cur_survey_name.trimBlanks();
}


const char* SurveyInfo::curSurveyName()
{
    Threads::Locker lock( GetSurvChangeLock() );
    if ( !cur_survey_name.isEmpty() )
	return cur_survey_name.buf();

    od_istream strm( surveyFileName() );
    if ( !strm.isOK() )
	return nullptr;

    strm.getLine( cur_survey_name );
    cur_survey_name.trimBlanks();

    if ( cur_survey_name.isEmpty() )
	return nullptr;

    if ( od_debug_isOn(DBG_SETTINGS) )
	mPrDebug( "SurveyInfo::curSurveyName", cur_survey_name );

    return cur_survey_name.buf();
}


mExternC(Basic) const char* GetSurveyName()
{
    return SurveyInfo::curSurveyName();
}


/*-> implementing oddirs.h */

/* 'survey data' scope */


static BufferString basedatadir;

extern "C" { mGlobal(Basic) void SetCurBaseDataDir(const char*); }
mExternC(Basic) void SetCurBaseDataDir( const char* dirnm )
{
    Threads::Locker lock( GetSurvChangeLock() );
    if ( StringView(dirnm) != basedatadir )
	SurveyInfo::rootDirChanged().trigger();

    if ( dirnm && *dirnm )
	basedatadir.set( dirnm );
    else
	basedatadir.setEmpty();
}

mExternC(Basic) const char* GetBaseDataDir()
{
    Threads::Locker lock( GetSurvChangeLock() );
    if ( !basedatadir.isEmpty() )
	return basedatadir.buf();

    const char* dir;
#ifdef __win__
    dir = GetEnvVar( "DTECT_WINDATA" );
    if ( !dir )
    {
	dir = getCleanWinPath( GetEnvVar("DTECT_DATA") );
	if ( !dir )
	    dir = getCleanWinPath( GetSettingsDataDir() );
    }
#else
    dir = GetEnvVar( "DTECT_DATA" );
    if ( !dir )
	dir = GetSettingsDataDir();
#endif

    if ( !dir )
	return nullptr;

    basedatadir.set( dir );
    return basedatadir.buf();
}


mExternC(Basic) const char* GetDataDir()
{
    const char* basedir = GetBaseDataDir();
    if ( !basedir || !*basedir )
	return nullptr;

    const char* survnm = GetSurveyName();
    if ( !survnm || !*survnm )
	survnm = "_no_current_survey_";

    mDeclStaticString( ret );
    ret = FilePath( basedir, survnm ).fullPath();
    if ( od_debug_isOn(DBG_SETTINGS) )
	mPrDebug( "GetDataDir", ret );
    return ret.buf();
}


mExternC(Basic) const char* GetProcFileName( const char* fname )
{
    mDeclStaticString( ret );
    ret = FilePath( GetSurveyProcDir(), fname ).fullPath();
    return ret.buf();
}


mExternC(Basic) const char* GetScriptsDir()
{
    mDeclStaticString( ret );
    const char* envval = GetEnvVar( "DTECT_SCRIPTS_DIR" );
    ret = envval && *envval ? envval : GetSurveyScriptsDir();
    return ret.buf();
}


mExternC(Basic) const char* GetScriptsLogDir()
{
    mDeclStaticString( ret );
    const char* envval = GetEnvVar( "DTECT_SCRIPTS_LOG_DIR" );
    ret = envval && *envval ? envval : GetSurveyScriptsLogDir();
    return ret.buf();
}


mExternC(Basic) const char* GetScriptsPicturesDir()
{
    mDeclStaticString( ret );
    ret = GetEnvVar( "DTECT_SCRIPTS_PICTURES_DIR" );
    if ( ret.isEmpty() )
    {
	ret = FilePath( GetScriptsLogDir(), "Pictures" ).fullPath();
	if ( !File::exists(ret) )
	    File::createDir( ret );
    }

    return ret.buf();
}


mExternC(Basic) const char* GetShellScript( const char* nm )
{
    mDeclStaticString( res );
    if ( !nm || !*nm )
	return GetScriptDir();

    res = FilePath(GetScriptDir(),nm).fullPath();
    return res.buf();
}


mExternC(Basic) const char* GetPythonScript( const char* nm )
{
    const StringView fnm( nm );
    if ( fnm.isEmpty() )
	return nullptr;

    const BufferStringSet pythondirs( OD::PythA().getBasePythonPath() );
    if ( pythondirs.isEmpty() )
	return nullptr;

    mDeclStaticString( res );
    res.setEmpty();
    const BufferStringSet modulenms( "odpy", "dgbpy" );
    for ( const auto* modulenm : modulenms )
    {
	for ( const auto* pythondir : pythondirs )
        {
	    const FilePath pythonfp( pythondir->buf(), modulenm->str(), nm );
            const BufferString scriptfnm( pythonfp.fullPath() );
	    const char* scriptfpstr = scriptfnm.buf();
	    if ( File::exists(scriptfpstr) && File::isReadable(scriptfpstr) &&
		 File::isFile(scriptfpstr) )
            {
                res = scriptfnm;
                return res.buf();
            }
        }
    }

    return res.buf();
}


mExternC(Basic) const char* GetSoftwareDir( bool acceptnone )
{
    mDeclStaticString( res );

    if ( res.isEmpty() )
    {
	const char* relinfostr = "relinfo";
	//Find the relinfo directory, and set sw dir to its parent
	const BufferString dtectappl( GetOSEnvVar("DTECT_APPL") );
	if ( File::exists(dtectappl.buf()) )
	{
	    FilePath datapath( dtectappl );
	    const int nrlevels = datapath.nrLevels();
	    if ( __ismac__ )
		datapath.add( sResources );

	    datapath.add( relinfostr );
	    if ( datapath.exists() && File::isDirectory( datapath.fullPath() ) )
		res = datapath.dirUpTo( nrlevels-1 );
	}

	if ( res.isEmpty() )
	{
	    const FilePath filepath = GetFullExecutablePath();
	    for ( int idx=filepath.nrLevels()-1; idx>=0; idx-- )
	    {
		FilePath datapath( filepath.dirUpTo(idx).buf() );
		if ( __ismac__ )
		    datapath.add( sResources );

		datapath.add( relinfostr );
		if ( !datapath.exists() )
		    continue;

		if ( File::isDirectory( datapath.fullPath()) )
		{
		    res = filepath.dirUpTo(idx);
		    break;
		}
	    }
	}

	if ( res.isEmpty() )
	{
	    if ( acceptnone )
		return nullptr;

	    std::cerr << "Cannot determine OpendTect location ..." << std::endl;
	    ApplicationData::exit( 1 );
	}

	if ( od_debug_isOn(DBG_SETTINGS) )
	    mPrDebug( "GetSoftwareDir", res );
    }

    return res.buf();
}


mExternC(Basic) const char* GetLibraryFnm( const void* fn )
{
    mDeclStaticString( res );
#ifdef __win__
    HMODULE hModule = NULL;
    if ( GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,(LPCTSTR)fn,
			   &hModule) )
    {
	TCHAR path[MAX_PATH];
	const DWORD length = GetModuleFileName( hModule, path, MAX_PATH );
	if ( length > 0 && length < MAX_PATH )
	    res.set( path );
    }
#else
    Dl_info info;
    if ( dladdr(fn,&info) )
	res.set( info.dli_fname );
#endif

    return  res.buf();
}


namespace OD
{

static const char* GetProjectSourceDir()
{
    mDeclStaticString( ret );
#ifdef __FILE__
    if ( ret.isEmpty() )
    {
	const FilePath srcdir( __FILE__ );
	ret = srcdir.dirUpTo( srcdir.nrLevels()-4 );
    }
#endif
    return ret.buf();
}


static const char* GetProjectBinaryDir()
{
    mDeclStaticString( ret );
#ifdef __od_build_dir__
    if ( ret.isEmpty() )
    {
	const FilePath bindir( __od_build_dir__ );
	ret = bindir.fullPath();
    }
#endif
    return ret.buf();
}


static const char* GetProjectInstDir()
{
    mDeclStaticString( ret );
#ifdef __od_install_dir__
    if ( ret.isEmpty() )
    {
	const FilePath instdir( __od_install_dir__ );
	ret = instdir.fullPath();
    }
#endif
    return ret.buf();
}


static const char* GetCurrentLibraryFnm()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
	ret = GetLibraryFnm( (const void*)GetCurrentLibraryFnm );

    return ret;
}


mExternC(Basic) bool isDeveloperBuild()
{
    static int ret = -1;
    if ( ret < 0 )
    {
	const StringView libfnm( GetCurrentLibraryFnm() );
	if ( libfnm.isEmpty() )
	    ret = 0;
	else
	{
	    FilePath projfp( GetProjectBinaryDir() );
	    FilePath libfp( libfnm.buf() );
	    projfp.makeCanonical();
	    libfp.makeCanonical();
	    ret = projfp.exists() && libfp.isSubDirOf( projfp ) ? 1 : 0;
	}
    }

    return ret == 1;
}


mExternC(Basic) bool isDeveloperInstallation()
{
    static int ret = -1;
    if ( ret < 0 )
    {
	const StringView libfnm( GetCurrentLibraryFnm() );
	if ( libfnm.isEmpty() )
	    ret = 0;
	else
	{
	    FilePath projfp( GetProjectInstDir() );
	    FilePath libfp( libfnm.buf() );
	    projfp.makeCanonical();
	    libfp.makeCanonical();
	    ret = projfp.exists() && libfp.isSubDirOf( projfp ) ? 1 : 0;
	}
    }

    return ret == 1;
}

} // namespace OD


static	const char* GetApplSetupDir_()
{
    mDeclStaticString( bs );
    if ( bs.isEmpty() )
    {
	if ( __iswin__ )
	    bs = GetEnvVar( "DTECT_WINAPPL_SETUP" );

	if ( bs.isEmpty() )
	    bs = GetEnvVar( "DTECT_APPL_SETUP" );
    }

    return bs.buf();
}


static const char* GetSoftwareDataDir( bool acceptnone )
{
    FilePath basedir = GetSoftwareDir( acceptnone );
    if ( basedir.isEmpty() )
	return nullptr;

    if ( __ismac__ )
	basedir.add( sResources );

    basedir.add( sShare );

    mDeclStaticString( dirnm );
    dirnm = basedir.fullPath();
    return dirnm.str();
}


static const char* GetUserPluginSoftwareDataDir()
{
    FilePath basedir = PluginManager::getUserDir();
    if ( basedir.isEmpty() )
	return nullptr;

    if ( __ismac__ )
	basedir.add( sResources );

    basedir.add( sShare );

    mDeclStaticString( dirnm );
    dirnm = basedir.fullPath();
    return dirnm.str();
}


namespace OD
{

const BufferStringSet& getCustomShareDirs()
{
    static PtrMan<BufferStringSet> ret = new BufferStringSet();
    if ( ret->isEmpty() )
    {
	if ( isDeveloperBuild() )
	{
	    const FilePath fp( GetProjectSourceDir(), sShare );
	    ret->add( fp.fullPath() );
	}
	else
	{ //old location, compatibility with external plugins
	    const FilePath fp( GetSoftwareDir(true), "data" );
	    ret->add( fp.fullPath() );
	}
    }

    return *ret.ptr();
}


const BufferStringSet& getCustomDocDirs()
{
    static PtrMan<BufferStringSet> ret = new BufferStringSet();
    if ( ret->isEmpty() )
    {
	if ( isDeveloperBuild() )
	{
	    const FilePath fp( GetProjectSourceDir(), sDoc );
	    ret->add( fp.fullPath() );
	}
	else
	{ //old location, compatibility with external plugins
	    const FilePath fp( GetSoftwareDir(true), sDoc );
	    if ( fp.exists() )
		ret->add( fp.fullPath() );
	}
    }

    return *ret.ptr();
}

} // namespace OD


mExternC(Basic) bool addCustomShareFolder( const char* path )
{
    if ( !File::isDirectory(path) )
	return false;

    auto& customsharedirs =
		    const_cast<BufferStringSet&>( OD::getCustomShareDirs() );
    return customsharedirs.addIfNew( path );
}


mExternC(Basic) const char* GetSetupDataFileDir( ODSetupLocType lt,
						 bool acceptnone )
{
    FilePath fp;
    if ( lt == ODSetupLoc_ApplSetupOnly || lt == ODSetupLoc_SWDirOnly ||
	 lt == ODSetupLoc_UserPluginDirOnly )
    {
	if ( lt == ODSetupLoc_ApplSetupOnly )
	    fp.set( GetApplSetupDir_() );
	else if ( lt == ODSetupLoc_SWDirOnly )
	    fp.set( GetSoftwareDataDir(acceptnone) );
	else if ( lt == ODSetupLoc_UserPluginDirOnly )
	    fp.set( GetUserPluginSoftwareDataDir() );
    }
    else
    {
	if ( lt == ODSetupLoc_ApplSetupPref )
	    fp.set( GetApplSetupDir_() );
	else if ( lt == ODSetupLoc_SWDirPref )
	    fp.set( GetSoftwareDataDir(true) );

	if ( !fp.exists() )
	{
	    if ( lt == ODSetupLoc_ApplSetupPref )
		fp.set( GetSoftwareDataDir(true) );
	    else if ( lt == ODSetupLoc_SWDirPref )
		fp.set( GetApplSetupDir_() );

	    if ( !fp.exists() )
		fp.set( GetUserPluginSoftwareDataDir() );

	    if ( !fp.exists() )
		fp.set( nullptr );
	}
    }

    mDeclStaticString( dirnm );
    dirnm = fp.fullPath();
    return dirnm.buf();
}


mExternC(Basic) const char* GetSWDirDataDir()
{
    return GetSetupDataFileDir( ODSetupLoc_SWDirOnly, false );
}


mExternC(Basic) const char* GetApplSetupDataDir()
{
    return GetSetupDataFileDir( ODSetupLoc_ApplSetupOnly, true );
}


mExternC(Basic) const char* GetUserPluginDataDir()
{
    return GetSetupDataFileDir( ODSetupLoc_ApplSetupOnly, true );
}


mExternC(Basic) const char* GetSetupShareFileName( const char* fnm,
					  ODSetupLocType lt, bool acceptnone )
{
    acceptnone = acceptnone || lt != ODSetupLoc_SWDirOnly;
    mDeclStaticString( ret );
    FilePath fp;
    if ( lt == ODSetupLoc_ApplSetupOnly || lt == ODSetupLoc_SWDirOnly ||
	 lt == ODSetupLoc_UserPluginDirOnly )
    {
	fp.set( GetSetupDataFileDir(lt,acceptnone) ).add(fnm );
    }
    else
    {
	if ( lt == ODSetupLoc_ApplSetupPref )
	    fp.set( GetSetupDataFileDir(ODSetupLoc_ApplSetupOnly,acceptnone) );
	else if ( lt == ODSetupLoc_SWDirPref )
	    fp.set( GetSetupDataFileDir(ODSetupLoc_SWDirOnly,acceptnone) );

	fp.add( fnm );
	if ( !fp.exists() )
	{
	    if ( lt == ODSetupLoc_ApplSetupPref )
		fp.set( GetSetupDataFileDir(ODSetupLoc_SWDirOnly,
					    acceptnone) );
	    else if ( lt == ODSetupLoc_SWDirPref )
		fp.set( GetSetupDataFileDir(ODSetupLoc_ApplSetupOnly,
					    acceptnone) );
	    fp.add( fnm );
	    if ( !fp.exists() )
	    {
		fp.set( GetSetupDataFileDir(ODSetupLoc_UserPluginDirOnly,
					    acceptnone) ).add( fnm );
		if ( !fp.exists() )
		    fp.set( nullptr );
	    }
	}
    }

    if ( fp.exists() )
	ret = fp.fullPath();
    else
    {
	ret.setEmpty();
	if ( lt == ODSetupLoc_ApplSetupPref ||
	     lt == ODSetupLoc_SWDirPref || lt == ODSetupLoc_SWDirOnly )
	{
	    for ( const auto* dir : OD::getCustomShareDirs() )
	    {
		fp.set( dir->str() ).add( fnm );
		if ( !fp.exists() )
		    continue;

		ret = fp.fullPath();
		break;
	    }
	}
    }

    if ( ret.isEmpty() && !acceptnone )
    {
	const BufferString msg( "Cannot find configuration file '", fnm,
				"' in the share folder(s) " );
	pFreeFnErrMsg( msg.str() );
    }

    return ret.buf();
}


mExternC(Basic) const char* GetSWSetupShareFileName( const char* fnm,
						     bool acceptnone )
{
    return GetSetupShareFileName( fnm, ODSetupLoc_SWDirOnly, acceptnone );
}


static bool GetSetupShareFileNames( const char* searchkey,
				    File::DirListType dltype,
				    BufferStringSet& ret, bool acceptnone )
{
    BufferStringSet sharedirnms;
    BufferString sharedirnm =
			GetSetupDataFileDir( ODSetupLoc_ApplSetupOnly, true );
    if ( File::isDirectory(sharedirnm.buf()) )
	sharedirnms.addIfNew( sharedirnm.str() );

    sharedirnm = GetSetupDataFileDir( ODSetupLoc_SWDirOnly, true );
    if ( File::isDirectory(sharedirnm.buf()) )
	sharedirnms.addIfNew( sharedirnm.str() );

    sharedirnm = GetSetupDataFileDir( ODSetupLoc_UserPluginDirOnly, true );
    if ( File::isDirectory(sharedirnm.buf()) )
	sharedirnms.addIfNew( sharedirnm.str() );

    for ( const auto* dir : OD::getCustomShareDirs() )
	sharedirnms.addIfNew( dir->str() );

    for ( const auto* dirnm : sharedirnms )
    {
	const DirList dl( dirnm->str(), dltype, searchkey );
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    const BufferString filenm = dl.fullPath( idx );
	    ret.addIfNew( filenm.str() );
	}
    }

    if ( ret.isEmpty() && !acceptnone )
    {
	BufferString msg( "Cannot find any matching '", searchkey,
			  "' " );
	msg.add( dltype == File::DirListType::FilesInDir
			? "file(s)" : "folder(s)" )
	   .add( " in the share folder(s)" );
	pFreeFnErrMsg( msg.str() );
    }

    return !ret.isEmpty();
}


mExternC(Basic) bool GetSetupShareFileNames( const char* searchkey,
					     BufferStringSet& ret,
					     bool acceptnone )
{
    return GetSetupShareFileNames( searchkey,File::DirListType::FilesInDir, ret,
				   acceptnone );
}


mExternC(Basic) bool GetSetupShareDirNames( const char* searchkey,
					    BufferStringSet& ret,
					    bool acceptnone )
{
    return GetSetupShareFileNames( searchkey,File::DirListType::DirsInDir, ret,
				   acceptnone );
}


mExternC(Basic) const char* GetSetupShareFileInDir( const char* subdir,
						    const char* filenm,
						    bool acceptnone )
{
    mDeclStaticString( ret );
    ret.setEmpty();

    BufferStringSet dirnms;
    if ( GetSetupShareDirNames(subdir,dirnms,acceptnone) )
    {
	for ( const auto* dirnm : dirnms )
	{
	    const FilePath fp( dirnm->str(), filenm );
	    if ( !fp.exists() )
		continue;

	    ret = fp.fullPath();
	    break;
	}
    }

    if ( ret.isEmpty() && !acceptnone )
    {
	BufferString msg( "Cannot find configuration file '", filenm,
			  "' in the subdir '");
	msg.add( subdir ).add( "' share folder(s)" );
	pFreeFnErrMsg( msg.str() );
    }

    return ret.buf();
}

mExternC(Basic) bool GetSetupShareFilesInDir( const char* subdir,
					      const char* searchkey,
					      BufferStringSet& ret,
					      bool acceptnone )
{
    BufferStringSet dirnms;
    if ( GetSetupShareDirNames(subdir,dirnms,acceptnone) )
    {
	const File::DirListType dltyp = File::DirListType::FilesInDir;
	for ( const auto* dirnm : dirnms )
	{
	    const DirList dl( dirnm->str(), dltyp, searchkey );
	    for ( int idx=0; idx<dl.size(); idx++ )
	    {
		const BufferString filenm = dl.fullPath( idx );
		ret.addIfNew( filenm.str() );
	    }
	}
    }

    if ( ret.isEmpty() && !acceptnone )
    {
	BufferString msg( "Cannot find any matching '", searchkey,
			  "' files in the subdir '" );
	msg.add( subdir ).add( "' of the share folder(s)" );
	pFreeFnErrMsg( msg.str() );
    }

    return !ret.isEmpty();
}


mExternC(Basic) bool addCustomDocFolder( const char* path )
{
    if ( !File::isDirectory(path) )
	return false;

    auto& customdocdirs =
		    const_cast<BufferStringSet&>( OD::getCustomDocDirs() );
    return customdocdirs.addIfNew( path );
}


mExternC(Basic) const char* GetDocFileDir( const char* file_or_dir )
{
    mDeclStaticString( dirnm );
    FilePath fp( GetSoftwareDir(false) );
    if ( __ismac__ )
	fp.add( sResources );

    fp.add( sDoc ).add( file_or_dir );
    dirnm = fp.fullPath();
    if ( !File::exists(dirnm.buf()) )
    {
	for ( const auto* dir : OD::getCustomDocDirs() )
	{
	    fp.set( dir->str() ).add( file_or_dir );
	    if ( !fp.exists() )
		continue;

	    dirnm = fp.fullPath();
	    break;
	}
    }

    return dirnm.buf();
}


mExternC(Basic) const char* GetPlfSubDir()
{
    return __plfsubdir__;
}


mExternC(Basic) const char* GetExecPlfDir()
{
    mDeclStaticString( res );
    if ( res.isEmpty() )
    {
	FilePath fp( GetSoftwareDir(false) );
	if ( __ismac__ )
	{
	    fp.add( "MacOS" );
	}
	else
	{
	    fp.add( "bin" ).add( GetPlfSubDir() );
	}

	fp.add( GetBinSubDir() );
	res = fp.fullPath();
    }

    return res.buf();
}


mExternC(Basic) const char* GetLibPlfDir()
{
    mDeclStaticString( res );
    if ( res.isEmpty() )
    {
	FilePath fp( GetSoftwareDir(false) );
	if ( __ismac__ )
	{
	    fp.add( "Frameworks" );
	}
	else
	{
	    fp.add( "bin" ).add( GetPlfSubDir() );
	}

	fp.add( GetBinSubDir() );
	res = fp.fullPath();
    }

    return res.buf();
}


mExternC(Basic) const char* GetScriptDir()
{
    mDeclStaticString( res );
    if ( res.isEmpty() )
    {
	FilePath fp( GetSoftwareDir(false) );
	if ( __ismac__ )
	    fp.add( sResources );

	fp.add( "bin" );
	res = fp.fullPath();
    }

    return res.buf();
}


static const char* gtExecScript( const char* basedir, int remote )
{
    mDeclStaticString( scriptnm );
    scriptnm = FilePath(basedir,"bin","od_exec").fullPath();
    if ( remote ) scriptnm.add( "_rmt" );
    return scriptnm;
}


mExternC(Basic) const char* GetExecScript( int remote )
{
#ifdef __msvc__
    return nullptr;
#else

    const BufferString basedir =
			GetSetupDataFileDir( ODSetupLoc_ApplSetupPref, false );
    BufferString fnm;
    if ( File::exists(basedir.buf()) )
	fnm = gtExecScript( basedir.str(), remote );

    mDeclStaticString( progname );
    progname.set( "'" ).add( fnm ).add( "' " );
    return progname.buf();
#endif
}


mExternC(Basic) const char* GetODExternalScript()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
    {
	FilePath retfp( GetScriptDir(), "od_external" );
	retfp.setExtension( __iswin__ ? "bat" : "sh" );
	if ( retfp.exists() )
	    ret.set( retfp.fullPath() );
    }

    return ret;
}


mExternC(Basic) const char* GetSoftwareUser()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
    {
	const char* envval = nullptr;
	if ( __iswin__ )
	    envval = GetEnvVar( "DTECT_WINUSER" );

	if ( !envval || !*envval )
	    envval = GetEnvVar( "DTECT_USER" );

	if ( od_debug_isOn(DBG_SETTINGS) )
	    mPrDebug( "GetSoftwareUser", envval ? envval : "<None>" );

	ret = envval;
    }

    return ret.str();
}


mExternC(Basic) const char* GetUserNm()
{
    mDeclStaticString( ret );
#ifdef __win__
    mDefineStaticLocalObject( TCHAR, usernm, [UNLEN+1] );
    DWORD len = UNLEN+1;
    GetUserName( usernm, &len );
    WinUtils::copyWString( usernm, ret );
#else
    ret = GetEnvVar( "USER" );
#endif
    return ret.buf();
}


mExternC(Basic) const char* GetInterpreterName()
{
    const StringView dtectuser = GetSoftwareUser();
    if ( dtectuser.isEmpty() )
	return GetUserNm();

    return dtectuser;
}


static void getHomeDir( BufferString& homedir )
{
    const char* dir;

#ifdef __unix__

    dir = GetEnvVar( "DTECT_HOME" );
    if ( !dir )
	dir = GetEnvVar( "HOME" );

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
		// during initialization of statics. (0xc0000005)
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

    homedir.replace( '\r', '\0' );
#endif
}


mExternC(Basic) const char* GetBinSubDir()
{
    if ( OD::InDebugMode() )
	return "Debug";

    return __ismac__ ? "" : "Release";
}


mExternC(Basic) const char* GetPersonalDir()
{
    mDeclStaticString( dirnm );

    if ( dirnm.isEmpty() )
    {
	const char* ptr = GetEnvVar( "DTECT_PERSONAL_DIR" );
	if ( ptr )
	    dirnm = ptr;
	else
	    getHomeDir( dirnm );

	if ( od_debug_isOn(DBG_SETTINGS) )
	    mPrDebug( "GetPersonalDir", dirnm );
    }

    return dirnm.buf();
}


mExternC(Basic) const char* GetDownloadsDir()
{
    mDeclStaticString( ret );
    if ( !ret.isEmpty() )
	return ret.buf();

    ret = QStandardPaths::writableLocation( QStandardPaths::DownloadLocation );
    if ( !ret.isEmpty() )
	return ret.buf();

    const FilePath fp( GetPersonalDir(), "Downloads" );
    ret = fp.fullPath();
    if ( File::isDirectory(ret) || File::createDir(ret) )
	return ret.buf();

    return File::getTempPath();
}


mExternC(Basic) const char* GetSettingsDir()
{
    mDeclStaticString( dirnm );

    if ( dirnm.isEmpty() )
    {
	const char* ptr = nullptr;
	ptr = GetOSEnvVar( __iswin__ ? "DTECT_WINSETTINGS"
				     : "DTECT_SETTINGS" );
#ifdef __win__
	if ( !ptr )
	    ptr = getCleanWinPath( GetEnvVar("DTECT_SETTINGS") );
#endif

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
		std::cerr << "Fatal: Cannot create '.od' folder in home "
		    "folder:\n" << dirnm.buf() << std::endl;
		ApplicationData::exit( 1 );
	    }

	    if ( od_debug_isOn(DBG_SETTINGS) )
		mPrDebug( "Had to create SettingsDir", dirnm );
	}

	if ( od_debug_isOn(DBG_SETTINGS) )
	    mPrDebug( "GetSettingsDir", dirnm );
    }

    return dirnm.buf();
}


mExternC(Basic) const char* GetSettingsFileName( const char* fnm )
{
    mDeclStaticString( ret );
    ret = FilePath( GetSettingsDir(), fnm ).fullPath();
    return ret;
}


static void getSurveySubDir( const char* subdir, BufferString& ret )
{
    ret = FilePath( GetDataDir(), subdir ).fullPath();
    if ( !File::exists(ret) )
	File::createDir( ret );
}


mExternC(Basic) const char* GetSurveyExportDir()
{
    mDeclStaticString( ret );
    getSurveySubDir( "Export", ret );
    return ret.buf();
}


mExternC(Basic) const char* GetSurveyPicturesDir()
{
    mDeclStaticString( ret );
    getSurveySubDir( "Pictures", ret );
    return ret;
}


mExternC(Basic) const char* GetSurveyScriptsDir()
{
    mDeclStaticString( ret );
    getSurveySubDir( "Scripts", ret );
    return ret;
}


mExternC(Basic) const char* GetSurveyScriptsLogDir()
{
    mDeclStaticString( ret );
    getSurveySubDir( "Scripts", ret );
    ret = FilePath( ret, "Log" ).fullPath();
    if ( !File::exists(ret) )
	File::createDir( ret );

    return ret;
}


mExternC(Basic) const char* GetSurveyTempDir()
{
    mDeclStaticString( ret );
    getSurveySubDir( "Temp", ret );
    return ret;
}


mExternC(Basic) const char* GetSurveyProcDir()
{
    mDeclStaticString( ret );
    getSurveySubDir( "Proc", ret );
    return ret;
}


mExternC(Basic) const char* GetImportFromDir()
{
    if ( sImportFromDir.isEmpty() )
	sImportFromDir = GetDataDir();

    return sImportFromDir;
}


mExternC(Basic) void SetImportFromDir( const char* dirnm )
{
    sImportFromDir = dirnm;
}


mExternC(Basic) const char* GetExportToDir()
{
    if ( sExportToDir.isEmpty() )
	sExportToDir = GetSurveyExportDir();

    return sExportToDir;
}


mExternC(Basic) void SetExportToDir( const char* dirnm )
{
    sExportToDir = dirnm;
}


mExternC(Basic) const char* GetPicturesDir()
{
    if ( sPicturesDir.isEmpty() )
	sPicturesDir = GetSurveyPicturesDir();

    return sPicturesDir;
}


mExternC(Basic) void SetPicturesDir( const char* dirnm )
{
    sPicturesDir = dirnm;
}


mExternC(Basic) void ResetDefaultDirs()
{
    sImportFromDir.setEmpty();
    sExportToDir.setEmpty();
    sPicturesDir.setEmpty();
}


// Deprecated implementations

mExternC(Basic) const char* GetApplSetupDir()
{
    return GetApplSetupDir_();
}


mExternC(Basic) const char* GetSetupDataFileName( ODSetupLocType lt,
				const char* fnm, bool acceptnone )
{
    return GetSetupShareFileName( fnm, lt, acceptnone );
}
