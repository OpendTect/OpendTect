/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/


#include "vispolygonbodydisplay.h"

#include "color.h"
#include "empolygonbody.h"
#include "emmanager.h"
#include "executor.h"
#include "explpolygonsurface.h"
#include "explplaneintersection.h"
#include "polygonsurfeditor.h"
#include "iopar.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "undo.h"
#include "viscoord.h"
#include "visdrawstyle.h"
#include "visevent.h"
#include "visgeomindexedshape.h"
#include "vismaterial.h"
#include "vismpeeditor.h"
#include "visnormals.h"
#include "visplanedatadisplay.h"
#include "vispolyline.h"
#include "vistransform.h"
#include "vistristripset.h"
#include "vispolygonoffset.h"

namespace visSurvey
{

PolygonBodyDisplay::PolygonBodyDisplay()
    : visBase::VisualObjectImpl(true)
    , empolygonsurf_( 0 )
    , bodydisplay_( 0 )
    , explicitbody_( 0 )
    , polygondisplay_( 0 )
    , explicitpolygons_( 0 )
    , intersectiondisplay_( 0 )
    , explicitintersections_( 0 )
    , viseditor_( 0 )
    , polygonsurfeditor_( 0 )
    , eventcatcher_( 0 )
    , displaytransform_( 0 )
    , nearestpolygon_( mUdf(int) )
    , nearestpolygonmarker_( visBase::PolyLine3D::create() )
    , showmanipulator_( false )
    , displaypolygons_( false )
    , drawstyle_( new visBase::DrawStyle )
    , intsurf_( visBase::TriangleStripSet::create() )
{
    nearestpolygonmarker_->ref();
    if ( !nearestpolygonmarker_->getMaterial() )
	nearestpolygonmarker_->setMaterial( new visBase::Material );
    addChild( nearestpolygonmarker_->osgNode() );
    getMaterial()->setAmbience( 0.2 );

    nearestpolygonmarker_->setPrimitiveType(Geometry::PrimitiveSet::LineStrips);

    drawstyle_->ref();
    drawstyle_->setLineStyle( OD::LineStyle(OD::LineStyle::Solid,2) );

    intsurf_->ref();
    intsurf_->turnOn( false );
    intsurf_->setNormalBindType( visBase::VertexShape::BIND_PER_VERTEX );
    intsurf_->setColorBindType( visBase::VertexShape::BIND_OVERALL );
    intsurf_->setRenderMode( visBase::RenderBothSides );

    addChild( intsurf_->osgNode() );

    visBase::PolygonOffset* polyoffset = new visBase::PolygonOffset;
    polyoffset->setFactor( -1.0f );
    polyoffset->setUnits( 1.0f );
    nearestpolygonmarker_->addNodeState( polyoffset );

    if ( getMaterial() )
	  mAttachCB( getMaterial()->change, PolygonBodyDisplay::matChangeCB );

}


PolygonBodyDisplay::~PolygonBodyDisplay()
{
    detachAllNotifiers();
    setSceneEventCatcher( 0 );

    if ( viseditor_ )
	viseditor_->unRef();

    if ( polygonsurfeditor_ )
	polygonsurfeditor_->unRef();

    polygonsurfeditor_ = 0;

    if ( empolygonsurf_ )
    {
	MPE::engine().removeEditor( empolygonsurf_->id() );
	empolygonsurf_->change.remove(mCB(this,PolygonBodyDisplay,emChangeCB));
	empolygonsurf_->unRef();
    }

    if ( bodydisplay_ )
	bodydisplay_->unRef();

    if ( polygondisplay_ )
	polygondisplay_->unRef();

    if ( intersectiondisplay_ )
	intersectiondisplay_->unRef();

    if ( displaytransform_ )
	displaytransform_->unRef();

    delete explicitbody_;
    delete explicitpolygons_;
    delete explicitintersections_;

    nearestpolygonmarker_->unRef();
    drawstyle_->unRef(); drawstyle_ = 0;

    removeChild( intsurf_->osgNode() );
    intsurf_->unRef();
}


void PolygonBodyDisplay::setSceneEventCatcher( visBase::EventCatcher* vec )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(
		mCB(this,PolygonBodyDisplay,mouseCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = vec;
    if ( eventcatcher_ )
    {
	eventcatcher_->ref();
	eventcatcher_->eventhappened.notify(
		mCB(this,PolygonBodyDisplay,mouseCB) );
    }

    if ( viseditor_ )
	viseditor_->setSceneEventCatcher( eventcatcher_ );
}


EM::ObjectID PolygonBodyDisplay::getEMID() const
{
    return empolygonsurf_ ? empolygonsurf_->id() : -1;
}


const OD::LineStyle* PolygonBodyDisplay::lineStyle() const
{ return &drawstyle_->lineStyle(); }


void PolygonBodyDisplay::setLineStyle( const OD::LineStyle& lst )
{
    if ( lineStyle()->width_<0 || lst.width_<0 )
    {
	drawstyle_->setLineStyle( lst );
	scene_->objectMoved( 0 );
    }
    else
	drawstyle_->setLineStyle( lst );

    setLineRadius( intersectiondisplay_ );
    if ( displayedOnlyAtSections() )
	intersectiondisplay_->touch( false );

    setLineRadius( polygondisplay_ );
    if ( arePolygonsDisplayed() )
	polygondisplay_->touch( false );
}


void PolygonBodyDisplay::setLineRadius( visBase::GeomIndexedShape* shape )
{
    const bool islinesolid = lineStyle()->type_ == OD::LineStyle::Solid;
    const float linewidth = islinesolid ? 1.5f*lineStyle()->width_ : -1.0f;

    OD::LineStyle lnstyle( *lineStyle() ) ;
    lnstyle.width_ = (int)( linewidth );

    if ( shape )
	shape->setLineStyle( lnstyle );

    lnstyle.width_ = (int)( linewidth+ 2.5f );
    nearestpolygonmarker_->setLineStyle( lnstyle );
}


#define mErrRet(s) { errmsg_ = s; return false; }

bool PolygonBodyDisplay::setEMID( const EM::ObjectID& emid )
{
    if ( empolygonsurf_ )
    {
	empolygonsurf_->change.remove(
		mCB(this,PolygonBodyDisplay,emChangeCB) );
	empolygonsurf_->unRef();
    }

    if ( polygonsurfeditor_ ) polygonsurfeditor_->unRef();
    if ( viseditor_ ) viseditor_->setEditor( (MPE::ObjectEditor*) 0 );
    empolygonsurf_ = 0;
    polygonsurfeditor_ = 0;

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet( EM::PolygonBody*, emplys, emobject.ptr() );
    if ( !emplys )
    {
	if ( bodydisplay_ ) bodydisplay_->turnOn( false );
	if ( intersectiondisplay_ ) intersectiondisplay_->turnOn( false );
	if ( polygondisplay_ ) polygondisplay_->turnOn( false );
	intsurf_->turnOn( false );
	return false;
    }

    empolygonsurf_ = emplys;
    empolygonsurf_->change.notify( mCB(this,PolygonBodyDisplay,emChangeCB) );
    empolygonsurf_->ref();

    if ( !empolygonsurf_->name().isEmpty() )
	setName( empolygonsurf_->name() );

    if ( !bodydisplay_ )
    {
	bodydisplay_ = visBase::GeomIndexedShape::create();
	bodydisplay_->ref();
	bodydisplay_->setGeometryShapeType( visBase::GeomIndexedShape::Triangle,
	    Geometry::PrimitiveSet::Triangles );
	bodydisplay_->setDisplayTransformation( displaytransform_ );
	bodydisplay_->setMaterial( 0 );
	bodydisplay_->setSelectable( false );
	bodydisplay_->setNormalBindType(visBase::VertexShape::BIND_PER_VERTEX);
	bodydisplay_->useOsgNormal( true );
	addChild( bodydisplay_->osgNode() );
    }

    if ( !intersectiondisplay_ )
    {
	intersectiondisplay_ = visBase::GeomIndexedShape::create();
	intersectiondisplay_->ref();
	intersectiondisplay_->setDisplayTransformation( displaytransform_ );
	bodydisplay_->setMaterial( 0 );
	intersectiondisplay_->setSelectable( false );
	intersectiondisplay_->setGeometryShapeType(
	    visBase::GeomIndexedShape::PolyLine3D,
	    Geometry::PrimitiveSet::LineStrips );

	addChild( intersectiondisplay_->osgNode() );
	intersectiondisplay_->turnOn( false );
	setLineRadius( intersectiondisplay_ );
    }

    if ( !polygondisplay_ )
    {
	polygondisplay_ = visBase::GeomIndexedShape::create();
	polygondisplay_->ref();
	polygondisplay_->setDisplayTransformation( displaytransform_ );

	polygondisplay_->setMaterial( getMaterial() );
	polygondisplay_->setSelectable( false );
	polygondisplay_->setGeometryShapeType(
	    visBase::GeomIndexedShape::PolyLine3D,
	    Geometry::PrimitiveSet::LineStrips );

	addChild( polygondisplay_->osgNode() );

	setLineRadius( polygondisplay_ );
    }

    const float zscale = scene_
	? scene_->getZScale() *scene_->getFixedZStretch()
	: SI().zScale();

    if ( !explicitbody_ )
    {
	mTryAlloc( explicitbody_,Geometry::ExplPolygonSurface(0,zscale));
	explicitbody_->display( false, true );
    }

    if ( !explicitpolygons_ )
    {
	mTryAlloc( explicitpolygons_,Geometry::ExplPolygonSurface(0,zscale) );
	explicitpolygons_->display( true, false );
    }

    if ( !explicitintersections_ )
	mTryAlloc( explicitintersections_, Geometry::ExplPlaneIntersection );

    mDynamicCastGet( Geometry::PolygonSurface*, polygonsurface,
	    empolygonsurf_->sectionGeometry(empolygonsurf_->sectionID(0)) );

    explicitbody_->setPolygonSurface( polygonsurface );
    bodydisplay_->setSurface( explicitbody_ );

    explicitintersections_->setShape( *explicitbody_ );
    intersectiondisplay_->setSurface( explicitintersections_ );

    explicitpolygons_->setPolygonSurface( polygonsurface );
    polygondisplay_->setSurface( explicitpolygons_ );

    if ( !viseditor_ )
    {
	viseditor_ = visSurvey::MPEEditor::create();
	viseditor_->ref();
	viseditor_->setSceneEventCatcher( eventcatcher_ );
	viseditor_->setDisplayTransformation( displaytransform_ );
	viseditor_->sower().alternateSowingOrder();
	addChild( viseditor_->osgNode() );
    }

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::PolygonBodyEditor*, pe, editor.ptr() );
    polygonsurfeditor_ =  pe;
    if ( polygonsurfeditor_ ) polygonsurfeditor_->ref();

    viseditor_->setEditor( polygonsurfeditor_ );
    bodydisplay_->turnOn( true );
    displaypolygons_ = false;

    nontexturecol_ = empolygonsurf_->preferredColor();
    updateSingleColor();
    updatePolygonDisplay();

    return true;
}


void PolygonBodyDisplay::touchAll( bool yn, bool updatemarker )
{
    if ( bodydisplay_ )
	bodydisplay_->touch( yn );

    if ( polygondisplay_ )
	polygondisplay_->touch( yn );

    if ( intersectiondisplay_ )
    {
	intersectiondisplay_->touch( yn );
	reMakeIntersectionSurface();
    }

    if ( updatemarker )
	updateNearestPolygonMarker();
}


bool PolygonBodyDisplay::removeSelections( TaskRunner* )
{
    const Selector<Coord3>* selector = scene_ ? scene_->getSelector() : 0;
    if ( !selector || !polygonsurfeditor_ )
	return false;

    polygonsurfeditor_->removeSelection( *selector );
    return true;
}


MultiID PolygonBodyDisplay::getMultiID() const
{
    return empolygonsurf_ ? empolygonsurf_->multiID() : MultiID();
}


void PolygonBodyDisplay::setColor( OD::Color nc )
{
    if ( empolygonsurf_ )
	empolygonsurf_->setPreferredColor(nc);
    else
    {
	nontexturecol_ = nc;
	updateSingleColor();
    }
}


void PolygonBodyDisplay::updateSingleColor()
{
    const OD::Color prevcol = getMaterial()->getColor();
    const OD::Color newcol = nontexturecol_.darker( 0.3 );
    if ( newcol==prevcol )
	return;

    getMaterial()->setColor( newcol );
    nearestpolygonmarker_->getMaterial()->setColor( nontexturecol_ );
    if ( polygondisplay_ )
	polygondisplay_->getMaterial()->setColor( nontexturecol_ );

    bodydisplay_->updateMaterialFrom( getMaterial() );
}


void PolygonBodyDisplay::matChangeCB(CallBacker*)
{
    if ( bodydisplay_ )
        bodydisplay_->updateMaterialFrom( getMaterial() );
}


OD::Color PolygonBodyDisplay::getColor() const
{
    return nontexturecol_;
}


void PolygonBodyDisplay::updatePolygonDisplay()
{
    if ( !polygondisplay_ )
	return;

    const EM::SectionID sid = empolygonsurf_->sectionID( 0 );
    const bool dodisplay = displaypolygons_ ||
	(isBodyDisplayed() && isManipulatorShown()) ||
	(isBodyDisplayed() && empolygonsurf_->geometry().nrPolygons(sid)==1);

    polygondisplay_->turnOn( dodisplay );
}


void PolygonBodyDisplay::display( bool polygons, bool body )
{
    displaypolygons_ = polygons;

    if ( nearestpolygonmarker_ )
	nearestpolygonmarker_->turnOn( polygons || body );

    viseditor_->turnOn( (polygons || body) && showmanipulator_ );

    if ( bodydisplay_ )
	bodydisplay_->turnOn( body );

    updatePolygonDisplay();
}



bool PolygonBodyDisplay::arePolygonsDisplayed() const
{
    return displaypolygons_;
}


bool PolygonBodyDisplay::isBodyDisplayed() const
{
    return bodydisplay_ ? bodydisplay_->isOn() : false;
}


void PolygonBodyDisplay::setMarkerStyle( const MarkerStyle3D& ms )
{
    if ( viseditor_ )
	viseditor_->setMarkerStyle( ms );
}


const MarkerStyle3D* PolygonBodyDisplay::markerStyle() const
{
    return viseditor_ ? viseditor_->markerStyle() : nullptr;
}


void PolygonBodyDisplay::fillPar( IOPar& par ) const
{
    visBase::VisualObjectImpl::fillPar( par );
    visSurvey::SurveyObject::fillPar( par );
    par.set( sKeyEMPolygonSurfID(), getMultiID() );
}


bool PolygonBodyDisplay::usePar( const IOPar& par )
{
    if ( !visBase::VisualObjectImpl::usePar( par ) ||
	 !visSurvey::SurveyObject::usePar( par ) )

	 return false;

    MultiID newmid;
    if ( par.get(sKeyEMPolygonSurfID(),newmid) )
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

    return true;
}


void PolygonBodyDisplay::setDisplayTransformation(const mVisTrans* nt)
{
    if ( bodydisplay_ )
	bodydisplay_->setDisplayTransformation( nt );

    if ( polygondisplay_ )
	polygondisplay_->setDisplayTransformation( nt );

    if ( intersectiondisplay_ )
	intersectiondisplay_->setDisplayTransformation( nt );

    intsurf_->setDisplayTransformation( nt );

    if ( viseditor_ )
	viseditor_->setDisplayTransformation( nt );

    nearestpolygonmarker_->setDisplayTransformation( nt );

    if ( displaytransform_ ) displaytransform_->unRef();
    displaytransform_ = nt;
    if ( displaytransform_ ) displaytransform_->ref();
}


const mVisTrans* PolygonBodyDisplay::getDisplayTransformation() const
{ return displaytransform_; }


bool PolygonBodyDisplay::isPicking() const
{
    return !locked_ && empolygonsurf_ && polygonsurfeditor_ && isOn() &&
	   viseditor_ && viseditor_->isOn() && isSelected();
}


Coord3 PolygonBodyDisplay::disp2world( const Coord3& displaypos ) const
{
    Coord3 pos = displaypos;
    if ( pos.isDefined() )
    {
	if ( scene_ )
	    scene_->getTempZStretchTransform()->transformBack( pos );
	if ( displaytransform_ )
	    displaytransform_->transformBack( pos );
    }

    return pos;
}


void PolygonBodyDisplay::mouseCB( CallBacker* cb )
{
    if ( !empolygonsurf_ || !polygonsurfeditor_ || !isOn() || !viseditor_ ||
	 !viseditor_->isOn() || eventcatcher_->isHandled() || !isSelected() )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);

