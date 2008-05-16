/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: visfaultdisplay.cc,v 1.12 2008-05-16 22:36:04 cvskris Exp $";

#include "visfaultdisplay.h"

#include "emeditor.h"
#include "emfault.h"
#include "emmanager.h"
#include "executor.h"
#include "explfaultsticksurface.h"
#include "faultsticksurface.h"
#include "faulteditor.h"
#include "iopar.h"
#include "mpeengine.h"
#include "randcolor.h"
#include "survinfo.h"
#include "viscoord.h"
#include "visdragger.h"
#include "visevent.h"
#include "visgeomindexedshape.h"
#include "vismaterial.h"
#include "vismarker.h"
#include "vismultitexture2.h"
#include "vismpeeditor.h"
#include "visplanedatadisplay.h"
#include "vispolyline.h"
#include "visshapehints.h"
#include "vistransform.h"

mCreateFactoryEntry( visSurvey::FaultDisplay );

#define mNearestKnotSize 4
#define mDefaultKnotSize 2

namespace visSurvey
{

FaultDisplay::FaultDisplay()
    : emfault_( 0 )
    , neareststickmarker_( visBase::IndexedPolyLine3D::create() )
    , paneldisplay_( 0 )
    , stickdisplay_( 0 )
    , viseditor_( 0 )
    , faulteditor_( 0 )
    , eventcatcher_( 0 )
    , explicitpanels_( 0 )
    , explicitsticks_( 0 )
    , displaytransform_( 0 )
    , neareststick_( mUdf(int) )
    , shapehints_( visBase::ShapeHints::create() )
{
    neareststickmarker_->ref();
    neareststickmarker_->setRadius( 20 );
    if ( !neareststickmarker_->getMaterial() )
	neareststickmarker_->setMaterial( visBase::Material::create() );
    addChild( neareststickmarker_->getInventorNode() );

    setColor( getRandomColor( false ) );
    getMaterial()->setAmbience( 0.2 );
    shapehints_->ref();
    addChild( shapehints_->getInventorNode() );
    shapehints_->setVertexOrder( visBase::ShapeHints::CounterClockWise );
}


FaultDisplay::~FaultDisplay()
{
    setSceneEventCatcher( 0 );
    if ( viseditor_ ) viseditor_->unRef();

    if ( emfault_ ) MPE::engine().removeEditor( emfault_->id() );
    if ( faulteditor_ ) faulteditor_->unRef();
    faulteditor_ = 0;

    if ( paneldisplay_ )
	paneldisplay_->unRef();

    if ( stickdisplay_ )
	stickdisplay_->unRef();

    delete explicitpanels_;
    delete explicitsticks_;

    if ( emfault_ )
    {
	emfault_->change.remove( mCB(this,FaultDisplay,emChangeCB) );
       	emfault_->unRef();
    }

    if ( displaytransform_ ) displaytransform_->unRef();
    shapehints_->unRef();

    neareststickmarker_->unRef();
}


void FaultDisplay::setSceneEventCatcher( visBase::EventCatcher* vec )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove( mCB(this,FaultDisplay,mouseCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = vec;
    
    if ( eventcatcher_ )
    {
	eventcatcher_->ref();
	eventcatcher_->eventhappened.notify( mCB(this,FaultDisplay,mouseCB) );
    }

    if ( viseditor_ ) viseditor_->setSceneEventCatcher( eventcatcher_ );
}


EM::ObjectID FaultDisplay::getEMID() const
{ return emfault_ ? emfault_->id() : -1; }


#define mErrRet(s) { errmsg = s; return false; }

bool FaultDisplay::setEMID( const EM::ObjectID& emid )
{
    if ( emfault_ )
    {
	emfault_->change.remove( mCB(this,FaultDisplay,emChangeCB) );
	emfault_->unRef();
    }

    emfault_ = 0;
    if ( faulteditor_ ) faulteditor_->unRef();
    faulteditor_ = 0;
    if ( viseditor_ ) viseditor_->setEditor( (MPE::ObjectEditor*) 0 );

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet( EM::Fault*, emfault, emobject.ptr() );
    if ( !emfault )
    {
	if ( paneldisplay_ ) paneldisplay_->turnOn( false );
	if ( stickdisplay_ ) stickdisplay_->turnOn( false );
	return false;
    }

    emfault_ = emfault;
    emfault_->change.notify( mCB(this,FaultDisplay,emChangeCB) );
    emfault_->ref();

    getMaterial()->setColor( emfault_->preferredColor() );
    neareststickmarker_->getMaterial()->setColor(
	    emfault_->preferredColor().complementaryColor() );
    if ( !emfault_->name().isEmpty() )
	setName( emfault_->name() );

    if ( !paneldisplay_ )
    {
	paneldisplay_ = visBase::GeomIndexedShape::create();
	paneldisplay_->ref();
	paneldisplay_->setDisplayTransformation( displaytransform_ );
	paneldisplay_->setMaterial( 0 );
	paneldisplay_->setSelectable( false );
	paneldisplay_->setRightHandSystem( righthandsystem_ );
	addChild( paneldisplay_->getInventorNode() );
    }

    if ( !stickdisplay_ )
    {
	stickdisplay_ = visBase::GeomIndexedShape::create();
	stickdisplay_->ref();
	stickdisplay_->setDisplayTransformation( displaytransform_ );
	stickdisplay_->setMaterial( 0 );
	stickdisplay_->setSelectable( false );
	stickdisplay_->setRightHandSystem( righthandsystem_ );
	addChild( stickdisplay_->getInventorNode() );
    }


    if ( !explicitpanels_ )
    {
	const float zscale = SI().zFactor() * scene_->getZScale();
	explicitpanels_ = new Geometry::ExplFaultStickSurface( 0, zscale );
	explicitpanels_->display( false, true );
	explicitsticks_ = new Geometry::ExplFaultStickSurface( 0, zscale );
	explicitsticks_->display( true, false );
    }

    mDynamicCastGet( Geometry::FaultStickSurface*, fss,
		     emfault_->sectionGeometry( emfault_->sectionID(0)) );

    explicitpanels_->setSurface( fss ); 
    paneldisplay_->setSurface( explicitpanels_ );
    paneldisplay_->touch( true );

    explicitsticks_->setSurface( fss ); 
    stickdisplay_->setSurface( explicitsticks_ );
    stickdisplay_->touch( true );

    if ( !viseditor_ )
    {
	viseditor_ = visSurvey::MPEEditor::create();
	viseditor_->ref();
	viseditor_->setSceneEventCatcher( eventcatcher_ );
	viseditor_->setDisplayTransformation( displaytransform_ );
	addChild( viseditor_->getInventorNode() );
    }

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::FaultEditor*, fe, editor.ptr() );
    faulteditor_ =  fe;
    if ( faulteditor_ ) faulteditor_->ref();

    viseditor_->setEditor( faulteditor_ );
    
    paneldisplay_->turnOn( true );
    stickdisplay_->turnOn( true );

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


void FaultDisplay::display( bool sticks, bool panels )
{
    if ( stickdisplay_ )
	stickdisplay_->turnOn( sticks );

    if ( paneldisplay_ )
	paneldisplay_->turnOn( panels );
}


bool FaultDisplay::areSticksDisplayed() const
{
    return stickdisplay_ ? stickdisplay_->isOn() : false;
}


bool FaultDisplay::arePanelsDisplayed() const
{
    return paneldisplay_ ? paneldisplay_->isOn() : false;
}


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
    if ( paneldisplay_ ) paneldisplay_->setDisplayTransformation( nt );
    if ( stickdisplay_ ) stickdisplay_->setDisplayTransformation( nt );
    if ( viseditor_ ) viseditor_->setDisplayTransformation( nt );
    neareststickmarker_->setDisplayTransformation( nt );

    if ( displaytransform_ ) displaytransform_->unRef();
    displaytransform_ = nt;
    if ( displaytransform_ ) displaytransform_->ref();
}


void FaultDisplay::setRightHandSystem(bool yn)
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
    if ( paneldisplay_ ) paneldisplay_->setRightHandSystem( yn );
    if ( stickdisplay_ ) stickdisplay_->setRightHandSystem( yn );
}


