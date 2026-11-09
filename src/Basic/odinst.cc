/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odinst.h"

#include "bufstringset.h"
#include "commandlaunchmgr.h"
#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "oddirs.h"
#include "odplatform.h"
#include "odver.h"
#include "oscommand.h"
#include "odruncontext.h"
#include "perthreadrepos.h"
#include "separstr.h"
#include "settings.h"

#include <QSettings>

#ifdef __win__
# include <Windows.h>
# include <direct.h>
# include "winutils.h"
#else
# ifndef OD_NO_QT
#  include <QProcess>
# endif
#endif


const char* ODInst::sKeyODExecNm( bool addexe )
{
    mDeclStaticString(ret);
    ret = GetODApplicationName( "od_main" );
    if ( addexe )
	ret += ".exe";

    return ret.str();
}


const char* ODInst::sKeyODBatchHostsExecNm( bool addexe )
{
    mDeclStaticString(ret);
    ret = GetODApplicationName( "od_BatchHosts" );
    if ( addexe )
	ret += ".exe";

    return ret.str();
}


const char* ODInst::sKeyODLicInstallExecNm( bool addexe )
{
    mDeclStaticString(ret);
    ret = GetODApplicationName( "od_LicInstall" );
    if ( addexe )
	ret += ".exe";

    return ret.str();
}


const char* ODInst::sKeyODRemSerMgrExecNm( bool addexe )
{
    mDeclStaticString( ret );
    ret = GetODApplicationName( "od_RemoteServiceMgr" );
    if ( addexe )
	ret += ".exe";

    return ret.str();
}


const char* ODInst::sKeyODFirewallExecNm()
{
    mDeclStaticString( ret );
    ret = GetODApplicationName( "od_FirewallProcSetter.exe" );
    return ret.str();
}


const char* ODInst::sKeyODProgressViewerExecNm()
{
    mDeclStaticString( ret );
    ret = GetODApplicationName( "od_ProgressViewer" );
    return ret.str();
}


mDefineNameSpaceEnumUtils(ODInst,AutoInstType,"Auto update")
{ "Manager", "Inform", "Full", "None", nullptr };


mDefineNameSpaceEnumUtils(ODInst,RelType,"Release type")
{
	"Stable",
	"Development",
	"Pre-Release Stable",
	"Pre-Release Development",
	"Old Version",
	"Other",
	nullptr
};


BufferString ODInst::GetRelInfoDir()
{
    FilePath ret( GetSoftwareDir(true) );
    if ( __ismac__ )
	ret.add( "Resources" );

    ret.add( "relinfo" );
    return ret.fullPath();
}


const char* sAutoInstTypeUserMsgs[] = {
    "[&Manager] Start the Installation Manager when updates are available",
    "[&Inform] When new updates are present, show this in OpendTect title bar",
    "[&Auto] Automatically download and install new updates "
	"(requires sufficient administrator rights)",
    "[&None] Never check for updates", 0 };


const BufferStringSet& ODInst::autoInstTypeUserMsgs()
{
    mDefineStaticLocalObject( BufferStringSet, ret, (sAutoInstTypeUserMsgs) );
    return ret;
}

const char* ODInst::sKeyAutoInst() { return ODInst::AutoInstTypeDef().name(); }


bool ODInst::canInstall( const char* dirnm )
{
    return File::isWritable( dirnm );
}


