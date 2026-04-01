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
#include "oddirs.h"
#include "odplatform.h"
#include "odver.h"
#include "od_iostream.h"
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
# include "unistd.h"
# ifndef OD_NO_QT
#  include <QProcess>
# endif
#endif

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


ODInst::RelType ODInst::getRelType()
{
    FilePath relinfofp( GetRelInfoDir(), "README.txt" );
    const BufferString reltxtfnm( relinfofp.fullPath() );
    od_istream strm( reltxtfnm );
    if ( !strm.isOK() )
	return ODInst::OtherRelease;

    BufferString appnm, relstr;
    strm.getWord( appnm, false );
    strm.getWord( relstr, false );
    const int relsz = relstr.size();
    if ( appnm[0] != '[' || relsz < 4 || relstr[0] != '('
	|| relstr[relsz-1] != ']' || relstr[relsz-2] != ')' )
	return ODInst::OtherRelease;

    relstr[relsz-2] = '\0';
    return ODInst::parseEnumRelType( relstr.buf()+1 );
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
    return "dGB_Earth_Sciences";
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


static bool isUserInst()
{
    const FilePath reqpath( GetSoftwareDir(true) );
    if ( reqpath.isEmpty() )
	return true;

    const FilePath homepath( File::getHomePath() );
    if ( reqpath.isSubDirOf(homepath) || reqpath == homepath )
	return true;

    const BufferString reqpathstr = reqpath.fullPath();
#ifdef __win__
    if ( WinUtils::pathContainsTrustedInstaller(reqpathstr.buf()) )
	return false;
#endif
    if ( __ismac__ && reqpathstr.startsWith("/Application") )
	return false;

    return File::isWritable( reqpathstr );
}


static const char* getInstallerExe( QSettings::Scope scope, ActionType typ )
{
    mDeclStaticString(ret);
    ret.setEmpty();
    const QSettings instsetts( scope, qtEditorName(), sKeyqInstaller() );
    const QVariant qvarinstexec = instsetts.value( sKeyqInstallerExecFnm() );
    const QString qinstexec = qvarinstexec.toString();
    BufferString instexec( qinstexec );
    if ( __iswin__ && typ == ActionType::UpdateCheck )
    { // od_instmgr may have embedded UAC, needs to use run_installer.exe
	FilePath instscriptfp( instexec );
	instscriptfp.setFileName( "run_installer.exe" );
	const BufferString instscript = instscriptfp.fullPath();
	if ( File::isExecutable(instscript.buf()) &&
	     !File::isDirectory(instscript.buf()) )
	    instexec = instscript;
    }
    else if ( __islinux__ )
    {
	FilePath instscriptfp( instexec );
	instscriptfp.setFileName( "run_installer" );
	const BufferString instscript = instscriptfp.fullPath();
	if ( File::isExecutable(instscript.buf()) &&
	     !File::isDirectory(instscript.buf()) )
	    instexec = instscript;
    }

    if ( File::isExecutable(instexec.buf()) &&
	 !File::isDirectory(instexec.buf()) )
	ret = File::getCanonicalPath( instexec.str() );

    return ret.buf();
}


static const char* getInstallerVersion( const char* execfnm )
{
    mDeclStaticString(ret);
    if ( !StringView(execfnm).isEmpty() )
    {
	FilePath fp( execfnm );
	fp.setFileName( nullptr );
	const BufferString mask( "ver.*_", GetPlfSubDir(), ".txt" );
	while ( !fp.isEmpty() )
	{
	    fp.setFileName( nullptr );
	    if ( fp.isEmpty() )
		break;

	    if ( __ismac__ && fp.fileName() == "Contents" )
		fp.setFileName( "Resources" );

	    fp.setFileName( "relinfo" );
	    if ( !fp.exists() )
		continue;

	    const DirList dl( fp.fullPath(), File::DirListType::FilesInDir,
			      mask.str() );
	    if ( !dl.isEmpty() )
	    {
		ret.setEmpty();
		File::getContent( dl.fullPath(0), ret );
	    }

	    break;
	}
    }

    return ret.buf();

}


static const char* getUserInstallerExe( ActionType typ )
{
    mDeclStaticString(ret);
    if ( ret.isEmpty() )
	ret = getInstallerExe( QSettings::Scope::UserScope, typ );

    return ret.buf();
}


static const char* getSystemInstallerExe( ActionType typ )
{
    mDeclStaticString(ret);
    if ( ret.isEmpty() )
	ret = getInstallerExe( QSettings::Scope::SystemScope, typ );

    return ret.buf();
}


static BufferString getLocalInstallerDir()
{
    mDeclStaticString(ret);
    BufferString appldir( softwareDir() );
    if ( File::isSymLink(appldir) )
	appldir = File::linkEnd( appldir );

    FilePath installerdir( appldir );
    installerdir.setFileName( __ismac__ ? "OpendTect Installer.app"
					: "Installer" );
    FilePath relinfosubdir( installerdir );
    if ( __ismac__ )
	relinfosubdir.add( "Contents" ).add( "Resources" );

    relinfosubdir.add( "relinfo" );
    if ( !relinfosubdir.exists() )
	return nullptr;

    bool isvalid = false;
    FilePath moddepfp( installerdir );
    if ( __ismac__ )
	moddepfp.add( "Contents" ).add( "Resources" );

    moddepfp.add( "share" ).add( "ModDeps.Installer" );
    if ( moddepfp.exists() )
    {
	isvalid = true;
    }
    else
    {
	const DirList dl( relinfosubdir.fullPath(),
			  File::DirListType::FilesInDir );
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    if ( dl.get(idx).contains("instmgr") )
	    {
		isvalid = true;
		break;
	    }
	}
    }

    if ( isvalid )
    {
	const BufferString instdir = installerdir.fullPath();
	ret = File::getCanonicalPath( instdir.str() );
    }

    return ret.buf();
}


