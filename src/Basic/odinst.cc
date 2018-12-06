/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2012
________________________________________________________________________

-*/


#include "odinst.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "odplatform.h"
#include "envvars.h"
#include "od_istream.h"
#include "oscommand.h"
#include "settings.h"
#include "staticstring.h"
#include "bufstringset.h"
#include "uistrings.h"

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

template<>
void EnumDefImpl<ODInst::AutoInstType>::init()
{
    uistrings_ += mEnumTr("Manager",0);
    uistrings_ += mEnumTr("Inform",0);
    uistrings_ += uiStrings::sFull();
    uistrings_ += uiStrings::sNone();
}


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

template<>
void EnumDefImpl<ODInst::RelType>::init()
{
    uistrings_ += mEnumTr("Stable",0);
    uistrings_ += mEnumTr("Development",0);
    uistrings_ += mEnumTr("Pre-Release Stable",0);
    uistrings_ += mEnumTr("Pre-Release Development",0);
    uistrings_ += mEnumTr("Old Version",0);
    uistrings_ += uiStrings::sOther();
}


BufferString ODInst::GetRelInfoDir()
{
#ifdef __mac__
    return File::Path( GetSoftwareDir(true), "Resources", "relinfo" ).fullPath();
#else
    return File::Path( GetSoftwareDir(true), "relinfo" ).fullPath();
#endif
}


ODInst::RelType ODInst::getRelType()
{
    File::Path relinfofp( GetRelInfoDir(), "README.txt" );
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
    return ODInst::RelTypeDef().parse( relstr.buf()+1 );
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
    machcomm.addKeyedArg( "instdir", reldir )

#define mGetFullMachComm(errretstmt) \
    File::Path installerfp( getInstallerPlfDir() ); \
    if ( !File::isDirectory(installerfp.fullPath()) ) \
	errretstmt; \
    if ( __iswin__ ) \
	installerfp.add( "od_instmgr.exe" ); \
    else if ( __islinux__ ) \
	installerfp.add( "run_installer" ); \
    BufferString prog( installerfp.fullPath() ); \
    if ( !File::isExecutable(prog) ) \
        errretstmt; \
    mMkMachComm( prog, mRelRootDir )


BufferString ODInst::GetInstallerDir()
{
    BufferString appldir( GetSoftwareDir(0) );
    if ( File::isLink(appldir) )
	appldir = File::linkEnd( appldir );

    File::Path installerfp( appldir );
    installerfp.setFileName( mInstallerDirNm );
    return installerfp.fullPath();
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
    File::Path installerfp( getInstallerPlfDir() );
    if ( installerfp.isEmpty() )
	return;
    installerfp.add( "od_instmgr" );
    mMkMachComm( installerfp.fullPath(), reldir );
    machcomm.execute( OS::RunInBG );
#endif
}


BufferString ODInst::getInstallerPlfDir()
{
    File::Path installerbasedir( GetInstallerDir() );
    if ( !File::isDirectory(installerbasedir.fullPath()) )
	return "";
    File::Path installerfp( installerbasedir, "bin", __plfsubdir__, "Release" );
    const BufferString path = installerfp.fullPath();
    if ( !File::exists(path) || !File::isDirectory(path) )
	return installerbasedir.fullPath();

    return installerfp.fullPath();
}


bool ODInst::updatesAvailable()
{
    mGetFullMachComm(return false);
    machcomm.addFlag( "updcheck_report" );
    return machcomm.execute( OS::Wait4Finish );
}


const char* ODInst::getPkgVersion( const char* file_pkg_basenm )
{
    mDeclStaticString( ret );
    const BufferString part1( "ver.", file_pkg_basenm );
    BufferString fnm = part1;
    fnm.add( "_" ).add( OD::Platform::local().shortName() );
    File::Path fp( GetRelInfoDir(), fnm );
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
    return res && *res ? AutoInstTypeDef().parse( res ) : ODInst::InformOnly;
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
