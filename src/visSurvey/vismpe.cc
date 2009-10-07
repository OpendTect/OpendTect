/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vismpe.cc,v 1.79 2009-10-07 09:26:35 cvssatyaki Exp $";

#include "vismpe.h"

#include "visboxdragger.h"
#include "viscolortab.h"
#include "viscoord.h"
#include "visdataman.h"
#include "visdatagroup.h"
#include "visdepthtabplanedragger.h"
#include "visevent.h"
#include "visfaceset.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistexture3.h"
#include "vistexturecoords.h"
#include "vistransform.h"

#include "arrayndsubsel.h"
#include "attribsel.h"
#include "attribdatacubes.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "emmanager.h"
#include "iopar.h"
#include "keystrs.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "undo.h"


mCreateFactoryEntry( visSurvey::MPEDisplay );

namespace visSurvey {

MPEDisplay::MPEDisplay()
    : VisualObjectImpl(true)
    , boxdragger_(visBase::BoxDragger::create())
    , rectangle_(visBase::FaceSet::create())
    , draggerrect_(visBase::DataObjectGroup::create())
    , dragger_(0)
    , engine_(MPE::engine())
    , sceneeventcatcher_(0)
    , as_(*new Attrib::SelSpec())
    , texture_(0)
    , manipulated_(false)
    , movement( this )
    , boxDraggerStatusChange( this )
    , planeOrientationChange( this )
    , curtexturecs_(false)
    , curtextureas_(*new Attrib::SelSpec())
{
    addChild( boxdragger_->getInventorNode() );
    boxdragger_->ref();
    boxdragger_->finished.notify( mCB(this,MPEDisplay,boxDraggerFinishCB) );
    boxdragger_->setBoxTransparency( 0.7 );
    boxdragger_->turnOn( false );
    updateBoxSpace();

    draggerrect_->setSeparate( true );
    draggerrect_->ref();

    rectangle_->setVertexOrdering(
	    visBase::VertexShape::cCounterClockWiseVertexOrdering() );
//  rectangle_->setFaceType(1);
    rectangle_->setMaterial( visBase::Material::create() );
    rectangle_->getCoordinates()->addPos( Coord3(-1,-1,0) );
    rectangle_->getCoordinates()->addPos( Coord3(1,-1,0) );
    rectangle_->getCoordinates()->addPos( Coord3(1,1,0) );
    rectangle_->getCoordinates()->addPos( Coord3(-1,1,0) );
    rectangle_->setCoordIndex( 0, 0 );
    rectangle_->setCoordIndex( 1, 1 );
    rectangle_->setCoordIndex( 2, 2 );
    rectangle_->setCoordIndex( 3, 3 );
    rectangle_->setCoordIndex( 4, -1 );
    rectangle_->setTextureCoords( visBase::TextureCoords::create() );
    rectangle_->getTextureCoords()->addCoord( Coord3(0,0,0) );
    rectangle_->getTextureCoords()->addCoord( Coord3(1,0,0) );
    rectangle_->getTextureCoords()->addCoord( Coord3(1,1,0) );
    rectangle_->getTextureCoords()->addCoord( Coord3(0,1,0) );
    rectangle_->setTextureCoordIndex( 0, 0 );
    rectangle_->setTextureCoordIndex( 1, 1 );
    rectangle_->setTextureCoordIndex( 2, 2 );
    rectangle_->setTextureCoordIndex( 3, 3 );
    rectangle_->setTextureCoordIndex( 4, -1 );
    rectangle_->getMaterial()->setColor( Color::White() );
    draggerrect_->addObject( rectangle_ );

    visBase::IndexedPolyLine* polyline = visBase::IndexedPolyLine::create();
    polyline->setCoordinates( rectangle_->getCoordinates() );
    polyline->setCoordIndex( 0, 0 );
    polyline->setCoordIndex( 1, 1 );
    polyline->setCoordIndex( 2, 2 );
    polyline->setCoordIndex( 3, 3 );
    polyline->setCoordIndex( 4, 0 );
    polyline->setCoordIndex( 5, -1 );
    draggerrect_->addObject( polyline );

    setDragger( visBase::DepthTabPlaneDragger::create() );

    engine_.activevolumechange.notify( mCB(this,MPEDisplay,updateBoxPosition) );
    setDraggerCenter( true );
    updateBoxPosition(0);
    turnOn( true );
}


MPEDisplay::~MPEDisplay()
{
    engine_.activevolumechange.remove( mCB(this,MPEDisplay,updateBoxPosition) );

    setSceneEventCatcher( 0 );
    setDragger(0);

    draggerrect_->unRef();
    boxdragger_->finished.remove( mCB(this,MPEDisplay,boxDraggerFinishCB) );
    boxdragger_->unRef();

    delete &curtextureas_;
}


void MPEDisplay::setColTabMapperSetup( int attrib,
				       const ColTab::MapperSetup& ms,
				       TaskRunner* )
{
    if ( !texture_ ) return;
    visBase::VisColorTab& vt = texture_->getColorTab();
    const bool autoscalechange =
		ms.type_!=vt.colorMapper().setup_.type_ &&
		ms.type_!=ColTab::MapperSetup::Fixed;
    vt.colorMapper().setup_ = ms;
    if ( autoscalechange )
    {
	vt.autoscalechange.trigger();
	vt.colorMapper().setup_.triggerAutoscaleChange();
    }
    else
    {
	vt.rangechange.trigger();
	vt.autoscalechange.trigger();
	vt.colorMapper().setup_.triggerRangeChange();
    }
}


void MPEDisplay::setColTabSequence( int attrib, const ColTab::Sequence& seq,
				    TaskRunner* )
{
    if ( !texture_ ) return;

    visBase::VisColorTab& vt = texture_->getColorTab();
    vt.colorSeq().colors() = seq;
    vt.colorSeq().colorsChanged();
}


const ColTab::MapperSetup* MPEDisplay::getColTabMapperSetup( int attrib ) const
{ return texture_ ? &texture_->getColorTab().colorMapper().setup_ : 0; }

const ColTab::Sequence* MPEDisplay::getColTabSequence( int attrib ) const
{ return texture_ ? &texture_->getColorTab().colorSeq().colors() : 0; }

bool MPEDisplay::canSetColTabSequence() const
{ return true; }


void MPEDisplay::setDragger( visBase::DepthTabPlaneDragger* dr )
{
    if ( dragger_ )
    {
	dragger_->changed.remove( mCB(this,MPEDisplay,rectangleMovedCB) );
	dragger_->started.remove( mCB(this,MPEDisplay,rectangleStartCB) );
	dragger_->finished.remove( mCB(this,MPEDisplay,rectangleStopCB) );
	removeChild( dragger_->getInventorNode() );
	dragger_->unRef();
    }

    dragger_ = dr;
    if ( !dragger_ ) return;
        
    dragger_->ref();
    addChild( dragger_->getInventorNode() );
    dragger_->setOwnShape( draggerrect_->getInventorNode() );
    dragger_->setDim(0);
    dragger_->changed.notify( mCB(this,MPEDisplay,rectangleMovedCB) );
    dragger_->started.notify( mCB(this,MPEDisplay,rectangleStartCB) );
    dragger_->finished.notify( mCB(this,MPEDisplay,rectangleStopCB) );
}


CubeSampling MPEDisplay::getCubeSampling( int attrib ) const
{ return getBoxPosition(); }


CubeSampling MPEDisplay::getBoxPosition() const
{
    Coord3 center = boxdragger_->center();
    Coord3 width = boxdragger_->width();

    CubeSampling cube;
    cube.hrg.start = BinID( mNINT(center.x-width.x/2),
			    mNINT(center.y-width.y/2) );
    cube.hrg.stop = BinID( mNINT(center.x+width.x/2),
			   mNINT(center.y+width.y/2) );
    cube.hrg.step = BinID( SI().inlStep(), SI().crlStep() );
    cube.zrg.start = center.z - width.z / 2;
    cube.zrg.stop = center.z + width.z / 2;
    cube.zrg.step = SI().zStep();
    cube.hrg.snapToSurvey();
    SI().snapZ( cube.zrg.start, 0 );
    SI().snapZ( cube.zrg.stop, 0 );
    return cube;
}


bool MPEDisplay::getPlanePosition( CubeSampling& planebox ) const
{
    const Coord3 center = dragger_->center();
    const Coord3 size = dragger_->size();
    const int dim = dragger_->getDim();
    if ( !dim )
    {
	planebox.hrg.start.inl = SI().inlRange(true).snap(center.x);
	planebox.hrg.stop.inl = planebox.hrg.start.inl;

	planebox.hrg.start.crl = SI().crlRange(true).snap(center.y-size.y/2);
	planebox.hrg.stop.crl =  SI().crlRange(true).snap(center.y+size.y/2);

	planebox.zrg.start = SI().zRange(true).snap(center.z-size.z/2);
	planebox.zrg.stop = SI().zRange(true).snap(center.z+size.z/2);
    }
    else if ( dim==1 )
    {
	planebox.hrg.start.inl = SI().inlRange(true).snap(center.x-size.x/2);
	planebox.hrg.stop.inl =  SI().inlRange(true).snap(center.x+size.x/2);

	planebox.hrg.stop.crl = SI().crlRange(true).snap(center.y);
	planebox.hrg.start.crl = planebox.hrg.stop.crl;

	planebox.zrg.start = SI().zRange(true).snap(center.z-size.z/2);
	planebox.zrg.stop = SI().zRange(true).snap(center.z+size.z/2);
    }
    else 
    {
	planebox.hrg.start.inl = SI().inlRange(true).snap(center.x-size.x/2);
	planebox.hrg.stop.inl =  SI().inlRange(true).snap(center.x+size.x/2);

	planebox.hrg.start.crl = SI().crlRange(true).snap(center.y-size.y/2);
	planebox.hrg.stop.crl =  SI().crlRange(true).snap(center.y+size.y/2);

	planebox.zrg.stop = SI().zRange(true).snap(center.z);
	planebox.zrg.start = planebox.zrg.stop;
    }

    planebox.hrg.step = BinID( SI().inlStep(), SI().crlStep() );
    planebox.zrg.step = SI().zRange(true).step;

    return true;
}


void MPEDisplay::setSelSpec( int attrib, const Attrib::SelSpec& as )
{
    if ( attrib ) return;
    as_ = as;
}


const char* MPEDisplay::getSelSpecUserRef() const
{
    if ( as_.id()==Attrib::SelSpec::cNoAttrib() )
	return sKey::None;
    else if ( as_.id()==Attrib::SelSpec::cAttribNotSel() )
	return 0;

    return as_.userRef();
}


void MPEDisplay::updateTexture()
{
    const CubeSampling displaycs = engine_.activeVolume();
    if ( curtextureas_==as_ && curtexturecs_==displaycs )
    {
	texture_->turnOn( true );
	return;
    }

    RefMan<const Attrib::DataCubes> attrdata = engine_.getAttribCache( as_ );
    if ( !attrdata )
    {
	if ( texture_ ) texture_->turnOn( false );
	return;
    }

    if ( !texture_ )
	setTexture( visBase::Texture3::create() );

    const Array3D<float>& data( attrdata->getCube(0) );

    if ( displaycs != attrdata->cubeSampling() )
    {
	const CubeSampling attrcs = attrdata->cubeSampling();
	if ( !attrcs.includes( displaycs ) )
	{
	    texture_->turnOn( false );
	    return;
	}

	const StepInterval<int> inlrg( attrcs.hrg.inlRange() );
	const StepInterval<int> crlrg( attrcs.hrg.crlRange() );
	const Interval<int> dispinlrg( inlrg.getIndex(displaycs.hrg.start.inl),
				       inlrg.getIndex(displaycs.hrg.stop.inl) );
	const Interval<int> dispcrlrg( crlrg.getIndex(displaycs.hrg.start.crl),
				       crlrg.getIndex(displaycs.hrg.stop.crl) );

	const StepInterval<float>& zrg( displaycs.zrg );

	const Interval<int> dispzrg( attrcs.zrg.nearestIndex( zrg.start ),
				     attrcs.zrg.nearestIndex( zrg.stop ) );

	const Array3DSubSelection<float> array( dispinlrg.start,dispcrlrg.start,
			  dispzrg.start, dispinlrg.width()+1,
			  dispcrlrg.width()+1,
			  dispzrg.width()+1,
			  const_cast< Array3D<float>& >(data) );

	if ( !array.isOK() )
	{
	    texture_->turnOn( false );
	    return;
	}

	texture_->setData( &array );
    }
    else
	texture_->setData( &data );

    curtextureas_ = as_;
    curtexturecs_ = displaycs;

    texture_->turnOn( true );
}


void MPEDisplay::setTexture( visBase::Texture3* nt )
{
    if ( texture_ )
    {
	int oldindex = draggerrect_->getFirstIdx( (const DataObject*)texture_ );
	if ( oldindex!=-1 )
	    draggerrect_->removeObject( oldindex );
    }

    texture_ = nt;
    if ( texture_ )
	draggerrect_->insertObject( 0, (DataObject*)texture_ );

    updateTextureCoords();
}


void MPEDisplay::moveMPEPlane( int nr )
{
    if ( !dragger_ || !nr ) return;

    const int dim = dragger_->getDim();
    Coord3 center = dragger_->center();
    Coord3 width = boxdragger_->width();
    
    center.x = 0.5 * ( SI().inlRange(true).snap( center.x - width.x/2 ) +
	    	       SI().inlRange(true).snap( center.x + width.x/2 ) );
    center.y = 0.5 * ( SI().crlRange(true).snap( center.y - width.y/2 ) +
	    	       SI().crlRange(true).snap( center.y + width.y/2 ) );
    center.z = 0.5 * ( SI().zRange(true).snap( center.z - width.z/2 ) +
		       SI().zRange(true).snap( center.z + width.z/2 ) );

    Interval<float> sx, sy, sz;
    dragger_->getSpaceLimits( sx, sy, sz );
    
    const int nrsteps = abs(nr);
    const float sign = nr > 0 ? 1.001 : -1.001;
    // sign is slightly to big to avoid that it does not trigger a track
    
    sx.widen( 0.5*SI().inlStep(), true );
    sy.widen( 0.5*SI().crlStep(), true );
    sz.widen( 0.5*SI().zStep(), true );
    // assure that border lines of survey are reachable in spite of foregoing
    
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	if ( !dim )
	    center.x += sign * SI().inlStep();
	else if ( dim==1 )
	    center.y += sign * SI().crlStep();
	else
	    center.z += sign * SI().zStep();

	if ( !sx.includes(center.x) || !sy.includes(center.y) || 
	     !sz.includes(center.z) )
	    return;

	dragger_->setCenter( center, false );
    }
}


