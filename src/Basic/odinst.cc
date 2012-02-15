/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: odinst.cc,v 1.2 2012-02-15 16:22:05 cvsbert Exp $";

#include "odinst.h"
#include "file.h"
#include "oddirs.h"
#include "strmprov.h"
#include "settings.h"
#include "bufstringset.h"

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
    userSettings().set( sKeyAutoInst(), ODInst::toString(ait) );
    userSettings().write();
}


Settings& ODInst::userSettings()
{
    return Settings::fetch( "instmgr" );
}