    polygonsurfeditor_->setSowingPivot(
				disp2world(viseditor_->sower().pivotPos()) );
    if ( viseditor_->sower().accept(eventinfo) )
	return;

    TrcKeyZSampling mouseplanecs;
    mouseplanecs.setEmpty();

    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	const int visid = eventinfo.pickedobjids[idx];
	visBase::DataObject* dataobj = visBase::DM().getObject( visid );

	mDynamicCastGet( visSurvey::PlaneDataDisplay*, plane, dataobj );
	if ( plane )
	{
	    mouseplanecs = plane->getTrcKeyZSampling();
	    break;
	}
    }

    if ( locked_ )
	return;

    Coord3 pos = disp2world( eventinfo.displaypickedpos );

    EM::PosID nearestpid0, nearestpid1, insertpid;
    const float zscale = scene_
	? scene_->getZScale() *scene_->getFixedZStretch()
	: SI().zScale();

    polygonsurfeditor_->getInteractionInfo( nearestpid0, nearestpid1, insertpid,
	pos, zscale );

    const int nearestpolygon =
	nearestpid0.isUdf() ? mUdf(int) : insertpid.getRowCol().row();

    if ( nearestpolygon_!=nearestpolygon )
    {
	nearestpolygon_ = nearestpolygon;
	updateNearestPolygonMarker();
    }

    if ( !pos.isDefined() )
	return;

    if ( eventinfo.type!=visBase::MouseClick )
	return;

    const EM::PosID pid = viseditor_->mouseClickDragger(eventinfo.pickedobjids);
    if ( !pid.isUdf() )
    {
	if ( OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	     !OD::altKeyboardButton(eventinfo.buttonstate_) &&
	     !OD::shiftKeyboardButton(eventinfo.buttonstate_) &&
	     OD::leftMouseButton(eventinfo.buttonstate_) )
	{
	    eventcatcher_->setHandled();
	    if ( !eventinfo.pressed )
	    {
		const int removepolygon = pid.getRowCol().row();
		const bool res = empolygonsurf_->geometry().nrKnots(
			pid.sectionID(),removepolygon)==1  ?
		    empolygonsurf_->geometry().removePolygon(
			    pid.sectionID(), removepolygon, true )
		    : empolygonsurf_->geometry().removeKnot(
			    pid.sectionID(), pid.subID(), true );

		polygonsurfeditor_->setLastClicked( EM::PosID::udf() );

		if ( res && !viseditor_->sower().moreToSow() )
		{
		    EM::EMM().undo(empolygonsurf_->id()).setUserInteractionEnd(
			EM::EMM().undo(empolygonsurf_->id()).currentEventID() );
		    touchAll( false );
		}
	    }
	}

	return;
    }

    if ( OD::altKeyboardButton(eventinfo.buttonstate_) ||
	 OD::ctrlKeyboardButton(eventinfo.buttonstate_) ||
	 !OD::leftMouseButton(eventinfo.buttonstate_) )
	return;

    if ( mouseplanecs.isEmpty() )
	return;

    const OD::Color& prefcol = empolygonsurf_->preferredColor();
    if ( viseditor_->sower().activate(prefcol, eventinfo) )
	return;

    if ( eventinfo.pressed || insertpid.isUdf() )
	return;

    if ( nearestpid0.isUdf() )
    {
	Coord3 editnormal(0,0,1);

	if ( mouseplanecs.defaultDir()==TrcKeyZSampling::Inl )
	    editnormal = Coord3( SI().transform(BinID(1,0))-
		    SI().transform(BinID(0,0)), 0 );
	else if ( mouseplanecs.defaultDir()==TrcKeyZSampling::Crl )
	    editnormal = Coord3( SI().transform(BinID(0,1))-
		    SI().transform(BinID(0,0)), 0 );

	if ( empolygonsurf_->geometry().insertPolygon( insertpid.sectionID(),
	       insertpid.getRowCol().row(), 0, pos, editnormal, true ) )
	{
	    polygonsurfeditor_->setLastClicked( insertpid );
	    if ( !viseditor_->sower().moreToSow() )
	    {
		EM::EMM().undo(empolygonsurf_->id()).setUserInteractionEnd(
		    EM::EMM().undo(empolygonsurf_->id()).currentEventID() );

		touchAll( false );
		polygonsurfeditor_->editpositionchange.trigger();
	    }
	}
    }
    else
    {
	if ( empolygonsurf_->geometry().insertKnot( insertpid.sectionID(),
		insertpid.subID(), pos, true ) )
	{
	    polygonsurfeditor_->setLastClicked( insertpid );
	    if ( !viseditor_->sower().moreToSow() )
	    {
		EM::EMM().undo(empolygonsurf_->id()).setUserInteractionEnd(
		    EM::EMM().undo(empolygonsurf_->id()).currentEventID() );
		polygonsurfeditor_->editpositionchange.trigger();
	    }
	}
    }

    eventcatcher_->setHandled();
}