void MPEDisplay::setSceneEventCatcher( visBase::EventCatcher* nevc )
{
    if ( sceneeventcatcher_ )
    {
	sceneeventcatcher_->eventhappened.remove(
					mCB(this,MPEDisplay,mouseClickCB) );
	sceneeventcatcher_->unRef();
    }

    sceneeventcatcher_ = nevc;

    if ( sceneeventcatcher_ )
    {
	sceneeventcatcher_->ref();
	sceneeventcatcher_->eventhappened.notify(
	    mCB(this,MPEDisplay,mouseClickCB) );
    }
}


void MPEDisplay::boxDraggerFinishCB(CallBacker*)
{
    const CubeSampling newcube = getBoxPosition();
    if ( newcube!=engine_.activeVolume() )
    {
//	if ( texture_ ) texture_->turnOn(false);
	manipulated_ = true;
    }
}


void MPEDisplay::showBoxDragger( bool yn )
{
    if ( yn==boxdragger_->isOn() )
	return;

    boxdragger_->turnOn( yn );
    boxDraggerStatusChange.trigger();
}


void MPEDisplay::updateSeedOnlyPropagation( bool yn )
{
    engine_.updateSeedOnlyPropagation( yn );
}


void MPEDisplay::updateMPEActiveVolume()
{
    if ( manipulated_ )
    {
	const CubeSampling newcube = getBoxPosition();
	engine_.setActiveVolume( newcube );
	manipulated_ = false;
    }
}


