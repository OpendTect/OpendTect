/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Mar 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uivismenuitemhandler.cc,v 1.4 2011/10/07 21:50:44 cvsnanne Exp $";

#include "uivismenuitemhandler.h"

#include "uivispartserv.h"
#include "visdata.h"


uiVisMenuItemHandler::uiVisMenuItemHandler(const char* classnm,
	uiVisPartServer& vps, const char* nm, const CallBack& cb,
	const char* parenttext, int placement )
    : MenuItemHandler( *vps.getMenuHandler(), nm, cb, parenttext, placement )
    , visserv_( vps )
    , classnm_( classnm )
{}


int uiVisMenuItemHandler::getDisplayID() const
{ return menuhandler_.menuID(); }


bool uiVisMenuItemHandler::shouldAddMenu() const
{
    RefMan<visBase::DataObject> dataobj =
	visserv_.getObject( menuhandler_.menuID() );
    if ( !dataobj ) return false;

    return !strcmp( classnm_, dataobj->getClassName() );
}
