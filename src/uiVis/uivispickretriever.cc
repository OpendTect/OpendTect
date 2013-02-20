/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uivispickretriever.h"

#include "visevent.h"
#include "visseis2ddisplay.h"
#include "vissurvscene.h"
#include "vistransform.h"
#include "uivispartserv.h"
#include "mousecursor.h"

uiVisPickRetriever::uiVisPickRetriever( uiVisPartServer* ps )
    : visserv_(ps)
    , status_( Idle )
    , finished_( this )    
{
    resetPickedPos();
}


uiVisPickRetriever::~uiVisPickRetriever()
{
    while ( scenes_.size() )
	removeScene( scenes_[0] );
}


bool uiVisPickRetriever::enable(  const TypeSet<int>* scenes )
{
    if ( status_ == Waiting ) return false;

    status_ = Waiting;
    if ( scenes ) allowedscenes_ = *scenes;
    else allowedscenes_.erase();

    visserv_->setWorkMode( uiVisPartServer::Pick );
    MouseCursorManager::setOverride( MouseCursor::Cross );

    return true;
}


void uiVisPickRetriever::addScene( visSurvey::Scene* scene )
{
    scene->eventCatcher().eventhappened.notify(
	    mCB( this, uiVisPickRetriever, pickCB ) );
    scenes_ += scene;
}


void uiVisPickRetriever::removeScene( visSurvey::Scene* scene )
{
    scene->eventCatcher().eventhappened.remove(
	    mCB( this, uiVisPickRetriever, pickCB ) );
    scenes_ -= scene;
}


void uiVisPickRetriever::pickCB( CallBacker* cb )
{
    if ( status_!=Waiting )
	return;

    mCBCapsuleUnpackWithCaller( const visBase::EventInfo&,
	    			eventinfo, caller, cb );
    if ( eventinfo.type!=visBase::MouseClick )
	return;

    visSurvey::Scene* scene = 0;
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	if ( &scenes_[idx]->eventCatcher()==caller )
	{
	    scene = scenes_[idx];
	    break;
	}
    }

    if ( !scene || scene->eventCatcher().isHandled() )
	return;

    scene->eventCatcher().setHandled();

    if ( eventinfo.pressed ) //Block other objects from taking action
	return;

    if ( (!allowedscenes_.isEmpty() &&
	  allowedscenes_.indexOf( scene->id() )==-1)
	  || eventinfo.pickedobjids.isEmpty() )
	status_ = Failed;
    else
    {
	pickedpos_ = eventinfo.worldpickedpos;
	pickedscene_ = scene->id();
	buttonstate_ = eventinfo.buttonstate_;
	status_ = Success;
    }

    pickedobjids_ = eventinfo.pickedobjids;
    for ( int idx=0; idx<pickedobjids_.size(); idx++ )
    {
	visBase::DataObject* dataobj =
		visBase::DM().getObject( pickedobjids_[idx] );
	if ( !dataobj ) continue;

	mDynamicCastGet(visSurvey::Seis2DDisplay*,s2dd,dataobj);
	if ( !s2dd || !s2dd->isOn() )
	    continue;

	pickedgeomid_ = s2dd->getGeomID();
	PosInfo::Line2DPos pos2d;
	const bool res =
	    s2dd->getGeometry().getPos( pickedpos_, pos2d, mUdf(float) );
	if ( res )
	    pickedtrcnr_ = pos2d.nr_;
    }
    
    MouseCursorManager::restoreOverride();
    visserv_->setWorkMode( uiVisPartServer::View );
    finished_.trigger();

    if ( status_ != Waiting )
	status_ = Idle;

    resetPickedPos();
}


void uiVisPickRetriever::reset()
{
    status_ = Idle;
    allowedscenes_.erase();
    MouseCursorManager::restoreOverride();
    visserv_->setWorkMode( uiVisPartServer::View );
    resetPickedPos();
}


void uiVisPickRetriever::resetPickedPos()
{
    pickedgeomid_.setUndef();
    pickedtrcnr_ = mUdf(int);
    pickedpos_ = Coord3::udf();
    pickedscene_ = -1;
    pickedobjids_.erase();
}