void MPEDisplay::removeSelectionInPolygon( const Selector<Coord3>& selector,
	TaskRunner* tr )
{
    engine_.removeSelectionInPolygon( selector, tr );
    manipulated_ = true;
}


bool MPEDisplay::isOn() const
{
    return visBase::VisualObjectImpl::isOn() &&
	( isBoxDraggerShown() || isDraggerShown() );
}


bool MPEDisplay::isBoxDraggerShown() const
{ return boxdragger_->isOn(); }


void MPEDisplay::setDraggerTransparency( float transparency )
{
    rectangle_->getMaterial()->setTransparency( transparency );
}


float MPEDisplay::getDraggerTransparency() const
{
    return rectangle_->getMaterial()->getTransparency();
}


void MPEDisplay::showDragger( bool yn )
{
    if ( yn==isDraggerShown() )
	return;
    if ( yn )
	updateTexture();
    dragger_->turnOn( yn );
    movement.trigger();
    planeOrientationChange.trigger();
}


bool MPEDisplay::isDraggerShown() const
{ return dragger_->isOn(); }


void MPEDisplay::rectangleMovedCB( CallBacker* )
{
    if ( isSelected() ) return;

    while( true ) {
	MPE::TrackPlane newplane = engine_.trackPlane();
	CubeSampling& planebox = newplane.boundingBox();
	getPlanePosition( planebox );

	if ( planebox==engine_.trackPlane().boundingBox() )
	    return;

	updateTextureCoords();

	const CubeSampling& engineplane = engine_.trackPlane().boundingBox();
	const int dim = dragger_->getDim();
	if ( !dim && planebox.hrg.start.inl==engineplane.hrg.start.inl )
	    return;
	if ( dim==1 && planebox.hrg.start.crl==engineplane.hrg.start.crl )
	    return;
	if ( dim==2 && mIsEqual( planebox.zrg.start, engineplane.zrg.start, 
				 0.1*SI().zStep() ) )
	    return;

	if ( !dim )
	{
	    const bool inc = planebox.hrg.start.inl>engineplane.hrg.start.inl;
	    int& start = planebox.hrg.start.inl;
	    int& stop =  planebox.hrg.stop.inl;
	    const int step = SI().inlStep();
	    start = stop = engineplane.hrg.start.inl + ( inc ? step : -step );
	    newplane.setMotion( inc ? step : -step, 0, 0 );
	}
	else if ( dim==1 )
	{
	    const bool inc = planebox.hrg.start.crl>engineplane.hrg.start.crl;
	    int& start = planebox.hrg.start.crl;
	    int& stop =  planebox.hrg.stop.crl;
	    const int step = SI().crlStep();
	    start = stop = engineplane.hrg.start.crl + ( inc ? step : -step );
	    newplane.setMotion( 0, inc ? step : -step, 0 );
	}
	else 
	{
	    const bool inc = planebox.zrg.start>engineplane.zrg.start;
	    float& start = planebox.zrg.start;
	    float& stop =  planebox.zrg.stop;
	    const double step = SI().zStep();
	    start = stop = engineplane.zrg.start + ( inc ? step : -step );
	    newplane.setMotion( 0, 0, inc ? step : -step );
	}
	const MPE::TrackPlane::TrackMode trkmode = newplane.getTrackMode();
	engine_.setTrackPlane( newplane, trkmode==MPE::TrackPlane::Extend
				      || trkmode==MPE::TrackPlane::ReTrack
				      || trkmode==MPE::TrackPlane::Erase );
	movement.trigger();
	planeOrientationChange.trigger();
	}
}