namespace ODInst {

static const char* softwareDir()
{
    mDeclStaticString(ret);
    if ( ret.isEmpty() )
    {
	FilePath instfp( GetSoftwareDir(true) );
	if ( __ismac__ )
	    ret  = instfp.pathOnly();
	else
	    ret = instfp.fullPath();
    }

    return ret.buf();
}


static const char* qtEditorName()
{
    return __islinux__ ? "dGB_Earth_Sciences" : "dGB Earth Sciences";
}


static const char* sKeyqInstaller()
{
    return "Installer";
}


static const char* sKeyqInstallerDir()
{
    return "ODInstallerDir";
}


static const char* sKeyqInstallerExecFnm()
{
    return "ODInstallerExecFnm";
}


static const char* sKeyqInstallerVersion()
{
    return "ODInstallerVersion";
}


static BufferString qSettingsString( const QSettings& setts, const char* key )
{
    const QVariant qvar = setts.value( key );
    if ( qvar.isNull() )
	return BufferString::empty();

    const QString qstr = qvar.toString();
    return BufferString( qstr );
}


#ifdef __win__
static bool windowsPathIsInFolder( const FilePath& reqpathfp, int csidl )
{
    BufferString dir = GetSpecialFolderLocation( csidl );
    if ( dir.isEmpty() )
	return false;

    dir = FilePath::getLongPath( dir.buf() );
    const FilePath trustedfp( dir.buf() );
    return reqpathfp == trustedfp || reqpathfp.isSubDirOf( trustedfp );
}


static bool isTrustedWindowsLocation( const FilePath& reqpathfp )
{
    return windowsPathIsInFolder( reqpathfp, CSIDL_PROGRAM_FILES ) ||
	   windowsPathIsInFolder( reqpathfp, CSIDL_PROGRAM_FILESX86 ) ||
	   windowsPathIsInFolder( reqpathfp, CSIDL_COMMON_APPDATA ) ||
	   windowsPathIsInFolder( reqpathfp, CSIDL_WINDOWS );
}


static BufferString nearestExistingWindowsDir( const FilePath& reqpathfp )
{
    const BufferString fullpath = reqpathfp.fullPath();
    if ( File::exists(fullpath.buf()) )
	return fullpath;

    for ( int idx=reqpathfp.nrLevels()-2; idx>=0; idx-- )
    {
	const BufferString dirpath = reqpathfp.dirUpTo( idx );
	if ( File::exists(dirpath.buf()) )
	    return dirpath;
    }

    return BufferString();
}
#endif


static bool isUserInst()
{
    BufferString reqpathfnm( GetSoftwareDir(true) );
    if ( reqpathfnm.isEmpty() )
	return true;

    BufferString homepathstr = File::getHomePath();
    if ( __iswin__ )
    {
	reqpathfnm = FilePath::getLongPath( reqpathfnm.buf() );
	homepathstr = FilePath::getLongPath( homepathstr.buf() );
    }

    const FilePath reqpathfp( reqpathfnm.buf() );
    const FilePath homepath( homepathstr.buf() );
    if ( reqpathfp.isSubDirOf(homepath) || reqpathfp == homepath )
	return true;

    if ( __islinux__ )
	return false; // Any path outside of home is Network for Linux

    if ( __ismac__ )
	return !reqpathfnm.startsWith("/Application");

#ifdef __win__
    if ( isTrustedWindowsLocation(reqpathfp) )
	return false;

    const BufferString fullpath = reqpathfp.fullPath();
    if ( !File::exists(fullpath.buf()) && WinUtils::IsUserAnAdmin() )
	return false;

    const BufferString existingdir = nearestExistingWindowsDir( reqpathfp );
    if ( !existingdir.isEmpty() &&
	 !WinUtils::belongsToStdUser(existingdir.buf()) )
	return false;

    return true;
#endif

    return false;
}


static BufferString getInstallerExe( QSettings::Scope scope, ActionType typ,
				     BufferString* verstr )
{
    QSettings instsetts( scope, qtEditorName(), sKeyqInstaller() );
    instsetts.setFallbacksEnabled( false );
    if ( instsetts.status() != QSettings::Status::NoError )
	return BufferString::empty();

    BufferString instver;
    QVariant qvarinstexec = instsetts.value( sKeyqInstallerExecFnm() );
    if ( __islinux__ && scope == QSettings::Scope::SystemScope &&
	qvarinstexec.isNull() )
    {
	QSettings instsettsnet( scope, qtEditorName(), "NetworkInstaller" );
	instsettsnet.setFallbacksEnabled( false );
	qvarinstexec = instsettsnet.value( sKeyqInstallerExecFnm() );
	instver = qSettingsString( instsettsnet, sKeyqInstallerVersion() );
    }
    else
	instver = qSettingsString( instsetts, sKeyqInstallerVersion() );

    if ( qvarinstexec.isNull() )
	return BufferString::empty();

    const QString qinstexec = qvarinstexec.toString();
    BufferString ret( qinstexec );
    if ( __iswin__ && scope == QSettings::Scope::SystemScope &&
	 typ != ActionType::UpdateCheck )
    { // od_instmgr may have embedded UAC,
      // run_installer.exe is stored under qvarinstexec
	FilePath instscriptfp( ret );
	instscriptfp.setFileName( "od_instmgr.exe" );
	BufferString instscript = instscriptfp.fullPath();
	if ( File::isExecutable(instscript.buf()) &&
	     !File::isDirectory(instscript.buf()) )
	     ret = instscript;
	else
	{
	    instscriptfp.setFileName( "od_instmgrd.exe" );
	    if ( File::isExecutable(instscript.buf()) &&
		 !File::isDirectory(instscript.buf()) )
		 ret = instscript;
	}
    }

    if ( File::isExecutable(ret.buf()) &&
	 !File::isDirectory(ret.buf()) )
    {
	if ( verstr )
	    *verstr = instver;

	const FilePath canonfp( File::getCanonicalPath( ret.str() ) );
	return canonfp.fullPath();
    }

    return BufferString::empty();
}


static BufferString getUserInstallerExe( ActionType typ,
					 BufferString* verstr )
{
    // No cache, always read from the current settings
    return getInstallerExe( QSettings::Scope::UserScope, typ, verstr );
}


static BufferString getSystemInstallerExe( ActionType typ,
					  BufferString* verstr )
{
    // No cache, always read from the current settings
    return getInstallerExe( QSettings::Scope::SystemScope, typ, verstr );
}


static FilePath installerRelInfoDir( const FilePath& installerdir )
{
    FilePath relinfodir( installerdir );
    if ( __ismac__ )
	relinfodir.add( "Contents" ).add( "Resources" );

    relinfodir.add( "relinfo" );
    return relinfodir;
}


static bool isValidInstallerDir( const FilePath& installerdir )
{
    const FilePath relinfosubdir = installerRelInfoDir( installerdir );
    if ( !relinfosubdir.exists() )
	return false;

    FilePath moddepfp( installerdir );
    if ( __ismac__ )
	moddepfp.add( "Contents" ).add( "Resources" );

    moddepfp.add( "share" ).add( "ModDeps.Installer" );
    if ( moddepfp.exists() )
	return true;

    const DirList dl( relinfosubdir.fullPath(),
		      File::DirListType::FilesInDir );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	if ( dl.get(idx).contains("instmgr") )
	    return true;
    }