static const char* getLocalInstallerExe( const char* odinstdir,
					 ActionType /*typ*/ )
{
    mDeclStaticString(ret);
    const BufferString installerbasedir( getLocalInstallerDir() );
    if ( !File::isDirectory(installerbasedir.buf()) )
	return nullptr;

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

    for ( const auto* execdir : installerbinfps )
    {
	FilePath execfp( *execdir );
	if ( __iswin__ )
	{
	    execfp.add( "od_instmgr.exe" );
	    if ( !execfp.exists() )
		execfp.add( "od_instmgrd.exe" );
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
	    ret = File::getCanonicalPath( instexec.str() );
	    break;
	}
    }

    return ret.buf();
}


static const char* getInstallerExe( ActionType typ=ActionType::Standard )
{
    mDeclStaticString(ret);
    const bool isuserinst = isUserInst();
    BufferString instexec = isuserinst ? getUserInstallerExe( typ )
				       : getSystemInstallerExe( typ );
    if ( instexec.isEmpty() )
	instexec = getLocalInstallerExe( GetSoftwareDir(true), typ );

    if ( !instexec.isEmpty() )
    {
	getInstallerVersion( instexec.str() );
	ret = instexec.str();
    }

    return ret.buf();
}


static OS::MachineCommand getFullMachComm( const char* odinstdir,
					   ActionType typ )
{
    const BufferString installerexe( getInstallerExe(typ) );
    OS::MachineCommand mc;
    if ( installerexe.isEmpty() )
	return mc;

    mc.setProgram( installerexe )
      .addKeyedArg( "instdir", odinstdir );

    const SeparString fms( getInstallerVersion(nullptr), '.' );
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


static bool submitCommand( const OS::MachineCommand& mc, const char* odinstdir )
{
    OS::CommandExecPars pars( OS::RunInBG );
    if ( __iswin__ )
    {
	pars.workingdir( FilePath(mc.program()).pathOnly() );
	pars.runasadmin( !canInstall(odinstdir) ||
			 !canInstall(pars.workingdir_) );
    }

    OS::MachineCommand isolatedmc( mc, __islinux__ );
    return isolatedmc.execute( pars );
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

    submitCommand( mc, softwareDir() );
}


void ODInst::startUpdateCheck( CallBack cb )
{
    const OS::MachineCommand mc = getFullMachComm( softwareDir(),
						   ActionType::UpdateCheck );
    if ( mc.isBad() )
	return;

    OS::MachineCommand isolatedmc( mc, __islinux__ );
    BufferString workdir;
    if ( __iswin__ )
    {
	const FilePath execfp( mc.program() );
	workdir = execfp.pathOnly();
    }

    auto& mgr = Threads::CommandLaunchMgr::getMgr();
    const bool readstdoutput = true;
    const bool readstderror = true;
    const bool inpythonenv = false;
    mgr.execute( isolatedmc, readstdoutput, readstderror, &cb, inpythonenv,
		 workdir.buf() );
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
    return res.isEmpty() ? ODInst::InformOnly : parseEnumAutoInstType( res );
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

BufferString ODInst::GetInstallerDir()
{
    if ( OD::InInstallerRunContext() )
	return softwareDir();

    const bool isuserinst = isUserInst();
    const QSettings::Scope instscope = isuserinst ? QSettings::Scope::UserScope
						: QSettings::Scope::SystemScope;
    const QSettings instsetts( instscope, qtEditorName(), sKeyqInstaller() );
    const QVariant qvarinstsettlist = instsetts.value( sKeyqInstallerDir() );
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
	submitCommand( mc, reldir );
}
