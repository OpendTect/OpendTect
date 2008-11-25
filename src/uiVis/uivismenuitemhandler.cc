/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Mar 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uivismenuitemhandler.cc,v 1.2 2008-11-25 15:35:26 cvsbert Exp $";

#include "uivismenuitemhandler.h"

#include "uivispartserv.h"
#include "visdata.h"


uiVisMenuItemHandler::uiVisMenuItemHandler(const char* classnm,
	uiVisPartServer& vps, const char* nm, const CallBack& cb,
	int placement )
    : MenuItemHandler( *vps.getMenuHandler(), nm, cb, placement )
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
