/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2012
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: odinst.cc,v 1.14 2012-07-04 12:00:28 cvsraman Exp $";

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
#include <direct.h>
static BufferString getInstDir()
{
    BufferString dirnm( _getcwd(NULL,0) );
    const int len = dirnm.size() - 10;
    if ( len > 0 )
	dirnm[len] = '\0';
    return dirnm;
}
#undef mRelRootDir
#define mRelRootDir getInstDir()
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
    FilePath installerdir( GetSoftwareDir(0) ); \
    installerdir.setFileName( "Installer" ); \
    if ( !File::isDirectory(installerdir.fullPath()) ) \
	return errretval; \
    BufferString cmd( __iswin__ ? "" : "@",\
    FilePath(GetBinPlfDir(),"od_instmgr").fullPath() ); \
    cmd.add( " --instdir " ).add( "\"" ).add( mRelRootDir ).add( "\"" );


void ODInst::startInstManagement()
{
    mDefCmd();
#ifndef __win__
    StreamProvider( cmd ).executeCommand( true, true );
#else
    ExecOSCmd( cmd, true, true );
#endif
}

bool ODInst::updatesAvailable()
{
    mDefCmd(false); cmd.add( " --updcheck_report" );
    return !StreamProvider( cmd ).executeCommand( false );
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