visBase::Transformation* FaultDisplay::getDisplayTransformation()
{ return displaytransform_; }


void FaultDisplay::mouseCB( CallBacker* cb )
{
    if ( !emfault_ || !faulteditor_ || !isOn() || eventcatcher_->isHandled() ||
	 !isSelected() )
	return;

    //if ( viseditor_ && !viseditor_->clickCB( cb ) )
	//return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
   
    CubeSampling mouseplanecs; 
    mouseplanecs.setEmpty();
    EM::PosID mousepid( EM::PosID::udf() );
    bool mouseondragger = false;

    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	const int visid = eventinfo.pickedobjids[idx];
	visBase::DataObject* dataobj = visBase::DM().getObject( visid );

	mDynamicCastGet( visSurvey::PlaneDataDisplay*, plane, dataobj );
	if ( plane )
	{
	    mouseplanecs = plane->getCubeSampling();
	    break;
	}
    }

    if ( locked_ )
	return;

    Coord3 pos = eventinfo.displaypickedpos;
    pos = scene_->getZScaleTransform()->transformBack( pos ); 
    if ( displaytransform_ ) pos = displaytransform_->transformBack( pos ); 

    EM::PosID nearestpid0, nearestpid1, insertpid;
    faulteditor_->getInteractionInfo( nearestpid0, nearestpid1, insertpid,
	    			      pos, SI().zFactor() );

    const int neareststick = nearestpid0.isUdf()
	? mUdf(int)
	: RowCol(insertpid.subID()).row;

    if ( neareststick_!=neareststick )
    {
	neareststick_ = neareststick;
	updateNearestStickMarker();
    }

    if ( !pos.isDefined() )
	return;

    if ( eventinfo.type==visBase::MouseClick &&
	 OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	 !OD::altKeyboardButton(eventinfo.buttonstate_) &&
	 !OD::shiftKeyboardButton(eventinfo.buttonstate_) &&
	 OD::leftMouseButton(eventinfo.buttonstate_) )
    {
	if ( !eventinfo.pressed )
	{
	    const EM::PosID pid =
		viseditor_->mouseClickDragger( eventinfo.pickedobjids );
	    if ( !pid.isUdf() )
	    {
		const int removestick = RowCol(pid.subID()).row;
		if ( emfault_->geometry().nrKnots(
			    pid.sectionID(),removestick)==1 )
		{
		    emfault_->geometry().removeStick( pid.sectionID(),
			    			      removestick, true );
		}
		else
		{
		    emfault_->geometry().removeKnot( pid.sectionID(),
			    			     pid.subID(), true );
		}

		paneldisplay_->touch( false );
		stickdisplay_->touch( false );
	    }
	}
	eventcatcher_->setHandled();
	return;
    }


    if ( eventinfo.type!=visBase::MouseClick || eventinfo.pressed ||
	 OD::altKeyboardButton(eventinfo.buttonstate_) ||
	 !OD::leftMouseButton(eventinfo.buttonstate_) )
	return;

    if ( insertpid.isUdf() )
	return;

    if ( nearestpid0.isUdf() )
    {
	//Add Stick
	Coord3 editnormal(0,0,1);
	if ( mouseplanecs.isEmpty() )
	    return;

	const int insertstick = insertpid.isUdf()
	    ? mUdf(int)
	    : RowCol(insertpid.subID()).row;

	if (  mouseplanecs.defaultDir()==CubeSampling::Inl )
	    editnormal = Coord3( SI().binID2Coord().rowDir(), 0);
	else if ( mouseplanecs.defaultDir()==CubeSampling::Crl ) 
	    editnormal = Coord3( SI().binID2Coord().colDir(), 0 );

	emfault_->geometry().insertStick( insertpid.sectionID(), insertstick,
					  pos, editnormal, true );
	paneldisplay_->touch( false );
	stickdisplay_->touch( false );
	faulteditor_->editpositionchange.trigger();
    }
    else
    {
	emfault_->geometry().insertKnot( insertpid.sectionID(),
		insertpid.subID(), pos, true );
	faulteditor_->editpositionchange.trigger();
    }

    eventcatcher_->setHandled();
}


