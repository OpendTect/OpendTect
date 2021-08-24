/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Mar 2002
________________________________________________________________________

-*/

#include "uivismenuitemhandler.h"

#include "uivispartserv.h"
#include "vispicksetdisplay.h"


uiVisMenuItemHandler::uiVisMenuItemHandler(const char* classnm,
	uiVisPartServer& vps, const uiString& nm, const CallBack& cb,
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
    mDynamicCastGet(visSurvey::SurveyObject*,survobj,dataobj.ptr())
    if ( !survobj )
	return false;

    return FixedString(classnm_) == survobj->factoryKeyword();
}


// uiPickSetPolygonMenuItemHandler
uiPickSetPolygonMenuItemHandler::uiPickSetPolygonMenuItemHandler(
	uiVisPartServer& vps, const uiString& nm,
	const CallBack& cb, const char* parenttext, int placement )
    : MenuItemHandler( *vps.getMenuHandler(), nm, cb, parenttext, placement )
    , visserv_(vps)
    , addwhenpolygon_(true)
    , addwhenpickset_(true)
{}


void uiPickSetPolygonMenuItemHandler::addWhenPickSet( bool yn )
{ addwhenpickset_ = yn; }

void uiPickSetPolygonMenuItemHandler::addWhenPolygon( bool yn )
{ addwhenpolygon_ = yn; }


int uiPickSetPolygonMenuItemHandler::getDisplayID() const
{ return menuhandler_.menuID(); }


bool uiPickSetPolygonMenuItemHandler::shouldAddMenu() const
{
    RefMan<visBase::DataObject> dataobj =
		visserv_.getObject( menuhandler_.menuID() );
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,dataobj.ptr())
    if ( !psd )
	return false;

    if ( addwhenpolygon_==addwhenpickset_ )
	return addwhenpolygon_;

    return psd->isPolygon()==addwhenpolygon_;
}