void PolygonBodyDisplay::emChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);

    if ( cbdata.event==EM::EMObjectCallbackData::BurstAlert ||
	 cbdata.event==EM::EMObjectCallbackData::SectionChange ||
	 cbdata.event==EM::EMObjectCallbackData::PositionChange )
    {
	updateSingleColor();
	if ( cbdata.event==EM::EMObjectCallbackData::PositionChange )
	{
	     if ( cbdata.pid0.getRowCol().row()==nearestpolygon_ )
		updateNearestPolygonMarker();
	}
	else
	    updateNearestPolygonMarker();

	touchAll( false );
    }
    else if ( cbdata.event==EM::EMObjectCallbackData::PrefColorChange )
    {
	nontexturecol_ = empolygonsurf_->preferredColor();
	updateSingleColor();
    }
}


void PolygonBodyDisplay::updateNearestPolygonMarker()
{
    if ( mIsUdf(nearestpolygon_) || !isManipulatorShown() )
	nearestpolygonmarker_->turnOn( false );
    else
    {
	mDynamicCastGet( Geometry::PolygonSurface*, plgs,
	    empolygonsurf_->sectionGeometry(empolygonsurf_->sectionID(0)));

	const StepInterval<int> rowrg = plgs->rowRange();
	if ( rowrg.isUdf() || !rowrg.includes(nearestpolygon_,false) )
	{
	    nearestpolygonmarker_->turnOn( false );
	    return;
	}

	const StepInterval<int> colrg = plgs->colRange( nearestpolygon_ );
	if ( colrg.isUdf() || colrg.start==colrg.stop )
	{
	    nearestpolygonmarker_->turnOn( false );
	    return;
	}

	nearestpolygonmarker_->removeAllPrimitiveSets();
	nearestpolygonmarker_->getCoordinates()->setEmpty();

	int markeridx = 0;
	TypeSet<Coord3> knots;
	TypeSet<int> idxps;
	const float zfactor = scene_ ? scene_->getZScale() : SI().zScale();
	plgs->getCubicBezierCurve( nearestpolygon_, knots, zfactor  );
	for ( int idy=0; idy<knots.size(); idy++ )
	{
	    nearestpolygonmarker_->getCoordinates()->addPos(knots[idy]);
	    idxps.add( markeridx++ );
	}

	if ( markeridx>2 )
	{
	    nearestpolygonmarker_->getCoordinates()->addPos(knots[0]);
	    idxps.add( markeridx++ );
	}

	Geometry::IndexedPrimitiveSet* primitiveset =
	    Geometry::IndexedPrimitiveSet::create( false );
	primitiveset->ref();
	idxps.add( idxps[0] );
	primitiveset->append( idxps.arr(), idxps.size() );
	nearestpolygonmarker_->addPrimitiveSet( primitiveset );
	primitiveset->unRef();

	nearestpolygonmarker_->turnOn( true );
    }
}