void MPEDisplay::rectangleStartCB( CallBacker* )
{
    Undo& undo = EM::EMM().undo();
    lasteventnr_ = undo.currentEventID();
}


void MPEDisplay::rectangleStopCB( CallBacker* )
{
    Undo& undo = EM::EMM().undo();
    const int currentevent = undo.currentEventID();
    if ( currentevent!=lasteventnr_ )
	undo.setUserInteractionEnd(currentevent);
}


void MPEDisplay::setPlaneOrientation( int orient )
{
    dragger_->setDim( orient );
    
    if ( !isOn() ) return;

    updateTextureCoords();
    movement.trigger();
}


const int MPEDisplay::getPlaneOrientation() const
{ return dragger_->getDim(); }


void MPEDisplay::mouseClickCB( CallBacker* cb )
{
    if ( sceneeventcatcher_->isHandled() || !isOn() ) return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    if ( eventinfo.type != visBase::MouseClick )
	return;

    if ( OD::leftMouseButton(eventinfo.buttonstate_) &&
	 OD::shiftKeyboardButton(eventinfo.buttonstate_) &&
	 !OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	 !OD::altKeyboardButton(eventinfo.buttonstate_) &&
	 eventinfo.pickedobjids.indexOf(id())!=-1 )
    {
	if ( eventinfo.pressed )
	{
	    int dim = dragger_->getDim();
	    if ( ++dim>=3 )
		dim = 0;

	    dragger_->setDim( dim );
	    MPE::TrackPlane ntp = engine_.trackPlane();
	    getPlanePosition( ntp.boundingBox() );
	    engine_.setTrackPlane( ntp, false );
	    updateTextureCoords();
	    movement.trigger();
	    planeOrientationChange.trigger();
	}
	sceneeventcatcher_->setHandled();
    }
    else if ( OD::rightMouseButton( eventinfo.buttonstate_ ) &&
	      OD::shiftKeyboardButton(eventinfo.buttonstate_) &&
	      !OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	      !OD::altKeyboardButton(eventinfo.buttonstate_) &&
	      eventinfo.pickedobjids.indexOf(id())!=-1 && isDraggerShown() )
    {
	if ( eventinfo.pressed )
	{
	    const MPE::TrackPlane::TrackMode tm = 
					engine_.trackPlane().getTrackMode();
	    if ( tm==MPE::TrackPlane::Move )
		engine_.setTrackMode( MPE::TrackPlane::Extend );
	    else if ( tm==MPE::TrackPlane::Extend )
		engine_.setTrackMode( MPE::TrackPlane::ReTrack );
	    else if ( tm==MPE::TrackPlane::ReTrack )
		engine_.setTrackMode( MPE::TrackPlane::Erase );
	    else 
		engine_.setTrackMode( MPE::TrackPlane::Move );
	}
	sceneeventcatcher_->setHandled();
    }
    else if ( OD::leftMouseButton(eventinfo.buttonstate_) &&
	      !OD::shiftKeyboardButton(eventinfo.buttonstate_) &&
	      !OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	      !OD::altKeyboardButton(eventinfo.buttonstate_) &&
	      isBoxDraggerShown() &&
	      eventinfo.pickedobjids.indexOf(boxdragger_->id())==-1 )
    {
	showBoxDragger( false );
	sceneeventcatcher_->setHandled();
    }
}


