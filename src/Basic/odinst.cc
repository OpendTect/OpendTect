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
#include "strmoper.h"
#include "strmprov.h"
#include "settings.h"
#include "bufstringset.h"

#define mDeclEnvVarVal const char* envvarval = GetEnvVar("OD_INSTALLER_POLICY")
#define mRelRootDir GetSoftwareDir(1)

#ifdef __win__
#include <Windows.h>
#include <direct.h>
#include <string.h>
#include "winutils.h"
static BufferString getInstDir()
{
    BufferString dirnm( _getcwd(NULL,0) );
    char* termchar = 0;
    termchar = strstr( dirnm.buf(), "\\bin\\win" ); 
    if ( !termchar )
	termchar = strstr( dirnm.buf(), "\\bin\\Win" );

    if ( termchar )
	*termchar = '\0';
    return dirnm;
}
#undef mRelRootDir
#define mRelRootDir getInstDir()
#else
#include "unistd.h"
#endif

DefineNameSpaceEnumNames(ODInst,AutoInstType,1,"Auto update")
{ "Manager", "Inform", "Full", "None", 0 };


DefineNameSpaceEnumNames(ODInst,RelType,0,"Release type")
{
	"Stable",
	"Development",
	"Pre-Release Stable",
	"Pre-Release Development",
	"Old Version",
	"Other",
	0
};


ODInst::RelType ODInst::getRelType()
{
    FilePath relinfofp( GetSoftwareDir(true), "relinfo", "README.txt" );
    const BufferString reltxtfnm( relinfofp.fullPath() );
    if ( !File::exists(reltxtfnm) )
	return ODInst::Other;
    StreamData sd( StreamProvider(reltxtfnm).makeIStream() );
    if ( !sd.usable() )
	return ODInst::Other;

    char appnm[64], relstr[64];
    appnm[0] = relstr[0] = '\0';
    StrmOper::wordFromLine(*sd.istrm,appnm,64);
    StrmOper::wordFromLine(*sd.istrm,relstr,64);
    sd.close();
    int relstrlen = strlen( relstr );
    if ( appnm[0] != '[' || relstrlen < 4 || relstr[0] != '('
	|| relstr[relstrlen-1] != ']' || relstr[relstrlen-2] != ')' )
	return ODInst::Other;

    relstr[relstrlen-2] = '\0';
    return ODInst::parseEnumRelType( relstr+1 );
}


const BufferStringSet& ODInst::autoInstTypeUserMsgs()
{
    static BufferStringSet* ret = 0;
    if ( !ret )
    {
	ret = new BufferStringSet;
	ret->add( "[&Manager] Start the Installation Manager "
		    "when updates are available" );
	ret->add( "[&Inform] When new updates are present, "
		    "show this in OpendTect's title bar" );
	ret->add( "[&Auto] Automatically download and install new updates "
		    "(requires sufficient administrator rights)" );
	ret->add( "[&None] Never check for updates" );
    };
    return *ret;
}
const char* ODInst::sKeyAutoInst() { return ODInst::AutoInstTypeDef().name(); }


bool ODInst::canInstall()
{
    return File::isWritable( mRelRootDir );
}


#define mDefCmd(errretval) \
    FilePath installerdir( GetInstallerDir() ); \
    if ( !File::isDirectory(installerdir.fullPath()) ) \
	return errretval; \
    installerdir.add( __iswin__ ? "od_instmgr" : "run_installer" ); \
    BufferString cmd( installerdir.fullPath() ); \
    cmd.add( " --instdir " ).add( "\"" ).add( mRelRootDir ).add( "\"" ); \
   

BufferString ODInst::GetInstallerDir()
{
    BufferString appldir( GetSoftwareDir(0) );
    if ( File::isLink(appldir) )
	appldir = File::linkTarget( appldir );

    FilePath installerdir( appldir );
    installerdir.setFileName( "Installer" );
    return installerdir.fullPath();
}


void ODInst::startInstManagement()
{
#ifndef __win__
    mDefCmd();
    chdir( installerdir.pathOnly() );
    StreamProvider( cmd ).executeCommand( true, true );
    chdir( GetSoftwareDir(0) );
#else
    FilePath installerdir( GetInstallerDir() ); 
    if ( !File::isDirectory(installerdir.fullPath()) )
	return;
    installerdir.add( "od_instmgr" );
    BufferString cmd( installerdir.fullPath() ); 
    BufferString parm( " --instdir "  );
    parm.add( "\"" ).add( mRelRootDir ).add( "\"" );
    
    executeWinProg( cmd, parm, installerdir.pathOnly() );
#endif
}


bool ODInst::runInstMgrForUpdt()
{
    mDefCmd(false); cmd.add( " --updcheck_report" );
    return ExecOSCmd( cmd, false, true );
}


bool ODInst::updatesAvailable()
{

    mDefCmd(false); cmd.add( " --updcheck_report" );
#ifndef __win__

    chdir( installerdir.pathOnly() );
    const int ret = system( cmd );
    chdir( GetSoftwareDir(0) );
    return ret == 1;
#else
    ExecOSCmd( cmd, false, true );
    FilePath tmp( File::getTempPath(), "od_updt" );
    bool ret = File::exists( tmp.fullPath() );
    if ( ret )
	File::remove( tmp.fullPath() );
    return ret;
#endif
}


const char* ODInst::getPkgVersion( const char* file_pkg_basenm )
{
    static BufferString ret;
    const BufferString part1( "ver.", file_pkg_basenm );
    BufferString fnm = part1;
    fnm.add( "_" ).add( OD::Platform::local().shortName() );
    FilePath fp( GetSoftwareDir(1), "relinfo", fnm );
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
			: userSettings().find( sKeyAutoInst() ).str();
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