void PolygonBodyDisplay::showManipulator( bool yn )
{
    if ( showmanipulator_==yn )
	return;

    showmanipulator_ = yn;
    updatePolygonDisplay();
    updateManipulator();
}


void PolygonBodyDisplay::updateManipulator()
{
    const bool show = showmanipulator_ &&
		      (arePolygonsDisplayed() || isBodyDisplayed() );

    if ( viseditor_ )
	viseditor_->turnOn( show );

    if ( nearestpolygonmarker_ )
	nearestpolygonmarker_->turnOn( show );

    //polygonsurfeditor_->editpositionchange.trigger();
}


bool  PolygonBodyDisplay::isManipulatorShown() const
{ return showmanipulator_; }


void PolygonBodyDisplay::setOnlyAtSectionsDisplay( bool yn )
{
    if ( !intersectiondisplay_ )
	return;

    if ( yn )
    {
	intersectiondisplay_->touch( false );
	reMakeIntersectionSurface();
    }

    intersectiondisplay_->turnOn( yn );
    intsurf_->turnOn( yn );
}


void PolygonBodyDisplay::reMakeIntersectionSurface()
{
    if ( !intersectiondisplay_ || !explicitintersections_ )
	return;

    const TypeSet<Geometry::ExplPlaneIntersection::PlaneIntersection>& pi =
	explicitintersections_->getPlaneIntersections();

    intsurf_->getCoordinates()->setEmpty();
    intsurf_->removeAllPrimitiveSets();

    Geometry::PrimitiveSet* ps =
	Geometry::IndexedPrimitiveSet::create( false );
    ps->ref();

    int ci = 0;
    for ( int idx=0; idx<pi.size(); idx++ )
    {
	const TypeSet<Coord3>& crds = pi[idx].knots_;
	const TypeSet<int>& conns = pi[idx].conns_;
	const int sz = crds.size();
	if ( sz<3 ) continue;

	const int startci = ci;
	Coord3 center(0,0,0);
	for ( int idy=0; idy<sz; idy++ )
	{
	    intsurf_->getCoordinates()->setPos( ci++, crds[idy] );
	    center += crds[idy];
	}
	center /= sz;
	intsurf_->getCoordinates()->setPos( ci, center );
	for ( int idy=0; idy<conns.size()/3; idy++ )
	{
	    ps->append( ci );
	    ps->append( conns[3*idy]+startci );
	    ps->append( conns[3*idy+1]+startci );
	}
	ci++;
    }

    if ( ps->size()> 3)
	intsurf_->addPrimitiveSet( ps );
    else
	ps->unRef();
}