void MPEDisplay::freezeBoxPosition( bool yn )
{
    if ( yn )
    {
    	engine_.activevolumechange.remove( 
				   mCB(this,MPEDisplay,updateBoxPosition) );
    }
    else
    {
    	engine_.activevolumechange.notifyIfNotNotified( 
				   mCB(this,MPEDisplay,updateBoxPosition) );
    }
}


void MPEDisplay::updateBoxPosition( CallBacker* )
{
    NotifyStopper stop( dragger_->changed );

    CubeSampling cube = engine_.activeVolume();
    Coord3 newwidth( cube.hrg.stop.inl-cube.hrg.start.inl,
		     cube.hrg.stop.crl-cube.hrg.start.crl,
		     cube.zrg.stop-cube.zrg.start );

    // Workaround for deadlock in COIN's polar_decomp() or Math::Sqrt(), which
    // occasionally occurs in case the box has one side of zero length.
    if ( cube.hrg.nrInl()==1 )
	newwidth.x = 0.1 * cube.hrg.step.inl;
    if ( cube.hrg.nrCrl()==1 )
	newwidth.y = 0.1 * cube.hrg.step.crl;
    if ( cube.nrZ()==1 )
	newwidth.z = 0.1 * cube.zrg.step;

    boxdragger_->setWidth( newwidth );
    dragger_->setSize( newwidth );

    const Coord3 newcenter( (cube.hrg.stop.inl+cube.hrg.start.inl)/2,
			    (cube.hrg.stop.crl+cube.hrg.start.crl)/2,
			    cube.zrg.center());

    boxdragger_->setCenter( newcenter );

    dragger_->setSpaceLimits(
	    Interval<float>(cube.hrg.start.inl,cube.hrg.stop.inl),
	    Interval<float>(cube.hrg.start.crl,cube.hrg.stop.crl),
	    Interval<float>(cube.zrg.start,cube.zrg.stop) );

    setDraggerCenter( true );
    if ( isDraggerShown() )
	updateTexture();

    updateTextureCoords();
    movement.trigger();
    planeOrientationChange.trigger();
}


