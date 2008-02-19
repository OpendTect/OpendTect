/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: visfaultdisplay.cc,v 1.4 2008-02-19 16:27:14 cvsjaap Exp $";

#include "visfaultdisplay.h"

#include "emeditor.h"
#include "emfault.h"
#include "emmanager.h"
#include "executor.h"
#include "explfaultsticksurface.h"
#include "faultsticksurface.h"
#include "iopar.h"
#include "mpeengine.h"
#include "randcolor.h"
#include "visevent.h"
#include "visgeomindexedshape.h"
#include "vismaterial.h"
#include "vismpeeditor.h"
#include "vistransform.h"

mCreateFactoryEntry( visSurvey::FaultDisplay );

namespace visSurvey
{

FaultDisplay::FaultDisplay()
    : VisualObjectImpl(true)
    , emfault_( 0 )
    , displaysurface_( 0 )
    , editor_( 0 )
    , eventcatcher_( 0 )
    , explicitsurface_( 0 )
    , displaytransform_( 0 )
{
    setColor( getRandomColor( false ) );
}


FaultDisplay::~FaultDisplay()
{
    setSceneEventCatcher( 0 );
    if ( editor_ ) editor_->unRef();

    if ( emfault_ ) MPE::engine().removeEditor( emfault_->id() );

    if ( displaysurface_ )
	displaysurface_->unRef();

    delete explicitsurface_;

    if ( emfault_ ) emfault_->unRef();
    if ( displaytransform_ ) displaytransform_->unRef();
}


void FaultDisplay::setSceneEventCatcher( visBase::EventCatcher* vec )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove( mCB(this,FaultDisplay,pickCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = vec;
    
    if ( eventcatcher_ )
    {
	eventcatcher_->ref();
	eventcatcher_->eventhappened.notify( mCB(this,FaultDisplay,pickCB) );
    }

    if ( editor_ ) editor_->setSceneEventCatcher( eventcatcher_ );
}


EM::ObjectID FaultDisplay::getEMID() const
{ return emfault_ ? emfault_->id() : -1; }


#define mErrRet(s) { errmsg = s; return false; }

bool FaultDisplay::setEMID( const EM::ObjectID& emid )
{
    if ( emfault_ )
	emfault_->unRef();

    emfault_ = 0;
    if ( editor_ ) editor_->setEditor( (MPE::ObjectEditor*) 0 );

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet( EM::Fault*, emfault, emobject.ptr() );
    if ( !emfault )
    {
	if ( displaysurface_ ) displaysurface_->turnOn( false );
	return false;
    }

    emfault_ = emfault;
    emfault_->ref();

    getMaterial()->setColor( emfault_->preferredColor() );
    if ( !emfault_->name().isEmpty() )
	setName( emfault_->name() );

    if ( !displaysurface_ )
    {
	displaysurface_ = visBase::GeomIndexedShape::create();
	displaysurface_->ref();
	displaysurface_->setDisplayTransformation( displaytransform_ );
	displaysurface_->setMaterial( 0 );
	displaysurface_->setSelectable( false );
	displaysurface_->setRightHandSystem( righthandsystem_ );
	addChild( displaysurface_->getInventorNode() );

	mDynamicCastGet( Geometry::FaultStickSurface*, fss,
			 emfault_->sectionGeometry( emfault_->sectionID(0)) );

	explicitsurface_ = new Geometry::ExplFaultStickSurface( fss ); 
	displaysurface_->setSurface( explicitsurface_ );
	if ( explicitsurface_ ) 
	    explicitsurface_->updateAll();
	displaysurface_->touch( false );
    }

    if ( !editor_ )
    {
	editor_ = visSurvey::MPEEditor::create();
	editor_->ref();
	editor_->setSceneEventCatcher( eventcatcher_ );
	editor_->setDisplayTransformation( displaytransform_ );
    }

    editor_->setEditor( MPE::engine().getEditor( emid, true ) );

    displaysurface_->turnOn( true );

    return true;
}


MultiID FaultDisplay::getMultiID() const
{
    return emfault_ ? emfault_->multiID() : MultiID();
}


void FaultDisplay::setColor( Color nc )
{
    if ( emfault_ ) emfault_->setPreferredColor(nc);
    getMaterial()->setColor( nc );
}


NotifierAccess* FaultDisplay::materialChange()
{ return &getMaterial()->change; }


Color FaultDisplay::getColor() const
{ return getMaterial()->getColor(); }


void FaultDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    par.set( sKeyEarthModelID(), getMultiID() );
}


int FaultDisplay::usePar( const IOPar& par )
{
    int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    MultiID newmid;
    if ( par.get(sKeyEarthModelID(),newmid) )
    {
	EM::ObjectID emid = EM::EMM().getObjectID( newmid );
	RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
	if ( !emobject )
	{
	    PtrMan<Executor> loader = EM::EMM().objectLoader( newmid );
	    if ( loader ) loader->execute();
	    emid = EM::EMM().getObjectID( newmid );
	    emobject = EM::EMM().getObject( emid );
	}

	if ( emobject ) setEMID( emobject->id() );
    }

    return 1;
}


void FaultDisplay::setDisplayTransformation(visBase::Transformation* nt)
{
    if ( displaysurface_ ) displaysurface_->setDisplayTransformation( nt );
    if ( editor_ ) editor_->setDisplayTransformation( nt );

    if ( displaytransform_ ) displaytransform_->unRef();
    displaytransform_ = nt;
    if ( displaytransform_ ) displaytransform_->ref();
}


void FaultDisplay::setRightHandSystem(bool yn)
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
    if ( displaysurface_ ) displaysurface_->setRightHandSystem( yn );
}


visBase::Transformation* FaultDisplay::getDisplayTransformation()
{ return displaytransform_; }


void FaultDisplay::pickCB( CallBacker* cb )
{
    if ( !emfault_ || !isOn() || eventcatcher_->isHandled() || !isSelected() )
	return;

    if ( editor_ && !editor_->clickCB( cb ) )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    if ( eventinfo.type!=visBase::MouseClick ||
	 eventinfo.mousebutton!=visBase::EventInfo::leftMouseButton() )
	return;

    if ( eventinfo.pressed )
	return;

    if ( !eventinfo.worldpickedpos.isDefined() )
	return;

    const EM::SectionID sid = emfault_->sectionID(0);
    RefMan<MPE::ObjectEditor> emeditor = editor_->getMPEEditor();

    Geometry::FaultStickSurface* sticksurface =
	(Geometry::FaultStickSurface*) emfault_->sectionGeometry( sid );
/*
    if ( !emfault_->geometry().nrSticks() )
    {
	const EM::PosID pid( emfault_->id(), sid, 0 );
	emfault_->setPos( pid, Coord3(eventinfo.worldpickedpos), true );
	eventcatcher_->setHandled();
	return;
    }
    */
}


}; // namespace visSurvey