    return false;
}


static void readLocalInstallerVersion( const FilePath& installerdir,
				       BufferString& verstr )
{
    FilePath verfp = installerRelInfoDir( installerdir );
    const BufferString plfverfnm( "ver.instmgr_", GetPlfSubDir(), ".txt" );
    verfp.add( plfverfnm.str() );
    if ( !verfp.exists() )
	verfp.setFileName( "ver.instmgr.txt" );

    verstr.setEmpty();
    if ( verfp.exists() )
	File::getContent( verfp.fullPath(), verstr );

    verstr.trimBlanks();
}


static BufferString getLocalInstallerDir( BufferString* verstr=nullptr )
{
    BufferString appldir( softwareDir() );
    if ( File::isSymLink(appldir) )
	appldir = File::linkEnd( appldir );

    const char* instdirnm = __ismac__ ? "OpendTect Installer.app"
				       : "Installer";
    // The installer is installed at the installation root. Versioned layouts
    // are <root>/<version>/<product>, so that root can be above the
    // application directory.
    FilePath basedir( appldir );
    while ( basedir.nrLevels() > 0 )
    {
	FilePath installerdir( basedir );
	installerdir.setFileName( instdirnm );
	if ( isValidInstallerDir(installerdir) )
	{
	    if ( verstr )
		readLocalInstallerVersion( installerdir, *verstr );

	    const BufferString instdir = installerdir.fullPath();
	    const FilePath canonfp( File::getCanonicalPath(instdir.str()) );
	    return canonfp.fullPath();
	}

	basedir.setFileName( nullptr );
    }

    return BufferString::empty();
}