void FaultDisplay::emChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);

    if ( cbdata.event==EM::EMObjectCallbackData::BurstAlert ||
	 cbdata.event==EM::EMObjectCallbackData::SectionChange ||
	 cbdata.event==EM::EMObjectCallbackData::PositionChange )
    {
	if ( cbdata.event==EM::EMObjectCallbackData::PositionChange )
	{
	     if ( RowCol(cbdata.pid0.subID()).row==neareststick_ )
		updateNearestStickMarker();
	}
	else
	    updateNearestStickMarker();

	paneldisplay_->touch( false );
	stickdisplay_->touch( false );
    }
}


void FaultDisplay::updateNearestStickMarker()
{
    if ( mIsUdf(neareststick_) )
	neareststickmarker_->turnOn( false );
    else
    {
	mDynamicCastGet( Geometry::FaultStickSurface*, fss,
			 emfault_->sectionGeometry( emfault_->sectionID(0)) );

	const StepInterval<int> rowrg = fss->rowRange();
	if ( rowrg.isUdf() || !rowrg.includes(neareststick_) )
	{
	    neareststickmarker_->turnOn( false );
	    return;
	}

	const StepInterval<int> colrg = fss->colRange( neareststick_ );
	if ( colrg.isUdf() || colrg.start==colrg.stop )
	{
	    neareststickmarker_->turnOn( false );
	    return;
	}

	neareststickmarker_->removeCoordIndexAfter(-1);
	neareststickmarker_->getCoordinates()->removeAfter(-1);

	int idx = 0;
	RowCol rc( neareststick_, 0 );
	for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col += colrg.step )
	{
	    const Coord3 pos = fss->getKnot( rc );
	    const int ci = neareststickmarker_->getCoordinates()->addPos( pos );
	    neareststickmarker_->setCoordIndex( idx++, ci );
	}

	neareststickmarker_->turnOn( true );
    }
}


void FaultDisplay::showManipulator( bool yn )
{
    viseditor_->turnOn( yn );
    neareststickmarker_->turnOn( yn );
}


bool  FaultDisplay::isManipulatorShown() const
{ return viseditor_->isOn(); }


int FaultDisplay::nrResolutions() const
{
    return texture_->canUseShading() ? 1 : 3;
}


void FaultDisplay::setResolution( int res )
{
    if ( texture_->canUseShading() )
	return;

    if ( res==resolution_ )
	return;

    resolution_ = res;
    texture_->clearAll();
}


bool FaultDisplay::getCacheValue( int attrib, int version, const Coord3& crd,
	                          float& value ) const
{ return true; }


void FaultDisplay::addCache()
{}

void FaultDisplay::removeCache( int attrib )
{}

void FaultDisplay::swapCache( int attr0, int attr1 )
{}

void FaultDisplay::emptyCache( int attrib )
{}

bool FaultDisplay::hasCache( int attrib ) const
{ return false; }


}; // namespace visSurvey
