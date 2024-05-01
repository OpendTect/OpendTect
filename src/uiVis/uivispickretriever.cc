/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivispickretriever.h"

#include "mousecursor.h"
#include "uivispartserv.h"
#include "visevent.h"
#include "visseis2ddisplay.h"
#include "vissurvscene.h"
#include "vistransform.h"
#include "zaxistransform.h"


uiVisPickRetriever::uiVisPickRetriever( uiVisPartServer* ps )
    : visserv_(ps)
    , pickedgeomid_(Pos::GeomID::udf())
    , finished_(this)
{
    resetPickedPos();
}


uiVisPickRetriever::~uiVisPickRetriever()
{
    detachAllNotifiers();
}


bool uiVisPickRetriever::enable( const TypeSet<SceneID>* scenes )
{
    if ( status_ == Waiting )
	return false;

    status_ = Waiting;
    if ( scenes )
	allowedscenes_ = *scenes;
    else
	allowedscenes_.erase();

    visserv_->setWorkMode( uiVisPartServer::Pick );
    MouseCursorManager::setOverride( MouseCursor::Cross );

    return true;
}


void uiVisPickRetriever::addScene( visSurvey::Scene* scene )
{
    mAttachCB( scene->eventCatcher().eventhappened, uiVisPickRetriever::pickCB);
}


void uiVisPickRetriever::removeScene( visSurvey::Scene* scene )
{
    mDetachCB( scene->eventCatcher().eventhappened, uiVisPickRetriever::pickCB);
}


void uiVisPickRetriever::pickCB( CallBacker* cb )
{
    if ( status_!=Waiting || !visserv_ )
	return;

    mCBCapsuleUnpackWithCaller( const visBase::EventInfo&,
				eventinfo, caller, cb );
    if ( eventinfo.type != visBase::MouseClick )
	return;

    RefMan<visSurvey::Scene> curscene;
    TypeSet<SceneID> sceneids;
    visserv_->getSceneIds( sceneids );
    for ( const auto& sceneid : sceneids )
    {
	RefMan<visSurvey::Scene> scene = visserv_->getScene( sceneid );
	if ( scene && &scene->eventCatcher()==caller )
	{
	    curscene = scene;
	    break;
	}
    }

    if ( !curscene || curscene->eventCatcher().isHandled() )
	return;

    curscene->eventCatcher().setHandled();

    if ( eventinfo.pressed ) //Block other objects from taking action
	return;

    if ( (!allowedscenes_.isEmpty() &&
	  !allowedscenes_.isPresent(curscene->getID())) ||
	  eventinfo.pickedobjids.isEmpty() )
	status_ = Failed;
    else
    {
	pickedpos_ = eventinfo.worldpickedpos;
	pickedscene_ = curscene->getID();
	buttonstate_ = eventinfo.buttonstate_;
	status_ = Success;
    }

    pickedobjids_ = eventinfo.pickedobjids;
    for ( int idx=0; idx<pickedobjids_.size(); idx++ )
    {
	visBase::DataObject* dataobj =
		visBase::DM().getObject( pickedobjids_[idx] );
	if ( !dataobj )
	    continue;

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
    pickedgeomid_ = Survey::GeometryManager::cUndefGeomID();
    pickedtrcnr_ = mUdf(int);
    pickedpos_ = Coord3::udf();
    pickedscene_.setUdf();
    pickedobjids_.erase();
}


const ZAxisTransform* uiVisPickRetriever::getZAxisTransform() const
{
    TypeSet<SceneID> sceneids;
    visserv_->getSceneIds( sceneids );
    for ( const auto& sceneid : sceneids )
    {
	if ( sceneid != pickedscene_ )
	    continue;

	ConstRefMan<visSurvey::Scene> scene = visserv_->getScene( sceneid );
	return scene ? scene->getZAxisTransform() : nullptr;
    }

    return nullptr;
}


SceneID uiVisPickRetriever::unTransformedSceneID() const
{
    TypeSet<SceneID> sceneids;
    visserv_->getSceneIds( sceneids );
    for ( const auto& sceneid : sceneids )
    {
	ConstRefMan<visSurvey::Scene> scene = visserv_->getScene( sceneid );
	if ( scene && !scene->getZAxisTransform() )
	    return scene->getID();
    }

    return SceneID::udf();
}
