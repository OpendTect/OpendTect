/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2012
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "odinst.h"

#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "odplatform.h"
#include "envvars.h"
#include "od_istream.h"
#include "oscommand.h"
#include "settings.h"
#include "perthreadrepos.h"
#include "bufstringset.h"

#define mDeclEnvVarVal const char* envvarval = GetEnvVar("OD_INSTALLER_POLICY")
#ifdef __mac__
    #define mRelRootDir \
    FilePath( GetSoftwareDir(true) ).pathOnly()
#else
	#define mRelRootDir GetSoftwareDir(true)
#endif

#ifdef __win__
#include <Windows.h>
#include <direct.h>
#include "winutils.h"
#else
# include "unistd.h"
# ifndef OD_NO_QT
#  include <QProcess>
# endif
#endif

mDefineNameSpaceEnumUtils(ODInst,AutoInstType,"Auto update")
{ "Manager", "Inform", "Full", "None", 0 };


mDefineNameSpaceEnumUtils(ODInst,RelType,"Release type")
{
	"Stable",
	"Development",
	"Pre-Release Stable",
	"Pre-Release Development",
	"Old Version",
	"Other",
	0
};


BufferString ODInst::GetRelInfoDir()
{
#ifdef __mac__
    return FilePath( GetSoftwareDir(true), "Resources", "relinfo" ).fullPath();
#else
    return FilePath( GetSoftwareDir(true), "relinfo" ).fullPath();
#endif
}


ODInst::RelType ODInst::getRelType()
{
    FilePath relinfofp( GetRelInfoDir(), "README.txt" );
    const BufferString reltxtfnm( relinfofp.fullPath() );
    od_istream strm( reltxtfnm );
    if ( !strm.isOK() )
	return ODInst::Other;

    BufferString appnm, relstr;
    strm.getWord( appnm, false );
    strm.getWord( relstr, false );
    const int relsz = relstr.size();
    if ( appnm[0] != '[' || relsz < 4 || relstr[0] != '('
	|| relstr[relsz-1] != ']' || relstr[relsz-2] != ')' )
	return ODInst::Other;

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


bool ODInst::canInstall()
{
    return File::isWritable( mRelRootDir );
}


namespace ODInst {

static OS::MachineCommand makeMachComm( const char* prog, const char* reldir )
{
    OS::MachineCommand mc( prog );
    mc.addKeyedArg( "instdir", reldir );
    return mc;
}

static OS::MachineCommand getFullMachComm( const char* reldir )
{
    OS::MachineCommand mc;
    FilePath installerfp( getInstallerPlfDir() );
    if ( !File::isDirectory(installerfp.fullPath()) )
	return mc;
    if ( __iswin__ )
	installerfp.add( "od_instmgr.exe" );
    else if( __ismac__ )
	installerfp.add( "od_instmgr" );
    else if ( __islinux__ )
    {
	installerfp.add( "run_installer" );
	if ( !installerfp.exists() )
	{
	    FilePath odinstmgrfp( installerfp );
	    odinstmgrfp.setFileName( "od_instmgr" );
	    if ( odinstmgrfp.exists() )
		installerfp = odinstmgrfp;
	}
    }

    return makeMachComm( installerfp.fullPath(), reldir );
}

static bool submitCommand( OS::MachineCommand& mc, const char* reldir )
{
    OS::CommandExecPars pars( OS::RunInBG );
    pars.workingdir( FilePath(mc.program()).pathOnly() );
    pars.runasadmin( !File::isWritable(reldir) );
    return mc.execute( pars );
}

};

#define mGetFullMachComm(reldir,errretstmt) \
    OS::MachineCommand machcomm = getFullMachComm( reldir ); \
    if ( machcomm.isBad() || !File::isExecutable(machcomm.program()) ) \
        errretstmt


const char* ODInst::sKeyHasUpdate()
{
    return "Updates available";
}


const char* ODInst::sKeyHasNoUpdate()
{
    return "No updates available";
}

BufferString ODInst::GetInstallerDir()
{
    BufferString appldir( GetSoftwareDir(false) );
    if ( File::isLink(appldir) )
	appldir = File::linkTarget( appldir );

#ifdef __mac__
    FilePath macpath( appldir );
    appldir = macpath.pathOnly();
#endif

    FilePath installerdir( appldir );
    installerdir.setFileName( mInstallerDirNm );
    if ( !File::isDirectory(installerdir.fullPath()) )
	installerdir = appldir;

    FilePath relinfosubdir( installerdir );
#ifdef __mac__
    relinfosubdir.add( "Contents" ).add( "Resources" );
#endif
    relinfosubdir.add( "relinfo" );
    if ( !relinfosubdir.exists() )
	return BufferString::empty();

    const DirList dl( relinfosubdir.fullPath(), DirList::FilesOnly );
    bool isvalid = false;
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	if ( dl.get(idx).contains("instmgr") )
	{
	    isvalid = true;
	    break;
	}
    }

    return isvalid ? installerdir.fullPath() : BufferString::empty();
}


void ODInst::startInstManagement()
{
    mGetFullMachComm(mRelRootDir,return);
    submitCommand( machcomm, mRelRootDir );
}


void ODInst::startInstManagementWithRelDir( const char* reldir )
{
    mGetFullMachComm(reldir,return);
    submitCommand( machcomm, reldir );
}


BufferString ODInst::getInstallerPlfDir()
{
    const FilePath installerbasedir( GetInstallerDir() );
    if ( !installerbasedir.exists() )
	return BufferString::empty();

#ifdef __mac__
    FilePath installerfp( installerbasedir, "Contents/MacOS" );
#else
    FilePath installerfp( installerbasedir, "bin", __plfsubdir__, "Release" );
    if ( !installerfp.exists() )
    {
	FilePath dbginstallerfp( installerfp );
	dbginstallerfp.setFileName( "Debug" );
	if ( dbginstallerfp.exists() )
	    installerfp = dbginstallerfp;
    }

#endif
    const BufferString path = installerfp.fullPath();
    if ( !File::exists(path) || !File::isDirectory(path) )
	return installerbasedir.fullPath();

    return installerfp.fullPath();
}


bool ODInst::runInstMgrForUpdt()
{
    return updatesAvailable();
}


bool ODInst::updatesAvailable()
{
    mGetFullMachComm(mRelRootDir,return false);
    machcomm.addFlag( "updcheck_report" );
    BufferString stdoutstr;
    if ( !machcomm.execute(stdoutstr) || stdoutstr.isEmpty() )
	return false;

    return stdoutstr == sKeyHasUpdate();
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
	    { ret = "[error: version file not found]"; return ret.buf(); }
    }

    File::getContent( fnm, ret );
    if ( ret.isEmpty() )
	ret = "[error: empty version file]";
    return ret.buf();
}


bool ODInst::autoInstTypeIsFixed()
{
    mDeclEnvVarVal;
    return envvarval && *envvarval;
}


ODInst::AutoInstType ODInst::getAutoInstType()
{
    mDeclEnvVarVal;
    const char* res = envvarval && *envvarval ? envvarval
			: userSettings().find( sKeyAutoInst() );
    return res && *res ? parseEnumAutoInstType( res ) : ODInst::InformOnly;
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