void MPEDisplay::updateBoxSpace()
{
    const HorSampling& hs = SI().sampling(true).hrg;
    const Interval<float> survinlrg( hs.start.inl, hs.stop.inl );
    const Interval<float> survcrlrg( hs.start.crl, hs.stop.crl );
    const Interval<float> survzrg( SI().zRange(true).start,
	    			   SI().zRange(true).stop );

    boxdragger_->setSpaceLimits( survinlrg, survcrlrg, survzrg );
}


void MPEDisplay::updateDraggerPosition( CallBacker* cb )
{
    setDraggerCenter( false );
}


void MPEDisplay::setDraggerCenter( bool alldims )
{
    NotifyStopper stop( dragger_->changed );
    const CubeSampling& cs = engine_.trackPlane().boundingBox();
    if ( cs.hrg.start.inl==cs.hrg.stop.inl && dragger_->getDim()!=0 )
	dragger_->setDim(0);
    else if ( cs.hrg.start.crl==cs.hrg.stop.crl && dragger_->getDim()!=1 )
	dragger_->setDim(1);
    else if ( !cs.zrg.width() && dragger_->getDim()!=2 ) dragger_->setDim(2);

    const Coord3 newcenter((cs.hrg.stop.inl+cs.hrg.start.inl)/2,
			   (cs.hrg.stop.crl+cs.hrg.start.crl)/2,
			   cs.zrg.center());
    if ( newcenter != dragger_->center() )
	dragger_->setCenter( newcenter, alldims );
}


