/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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


VisID uiVisMenuItemHandler::getDisplayID() const
{
    return VisID(menuhandler_.menuID());
}


bool uiVisMenuItemHandler::shouldAddMenu() const
{
    RefMan<visBase::DataObject> dataobj = visserv_.getObject( getDisplayID() );
    mDynamicCastGet(visSurvey::SurveyObject*,survobj,dataobj.ptr())
    if ( !survobj )
	return false;

    return StringView(classnm_) == survobj->factoryKeyword();
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


VisID uiPickSetPolygonMenuItemHandler::getDisplayID() const
{
    return VisID(menuhandler_.menuID());
}


bool uiPickSetPolygonMenuItemHandler::shouldAddMenu() const
{
    RefMan<visBase::DataObject> dataobj = visserv_.getObject( getDisplayID() );
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,dataobj.ptr())
    if ( !psd )
	return false;

    if ( addwhenpolygon_==addwhenpickset_ )
	return addwhenpolygon_;

    return psd->isPolygon()==addwhenpolygon_;
}