static BufferString getLocalInstallerExe( ActionType typ,
					  BufferString* verstr=nullptr )
{
    const BufferString installerbasedir( getLocalInstallerDir(verstr) );
    if ( !File::isDirectory(installerbasedir.buf()) )
	return BufferString::empty();

    ManagedObjectSet<FilePath> installerbinfps;
    FilePath installerfp( installerbasedir.str() );
    if ( __ismac__ )
    {
	installerfp.add( "Contents" ).add( "MacOS" );
	installerbinfps.add( new FilePath(installerfp) );
	installerfp.add( "Debug" );
	installerbinfps.add( new FilePath(installerfp) );
    }
    else
    {
	installerfp.add( "bin" );
	installerbinfps.add( new FilePath(installerfp) );
	installerfp.add( GetPlfSubDir() ).add( "Release" );
	installerbinfps.add( new FilePath(installerfp) );
	installerfp.setFileName( "Debug" );
	installerbinfps.add( new FilePath(installerfp) );
    }

    BufferString ret;
    for ( const auto* execdir : installerbinfps )
    {
	FilePath execfp( *execdir );
	if ( __iswin__ )
	{
	    if ( typ == ActionType::UpdateCheck )
	    {
		execfp.add( "run_installer.exe" );
		if ( !execfp.exists() )
		{
		    execfp.setFileName( "run_installer.exe" );
		    if ( !execfp.exists() )
		    {
			execfp.setFileName( "od_instmgr.exe" );
			if ( !execfp.exists() )
			    execfp.setFileName( "od_instmgrd.exe" );
		    }
		}
	    }
	    else
	    {
		execfp.add( "od_instmgr.exe" );
		if ( !execfp.exists() )
		    execfp.add( "od_instmgrd.exe" );
	    }
	}
	else if ( __ismac__ )
	{
	    execfp.add( "od_instmgr" );
	    if ( !execfp.exists() )
		execfp.setFileName( "od_instmgrd" );
	}
	else
	{
	    execfp.add( "run_installer" );
	    if ( !execfp.exists() )
	    {
		execfp.setFileName( "od_instmgr" );
		if ( !execfp.exists() )
		    execfp.setFileName( "od_instmgrd" );
	    }
	}

	const BufferString instexec = execfp.fullPath();
	if ( File::isExecutable(instexec.buf()) &&
	     !File::isDirectory(instexec.buf()) )
	{
	    const FilePath canonfp( File::getCanonicalPath( instexec.str() ) );
	    ret = canonfp.fullPath();
	    break;
	}
    }

    if ( ret.isEmpty() && verstr )
	verstr->setEmpty();

    return ret;
}


static BufferString getInstallerExe( ActionType typ=ActionType::Standard,
				     BufferString* verstr=nullptr )
{
    const bool isuserinst = isUserInst();
    BufferString ret = isuserinst ? getUserInstallerExe( typ, verstr )
				  : getSystemInstallerExe( typ, verstr );
    if ( ret.isEmpty() )
	ret = getLocalInstallerExe( typ, verstr );

    return ret;
}


