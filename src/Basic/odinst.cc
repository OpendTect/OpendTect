/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2012
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "odinst.h"
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
#define mRelRootDir GetSoftwareDir(1)

#ifdef __win__
#include <Windows.h>
#include <direct.h>
#include "winutils.h"
static BufferString getInstDir()
{
    BufferString dirnm( _getcwd(NULL,0) );
    char* termchar = 0;
    termchar = dirnm.find( "\\bin\\win" );
    if ( !termchar )
	termchar = dirnm.find( "\\bin\\Win" );

    if ( termchar )
	*termchar = '\0';
    return dirnm;
}
#undef mRelRootDir
# define mRelRootDir getInstDir()
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


#define mMkMachComm( prog, reldir ) \
    OS::MachineCommand machcomm( prog ); \
    if ( __ismac__ ) \
    { \
	const FilePath instdir( reldir ); \
	machcomm.addKeyedArg( "instdir", reldir ); \
    } \
    else \
	machcomm.addKeyedArg( "instdir", reldir );

#define mGetFullMachComm(errretstmt) \
    FilePath installerfp( getInstallerPlfDir() ); \
    if ( !File::isDirectory(installerfp.fullPath()) ) \
	errretstmt; \
    if ( __iswin__ ) \
	installerfp.add( "od_instmgr.exe" ); \
    else if( __ismac__ ) \
	installerfp.add( "od_instmgr" ); \
    else if ( __islinux__ ) \
	installerfp.add( "run_installer" ); \
    BufferString prog( installerfp.fullPath() ); \
    if ( !File::isExecutable(prog) ) \
	errretstmt; \
    mMkMachComm( prog, mRelRootDir )


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
    BufferString appldir( GetSoftwareDir(0) );
    if ( File::isLink(appldir) )
	appldir = File::linkTarget( appldir );

#ifdef __mac__
    FilePath macpath( appldir );
    appldir = macpath.pathOnly();
#endif

    FilePath installerdir( appldir );
    installerdir.setFileName( mInstallerDirNm );
    return installerdir.fullPath();
}


void ODInst::startInstManagement()
{
    mGetFullMachComm(return);
    const BufferString curpath = File::getCurrentPath();
    File::changeDir( installerfp.pathOnly() );
    machcomm.execute( OS::RunInBG );
    File::changeDir( curpath.buf() );
}


void ODInst::startInstManagementWithRelDir( const char* reldir )
{
#ifdef __win__
    FilePath installerfp( getInstallerPlfDir() );
    if ( installerfp.isEmpty() )
	return;
    installerfp.add( "od_instmgr" );
    mMkMachComm( installerfp.fullPath(), reldir );
    machcomm.execute( OS::RunInBG );
#endif
}


BufferString ODInst::getInstallerPlfDir()
{
    FilePath installerbasedir( GetInstallerDir() );
    if ( !File::isDirectory(installerbasedir.fullPath()) )
	return "";
#ifdef __mac__
    FilePath installerfp( installerbasedir, "Contents/MacOS" );
#else
    FilePath installerfp( installerbasedir, "bin", __plfsubdir__, "Release" );
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
    mGetFullMachComm(return false);
    machcomm.addFlag( "updcheck_report" );
    BufferString stdout;
    if ( !machcomm.execute(stdout) || stdout.isEmpty() )
	return false;

    return stdout == sKeyHasUpdate();
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