bool PolygonBodyDisplay::displayedOnlyAtSections() const
{ return intersectiondisplay_ ? intersectiondisplay_->isOn() : false; }


void PolygonBodyDisplay::otherObjectsMoved(
	const ObjectSet<const SurveyObject>& objs, int whichobj )
{
    if ( !explicitintersections_ ) return;

    ObjectSet<const SurveyObject> usedobjects;
    TypeSet<int> planeids;

    for ( int idx=0; idx<objs.size(); idx++ )
    {
	mDynamicCastGet( const PlaneDataDisplay*, plane, objs[idx] );
	if ( !plane || !plane->isOn() )
	    continue;

	const TrcKeyZSampling cs = plane->getTrcKeyZSampling(true,true,-1);
	const BinID b00 = cs.hsamp_.start_, b11 = cs.hsamp_.stop_;
	BinID b01, b10;

	if ( plane->getOrientation()==OD::ZSlice )
	{
	    b01 = BinID( cs.hsamp_.start_.inl(), cs.hsamp_.stop_.crl() );
	    b10 = BinID( cs.hsamp_.stop_.inl(), cs.hsamp_.start_.crl() );
	}
	else
	{
	    b01 = b00;
	    b10 = b11;
	}

	const Coord3 c00( SI().transform(b00),cs.zsamp_.start );
	const Coord3 c01( SI().transform(b01),cs.zsamp_.stop );
	const Coord3 c11( SI().transform(b11),cs.zsamp_.stop );
	const Coord3 c10( SI().transform(b10),cs.zsamp_.start );

	const Coord3 normal = (c01-c00).cross(c10-c00).normalize();

	TypeSet<Coord3> positions;
	positions += c00;
	positions += c01;
	positions += c11;
	positions += c10;

	const int idy = intersectionobjs_.indexOf( objs[idx] );
	if ( idy==-1 )
	{
	    usedobjects += plane;
	    planeids += explicitintersections_->addPlane(normal,positions);
	}
	else
	{
	    usedobjects += plane;
	    explicitintersections_->setPlane(planeids_[idy], normal, positions);
	    planeids += planeids_[idy];

	    intersectionobjs_.removeSingle( idy );
	    planeids_.removeSingle( idy );
	}
    }

    for ( int idx=planeids_.size()-1; idx>=0; idx-- )
	explicitintersections_->removePlane( planeids_[idx] );

    intersectionobjs_ = usedobjects;
    planeids_ = planeids;

    if ( displayedOnlyAtSections() )
    {
	intersectiondisplay_->touch( false );
	reMakeIntersectionSurface();
    }
}


