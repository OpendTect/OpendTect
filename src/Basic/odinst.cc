/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: odinst.cc,v 1.5 2012-03-13 08:14:55 cvsbert Exp $";

#include "odinst.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "odplatform.h"
#include "envvars.h"
#include "strmprov.h"
#include "settings.h"
#include "bufstringset.h"

#define mDeclEnvVarVal const char* envvarval = GetEnvVar("OD_INSTALLER_POLICY")
#define mRelRootDir GetSoftwareDir(1)


DefineNameSpaceEnumNames(ODInst,AutoInstType,1,"Auto update")
{ "Manager", "Inform", "Full", "None", 0 };
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


void ODInst::startInstManagement()
{
    const BufferString cmd( "@od_instmgr --instdir ", mRelRootDir );
    StreamProvider( cmd ).executeCommand( true );
}

bool ODInst::updatesAvailable()
{
    const BufferString cmd( "@od_instmgr --updcheck_report --instdir ",
	    			mRelRootDir );
    return StreamProvider( cmd ).executeCommand( false );
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
    return res && *res ? parseEnumAutoInstType( res ) : ODInst::UseManager;
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