#define mGetRelCrd(val,dim) \
		(val-boxcenter[dim]+boxwidth[dim]/2)/boxwidth[dim]

void MPEDisplay::updateTextureCoords()
{
    if ( !dragger_ ) return;
    Coord3 boxcenter = boxdragger_->center();
    Coord3 boxwidth = boxdragger_->width();

    const Coord3 draggercenter = dragger_->center();
    const Coord3 draggerwidth = dragger_->size();
    const int dim = dragger_->getDim();

    const float relcoord = mGetRelCrd(draggercenter[dim],dim);
    const Interval<float> intv0( 
	    mGetRelCrd(draggercenter[0]-draggerwidth[0]/2,0),
	    mGetRelCrd(draggercenter[0]+draggerwidth[0]/2,0) );
    const Interval<float> intv1( 
	    mGetRelCrd(draggercenter[1]-draggerwidth[1]/2,1),
	    mGetRelCrd(draggercenter[1]+draggerwidth[1]/2,1) );
    const Interval<float> intv2( 
	    mGetRelCrd(draggercenter[2]-draggerwidth[2]/2,2),
	    mGetRelCrd(draggercenter[2]+draggerwidth[2]/2,2) );

    if ( !dim )
    {
	rectangle_->getTextureCoords()->setCoord( 0, 
				Coord3(relcoord,intv1.start,intv2.start) );
	rectangle_->getTextureCoords()->setCoord( 1, 
				Coord3(relcoord,intv1.start,intv2.stop) );
	rectangle_->getTextureCoords()->setCoord( 2, 
				Coord3(relcoord,intv1.stop,intv2.stop) );
	rectangle_->getTextureCoords()->setCoord( 3,
				Coord3(relcoord,intv1.stop,intv2.start) );
    }
    else if ( dim==1 )
    {
	rectangle_->getTextureCoords()->setCoord( 0, 
				Coord3(intv0.start,relcoord,intv2.start) );
	rectangle_->getTextureCoords()->setCoord( 1, 
				Coord3(intv0.stop,relcoord,intv2.start) );
	rectangle_->getTextureCoords()->setCoord( 2, 
				Coord3(intv0.stop,relcoord,intv2.stop) );
	rectangle_->getTextureCoords()->setCoord( 3, 
				Coord3(intv0.start,relcoord,intv2.stop) );
    }
    else
    {
	rectangle_->getTextureCoords()->setCoord( 0, 
				Coord3(intv0.start,intv1.start,relcoord) );
	rectangle_->getTextureCoords()->setCoord( 1, 
				Coord3(intv0.stop,intv1.start,relcoord) );
	rectangle_->getTextureCoords()->setCoord( 2, 
				Coord3(intv0.stop,intv1.stop,relcoord) );
	rectangle_->getTextureCoords()->setCoord( 3, 
				Coord3(intv0.start,intv1.stop,relcoord) );
    }
}