void PolygonBodyDisplay::setNewIntersectingPolygon( const Coord3& normal,
						    const Coord3& c00 )
{
    //To be used later.
    if ( !empolygonsurf_ )
	return;

    EM::PolygonBodyGeometry& geo = empolygonsurf_->geometry();
    const EM::SectionID sid = geo.sectionID( 0 );
    const int nrplgs = geo.nrPolygons(sid);
    if ( !nrplgs )
	return;

    const float zfactor = scene_ ? scene_->getZScale() : SI().zScale();
    const Coord3 scale( 1, 1, zfactor  );
    const Coord3 invscale( 1, 1, 1.0/zfactor  );
    const Plane3 newplane( normal, c00.scaleBy(scale), false );
    TypeSet<Coord3> intersections;

    for ( int plg=0; plg<nrplgs; plg++ )
    {
	TypeSet<Coord3> knots;
	if ( geo.sectionGeometry(sid) )
	    geo.sectionGeometry(sid)->getCubicBezierCurve(
						plg, knots, (float) scale.z );

	const int nrknots = knots.size();
	if ( !nrknots )
	    continue;

	const Coord3 plgnormal = geo.getPolygonNormal( sid, plg ).normalize();
	const Plane3 plgplane( plgnormal, knots[0].scaleBy(scale), false );
	Line3 intersectionline;
	if ( !newplane.intersectWith(plgplane,intersectionline) )
	    continue;

	for ( int knot=0; knot<nrknots; knot++ )
	{
	    const Coord3 p0 = knots[knot].scaleBy(scale);
	    const Coord3 p1 = knots[knot<nrknots-1 ? knot+1 : 0].scaleBy(scale);
	    const Line3 segment(p0, p1-p0);
	    double t, segt;
	    intersectionline.closestPoint(segment,t,segt);
	    if ( segt<=1+1e-3 && segt+1e-3>=0 )
		intersections += segment.getPoint(segt).scaleBy(invscale);
	}
    }

    for ( int pt=0; pt<intersections.size(); pt++ )
    {
	if ( pt==0 )
	    geo.insertPolygon( sid, nrplgs, 0, intersections[0], normal, true );
        else
	    geo.insertKnot( sid, RowCol(nrplgs,pt).toInt64(),
			    intersections[pt], true );
    }
}


void PolygonBodyDisplay::setPixelDensity( float dpi )
{
    VisualObjectImpl::setPixelDensity( dpi );

    if ( bodydisplay_ ) bodydisplay_->setPixelDensity( dpi );
    if ( polygondisplay_ ) polygondisplay_->setPixelDensity( dpi );
    if ( intersectiondisplay_ ) intersectiondisplay_->setPixelDensity( dpi );
    if ( nearestpolygonmarker_ ) nearestpolygonmarker_->setPixelDensity( dpi );
}


}; // namespace visSurvey