static OS::MachineCommand getFullMachComm( const char* odinstdir,
					   ActionType typ )
{
    BufferString odinstdirstr;
    if ( __iswin__ )
	odinstdirstr = FilePath::getLongPath( odinstdir );
    else
	odinstdirstr.set( odinstdir );

    odinstdir = odinstdirstr.buf();
    BufferString instver;
    const BufferString installerexe( getInstallerExe(typ,&instver) );
    OS::MachineCommand mc;
    if ( installerexe.isEmpty() )
	return mc;

    mc.setProgram( installerexe )
      .addKeyedArg( "instdir", odinstdir );

    const SeparString fms( instver.buf(), '.' );
    const bool ispre2026 = !fms.isEmpty() && fms.getUI16Value(0) > 1999 &&
			   fms.getUI16Value(0) < 2026;

    if ( ispre2026 )
    {
	if ( typ == ActionType::Manage || typ == ActionType::Update )
	    mc.addFlag( "update" ).addFlag( "skip-first-dlg" );
	else if ( typ == ActionType::UpdateCheck )
	    mc.addFlag( "updcheck_report" );

	return mc;
    }

    if ( typ == ActionType::Install )
	mc.addFlag( "install" );
    else if ( typ == ActionType::Manage )
	mc.addFlag( "manage" );
    else if ( typ == ActionType::Uninstall )
	mc.addFlag( "uninstall" );
    else if ( typ == ActionType::Update )
	mc.addFlag( "update" );
    else if ( typ == ActionType::UpdateCheck )
	mc.addFlag( "updcheck_report" );

    //TODO Use --sitesubdir

    return mc;
}


static bool submitCommand( const OS::MachineCommand& mc )
{
    OS::MachineCommand isolatedmc( mc, true );
    return isolatedmc.execute( OS::RunInBG );
}

} // namespace ODInst


const char* ODInst::sKeyHasUpdate()
{
    return "Updates available";
}


const char* ODInst::sKeyHasNoUpdate()
{
    return "No updates available";
}


bool ODInst::HasInstaller()
{
    const BufferString instexec = getInstallerExe();
    return !instexec.isEmpty();
}


void ODInst::startInstManagement( ActionType typ )
{
    const OS::MachineCommand mc = getFullMachComm( softwareDir(), typ );
    if ( mc.isBad() )
	return;

    submitCommand( mc );
}


void ODInst::startUpdateCheck( CallBack cb )
{
    const OS::MachineCommand mc = getFullMachComm( softwareDir(),
						   ActionType::UpdateCheck );
    if ( mc.isBad() )
	return;

    OS::MachineCommand isolatedmc( mc, true );
    auto& mgr = Threads::CommandLaunchMgr::getMgr();
    const bool readstdoutput = true;
    const bool readstderror = true;
    const bool inpythonenv = false;
    mgr.execute( isolatedmc, readstdoutput, readstderror, &cb, inpythonenv );
}


bool ODInst::runInstMgrForUpdt()
{
    return updatesAvailable();
}


namespace ODInst {

static Threads::Lock& Lock()
{
    static Threads::Lock lock( Threads::Lock::SmallWork );
    return lock;
}

} // namespace ODInst


bool ODInst::updatesAvailable( int isavailable )
{
    Threads::Locker lock( ODInst::Lock(),
			  isavailable<0 ? Threads::Locker::ReadLock
					: Threads::Locker::WriteLock );
    static int updavailable = -1;
    if ( isavailable > -1 )
	updavailable = isavailable;

    return updavailable == 1;
}