float MPEDisplay::calcDist( const Coord3& pos ) const
{
    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    const Coord3 xytpos = utm2display->transformBack( pos );
    const BinID binid = SI().transform( Coord(xytpos.x,xytpos.y) );

    CubeSampling cs; 
    if ( !getPlanePosition(cs) )
	return mUdf(float);

    BinID inlcrldist( 0, 0 );
    float zdiff = 0;

    inlcrldist.inl =
	binid.inl>=cs.hrg.start.inl && binid.inl<=cs.hrg.stop.inl
	     ? 0
	     : mMIN( abs(binid.inl-cs.hrg.start.inl),
		     abs( binid.inl-cs.hrg.stop.inl) );
    inlcrldist.crl =
        binid.crl>=cs.hrg.start.crl && binid.crl<=cs.hrg.stop.crl
             ? 0
	     : mMIN( abs(binid.crl-cs.hrg.start.crl),
		     abs( binid.crl-cs.hrg.stop.crl) );
    const float zfactor = scene_ ? scene_->getZScale() : SI().zScale();
    zdiff = cs.zrg.includes(xytpos.z)
	     ? 0
	     : mMIN(xytpos.z-cs.zrg.start,xytpos.z-cs.zrg.stop) *
	       zfactor  * scene_->getZStretch();

    const float inldist = SI().inlDistance();
    const float crldist = SI().crlDistance();
    float inldiff = inlcrldist.inl * inldist;
    float crldiff = inlcrldist.crl * crldist;

    return Math::Sqrt( inldiff*inldiff + crldiff*crldiff + zdiff*zdiff );
}

    
float MPEDisplay::maxDist() const
{
    const float zfactor = scene_ ? scene_->getZScale() : SI().zScale();
    float maxzdist = zfactor * scene_->getZStretch() * SI().zStep() / 2;
    return engine_.trackPlane().boundingBox().nrZ()==1 
					? maxzdist : SurveyObject::sDefMaxDist();
}


void MPEDisplay::getMousePosInfo( const visBase::EventInfo&, Coord3& pos,
				  BufferString& val, BufferString& info ) const
{
    val = "undef";
    info = "";

    const BinID bid( SI().transform(pos) );
    RefMan<const Attrib::DataCubes> attrdata = engine_.getAttribCache( as_ );
    if ( !attrdata )
	return;

    const CubeSampling& datacs = attrdata->cubeSampling();
    if ( !datacs.hrg.includes(bid) || !datacs.zrg.includes(pos.z) )
	return;
    const float fval = attrdata->getCube(0).get( datacs.inlIdx(bid.inl),
	    					 datacs.crlIdx(bid.crl),
						 datacs.zIdx(pos.z) );
    if ( !mIsUdf(fval) )
	val = fval;

    const int dim = dragger_->getDim();
    CubeSampling planecs;
    getPlanePosition( planecs );

    if ( !dim )
    {
	info = "Inline: ";
	info += planecs.hrg.start.inl;
    }
    else if ( dim==1 )
    {
	info = "Crossline: ";
	info += planecs.hrg.start.crl;
    }
    else
    {
	info = SI().zIsTime() ? "Time: " : "Depth: ";
	const float z = planecs.zrg.start;
	info += SI().zIsTime() ? mNINT( z * 1000) : z;
    }
}


void MPEDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    if ( texture_ )
    {
	par.set( sKeyTexture(), texture_->id() );
	if ( saveids.indexOf(texture_->id())==-1 )
	    saveids += texture_->id();
    }

    as_.fillPar( par );
    par.set( sKeyTransperancy(), getDraggerTransparency() );
    par.setYN( sKeyBoxShown(), isBoxDraggerShown() );
}


int MPEDisplay::usePar( const IOPar& par )
{
    const int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    int textureid;
    if ( par.get(sKeyTexture(),textureid) )
    {
	mDynamicCastGet( visBase::Texture3*, texture,
		         visBase::DM().getObject(textureid) );
	if ( texture ) setTexture( texture );
	else return 0;
    }

    float transparency = 0.5;
    par.get( sKeyTransperancy(), transparency );
    setDraggerTransparency( transparency );

    bool dispboxdragger = false;
    par.getYN( sKeyBoxShown(), dispboxdragger );

    if ( as_.usePar( par ) )
	updateTexture();

    turnOn( true );
    showBoxDragger( dispboxdragger );
    
    return 1;
}


}; // namespace visSurvey
