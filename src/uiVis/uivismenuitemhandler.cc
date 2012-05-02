/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Mar 2002
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: uivismenuitemhandler.cc,v 1.5 2012-05-02 11:54:02 cvskris Exp $";

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