const char* ODInst::getPkgVersion( const char* file_pkg_basenm )
{
    mDeclStaticString( ret );
    const BufferString part1( "ver.", file_pkg_basenm );
    BufferString fnm = part1;
    fnm.add( "_" ).add( OD::Platform::local().shortName() );
    FilePath fp( GetRelInfoDir(), fnm );
    fp.setExtension( "txt", false );

    fnm = fp.fullPath();
    if ( !File::exists(fnm) )
    {
	fp.setFileName( part1 ); fp.setExtension( "txt", false );
	fnm = fp.fullPath();
	if ( !File::exists(fnm) )
	{
	    // Most probably you have built from source
	    ret.set( mODFullVersion ).add( " (development)" );
	    return ret.buf();
	}
    }

    File::getContent( fnm, ret );
    if ( ret.isEmpty() )
	ret = "[error: empty version file]";

    return ret.buf();
}


bool ODInst::autoInstTypeIsFixed()
{
    const BufferString envvarval = GetEnvVar( "OD_INSTALLER_POLICY" );
    return !envvarval.isEmpty();
}


ODInst::AutoInstType ODInst::getAutoInstType()
{
    const BufferString envvarval = GetEnvVar( "OD_INSTALLER_POLICY" );
    const BufferString res = envvarval.isEmpty()
			   ? userSettings().find( sKeyAutoInst() )
			   : envvarval;
    return res.isEmpty() ? ODInst::AutoInstType::InformOnly
			 : parseEnumAutoInstType( res );
}


void ODInst::setAutoInstType( ODInst::AutoInstType ait )
{
    userSettings().set( sKeyAutoInst(), ODInst::toString(ait) );
    userSettings().write();
}


Settings& ODInst::userSettings()
{
    return Settings::fetch( "instmgr" );
}


// Deprecated impls

const char* ODInst::sKeyODInstMgrExecNm( bool addexe )
{
    mDeclStaticString(ret);
    ret = GetODApplicationName( "od_instmgr" );
    if ( addexe )
	ret += ".exe";

    return ret.str();
}


BufferString ODInst::GetInstallerDir()
{
    if ( OD::InInstallerRunContext() )
	return softwareDir();

    const bool isuserinst = isUserInst();
    const QSettings::Scope instscope = isuserinst ? QSettings::Scope::UserScope
						: QSettings::Scope::SystemScope;
    QSettings instsetts( instscope, qtEditorName(), sKeyqInstaller() );
    instsetts.setFallbacksEnabled( false );
    QVariant qvarinstsettlist = instsetts.value( sKeyqInstallerDir() );
    if ( __islinux__ && instscope == QSettings::Scope::SystemScope &&
	qvarinstsettlist.isNull() )
    {
	QSettings instsettsnet( instscope, qtEditorName(), "NetworkInstaller" );
	instsettsnet.setFallbacksEnabled( false );
	qvarinstsettlist = instsettsnet.value( sKeyqInstallerDir() );
    }

    if ( qvarinstsettlist.isNull() )
	return BufferString::empty();

    const QString qinstsettlist = qvarinstsettlist.toString();
    const BufferString instsettlist( qinstsettlist );
    if ( File::isDirectory(instsettlist.buf()) )
	return instsettlist;

    const BufferString localinstdir = getLocalInstallerDir();
    if ( File::isDirectory(localinstdir.buf()) )
	return localinstdir;

    return BufferString::empty();
}


BufferString ODInst::getInstallerPlfDir()
{
    if ( OD::InInstallerRunContext() )
	return GetExecPlfDir();

    const FilePath instexefp( getInstallerExe() );
    if ( instexefp.exists() )
	return instexefp.pathOnly();

    return BufferString::empty();
}


void ODInst::getMachComm( const char* reldir, OS::MachineCommand& mc )
{
    mc = getFullMachComm( reldir, ActionType::UpdateCheck );
    const BufferString odverstr( GetFullODVersion() );
    if ( odverstr.contains("development") )
	mc = OS::MachineCommand();
}


void ODInst::startInstManagement()
{
    startInstManagement( ActionType::Standard );
}


void ODInst::startInstManagementWithRelDir( const char* reldir )
{
    const OS::MachineCommand mc = getFullMachComm( reldir,ActionType::Standard);
    if ( !mc.isBad() )
	submitCommand( mc );
}
