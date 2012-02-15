/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: odinst.cc,v 1.1 2012-02-15 13:44:07 cvsbert Exp $";

#include "odinst.h"
#include "file.h"
#include "oddirs.h"
#include "strmprov.h"
#include "settings.h"

DefineNameSpaceEnumNames(ODInst,AutoInstType,1,"Auto update")
{ "Manager", "Inform", "Full", "None", 0 };
const char** ODInst::autoInstTypeUserMsgs()
{
    static const char* ret[] =
    {
	"[&Manager] Start the Installation Manager when updates are available",
	"[&Inform] When new updates are present, "
		    "show this in OpendTect's title bar",
	"[&Auto] Automatically download and install new updates "
		    "(requires sufficient administrator rights)",
	"[&None] Never check for updates",
	0
    };
    return ret;
}
const char* ODInst::sKeyAutoInst() { return ODInst::AutoInstTypeDef().name(); }

#define mRelRootDir GetSoftwareDir(1)


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


ODInst::AutoInstType ODInst::getAutoInstType()
{
    const char* res = userSettings().find( sKeyAutoInst() );
    return res && *res ? parseEnumAutoInstType( res ) : ODInst::UseManager;
}


void ODInst::setAutoInstType( ODInst::AutoInstType ait )
{
    userSettings().set( sKeyAutoInst(), ait );
    userSettings().write();
}


Settings& ODInst::userSettings()
{
    return Settings::fetch( "instmgr" );
}
