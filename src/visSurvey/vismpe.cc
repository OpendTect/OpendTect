/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vismpe.cc,v 1.5 2005-03-09 16:44:10 cvsnanne Exp $";

#include "vismpe.h"

#include "emhistory.h"
#include "emmanager.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "visboxdragger.h"
#include "viscoord.h"
#include "visdatagroup.h"
#include "visdepthtabplanedragger.h"
#include "visevent.h"
#include "visfaceset.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistexturecoords.h"
#include "attribsel.h"


mCreateFactoryEntry( visSurvey::MPEDisplay );

namespace visSurvey {

MPEDisplay::MPEDisplay()
    : VisualObjectImpl(true )
    , boxdragger_(visBase::BoxDragger::create())
    , rectangle_(visBase::FaceSet::create())
    , draggerrect_(visBase::DataObjectGroup::create())
    , dragger_(visBase::DepthTabPlaneDragger::create())
    , engine(MPE::engine())
    , sceneeventcatcher_(0)
    , as_(*new AttribSelSpec())
{
    addChild( boxdragger_->getInventorNode() );
    boxdragger_->ref();
    boxdragger_->finished.notify( mCB(this,MPEDisplay,boxDraggerFinishCB) );
    boxdragger_->turnOn(false);

    const HorSampling& hs = SI().sampling(true).hrg;
    const Interval<float> survinlrg( hs.start.inl, hs.stop.inl );
    const Interval<float> survcrlrg( hs.start.crl, hs.stop.crl );
    const Interval<float> survzrg( SI().zRange(true).start,
	    			   SI().zRange(true).stop );

    boxdragger_->setSpaceLimits( survinlrg, survcrlrg, survzrg );

    draggerrect_->setSeparate(true);
    draggerrect_->ref();

    rectangle_->setVertexOrdering(1);
//  rectangle_->setFaceType(1);
    rectangle_->setMaterial(visBase::Material::create());
    rectangle_->getCoordinates()->addPos( Coord3(-1,-1,0) );
    rectangle_->getCoordinates()->addPos( Coord3(1,-1,0) );
    rectangle_->getCoordinates()->addPos( Coord3(1,1,0) );
    rectangle_->getCoordinates()->addPos( Coord3(-1,1,0) );
    rectangle_->setCoordIndex( 0, 0 );
    rectangle_->setCoordIndex( 1, 1 );
    rectangle_->setCoordIndex( 2, 2 );
    rectangle_->setCoordIndex( 3, 3 );
    rectangle_->setCoordIndex( 4, -1 );
    rectangle_->setTextureCoords(visBase::TextureCoords::create());
    rectangle_->getTextureCoords()->addCoord( Coord3(0,0,0) );
    rectangle_->getTextureCoords()->addCoord( Coord3(1,0,0) );
    rectangle_->getTextureCoords()->addCoord( Coord3(1,1,0) );
    rectangle_->getTextureCoords()->addCoord( Coord3(0,1,0) );
    rectangle_->setTextureCoordIndex( 0, 0 );
    rectangle_->setTextureCoordIndex( 1, 1 );
    rectangle_->setTextureCoordIndex( 2, 2 );
    rectangle_->setTextureCoordIndex( 3, 3 );
    rectangle_->setTextureCoordIndex( 4, -1 );
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

    dragger_->ref();
    addChild( dragger_->getInventorNode() );
    dragger_->setOwnShape( draggerrect_->getInventorNode() );
    dragger_->setDim(0);
    dragger_->changed.notify( mCB(this,MPEDisplay,rectangleMovedCB) );
    dragger_->started.notify( mCB(this,MPEDisplay,rectangleStartCB) );
    dragger_->finished.notify( mCB(this,MPEDisplay,rectangleStopCB) );

    engine.activevolumechange.notify( mCB(this,MPEDisplay,updateBoxPosition) );
    engine.trackplanechange.notify( mCB(this,MPEDisplay,updateDraggerPosition));
    updateDraggerPosition(0);
    updateBoxPosition(0);
}


MPEDisplay::~MPEDisplay()
{
    engine.activevolumechange.remove( mCB(this,MPEDisplay,updateBoxPosition) );
    engine.trackplanechange.remove( mCB(this,MPEDisplay,updateDraggerPosition));

    setSceneEventCatcher( 0 );

    if ( dragger_ ) dragger_->unRef();
    draggerrect_->unRef();
    boxdragger_->finished.remove( mCB(this,MPEDisplay,boxDraggerFinishCB) );
    boxdragger_->unRef();
}


void MPEDisplay::setCubeSampling( CubeSampling cs )
{
    cs.snapToSurvey( true );
    const Coord3 newwidth( cs.hrg.stop.inl-cs.hrg.start.inl,
			   cs.hrg.stop.crl-cs.hrg.start.crl,
			   cs.zrg.stop-cs.zrg.start );
    boxdragger_->setWidth( newwidth );
    
    const Coord3 newcenter( (cs.hrg.stop.inl+cs.hrg.start.inl)/2,
    			    (cs.hrg.stop.crl+cs.hrg.start.crl)/2,
    			    cs.zrg.center() );
    boxdragger_->setCenter( newcenter );
}


CubeSampling MPEDisplay::getCubeSampling() const
{ return getBoxPosition(); }


CubeSampling MPEDisplay::getBoxPosition() const
{
    Coord3 center = boxdragger_->center();
    Coord3 width = boxdragger_->width();

    CubeSampling cube;
    cube.hrg.start = BinID( mNINT( center.x - width.x / 2 ),
				     mNINT( center.y - width.y / 2 ) );
    cube.hrg.stop = BinID( mNINT( center.x + width.x / 2 ),
				    mNINT( center.y + width.y / 2 ) );
    cube.hrg.step = BinID( SI().inlStep(), SI().crlStep() );
    cube.zrg.start = center.z - width.z / 2;
    cube.zrg.stop = center.z + width.z / 2;
    cube.zrg.step = SI().zRange().step;
    cube.snapToSurvey(true);
    return cube;
}


bool MPEDisplay::getPlanePosition( CubeSampling& planebox ) const
{
    const Coord3 center = dragger_->center();
    const Coord3 size = dragger_->size();
    const int dim = dragger_->getDim();
    if ( !dim )
    {
	planebox.hrg.start.inl = SI().inlRange().snap(center.x);
	planebox.hrg.stop.inl = planebox.hrg.start.inl;

	planebox.hrg.start.crl = SI().crlRange().snap(center.y-size.y/2);
	planebox.hrg.stop.crl =  SI().crlRange().snap(center.y+size.y/2);

	planebox.zrg.start = SI().zRange().snap(center.z-size.z/2);
	planebox.zrg.stop = SI().zRange().snap(center.z+size.z/2);
    }
    else if ( dim==1 )
    {
	planebox.hrg.start.inl = SI().inlRange().snap(center.x-size.x/2);
	planebox.hrg.stop.inl =  SI().inlRange().snap(center.x+size.x/2);

	planebox.hrg.stop.crl = SI().crlRange().snap(center.y);
	planebox.hrg.start.crl = planebox.hrg.stop.crl;

	planebox.zrg.start = SI().zRange().snap(center.z-size.z/2);
	planebox.zrg.stop = SI().zRange().snap(center.z+size.z/2);
    }
    else 
    {
	planebox.hrg.start.inl = SI().inlRange().snap(center.x-size.x/2);
	planebox.hrg.stop.inl =  SI().inlRange().snap(center.x+size.x/2);

	planebox.hrg.start.crl = SI().crlRange().snap(center.y-size.y/2);
	planebox.hrg.stop.crl =  SI().crlRange().snap(center.y+size.y/2);

	planebox.zrg.stop = SI().zRange().snap(center.z);
	planebox.zrg.start = planebox.zrg.stop;
    }

    return true;
}


void MPEDisplay::setSelSpec( const AttribSelSpec& as )
{ as_ = as; }


const AttribSelSpec* MPEDisplay::getSelSpec() const
{ return &as_; }


void MPEDisplay::updateTexture()
{
}


void MPEDisplay::moveMPEPlane( int nr )
{
    if ( !dragger_ || !nr ) return;

    const int dim = dragger_->getDim();
    Coord3 center = dragger_->center();
    center.x = SI().inlRange().snap( center.x );
    center.y = SI().crlRange().snap( center.y );
    center.z = SI().zRange().snap( center.z );

    Interval<float> sx, sy, sz;
    dragger_->getSpaceLimits( sx, sy, sz );

    const int nrsteps = abs(nr);
    const float sign = nr > 0 ? 1.001 : -1.001;
    // sign is slightly to big to avoid that it does not trigger a track
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	if ( !dim )
	    center.x += sign * SI().inlStep();
	else if ( dim==1 )
	    center.y += sign * SI().crlStep();
	else
	    center.z += sign * SI().zRange().step;

	if ( !sx.includes(center.x) || !sy.includes(center.y) || 
	     !sz.includes(center.z) )
	    return;

	dragger_->setCenter( center, false );
    }
}


/*
int MPEDisplay::getDim() const
{
    return dragger ? dragger_->getDim() : 0;
}


void MPEDisplay::setDim( int dim )
{
    if ( dragger )
	dragger_->setDim( dim );
}
*/

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
    if ( newcube!=engine.activeVolume() )
	engine.setActiveVolume( newcube );
}


void MPEDisplay::showManipulator( bool yn )
{
    boxdragger_->turnOn( yn );
}


void MPEDisplay::setDraggerTransparency( float transparency )
{
    rectangle_->getMaterial()->setTransparency( transparency );
}


float MPEDisplay::getDraggerTransparency() const
{
    return rectangle_->getMaterial()->getTransparency();
}


void MPEDisplay::showDragger( bool yn )
{ dragger_->turnOn( yn ); }


bool MPEDisplay::isDraggerShown() const
{ return dragger_->isOn(); }


void MPEDisplay::rectangleMovedCB( CallBacker* )
{
    if ( isSelected() ) return;

    MPE::TrackPlane newplane;
    CubeSampling& planebox = newplane.boundingBox();
    getPlanePosition( planebox );

    if ( planebox==engine.trackPlane().boundingBox() )
	return;

    const CubeSampling& engineplane = engine.trackPlane().boundingBox();

    const int dim = dragger_->getDim();
    if ( !dim && planebox.hrg.start.inl==engineplane.hrg.start.inl )
	return;
    if ( dim==1 && planebox.hrg.start.crl==engineplane.hrg.start.crl )
	return;
    if ( dim==2 && planebox.zrg.start==engineplane.zrg.start )
	return;

    if ( !dim )
    {
	const bool inc = planebox.hrg.start.inl>engineplane.hrg.start.inl;
	int& start = planebox.hrg.start.inl;
	int& stop =  planebox.hrg.stop.inl;
	const int step = SI().inlRange().step;
	start = stop = engineplane.hrg.start.inl + ( inc ? step : -step );
	newplane.setMotion( inc ? step : -step, 0, 0 );
    }
    else if ( dim==1 )
    {
	const bool inc = planebox.hrg.start.crl>engineplane.hrg.start.crl;
	int& start = planebox.hrg.start.crl;
	int& stop =  planebox.hrg.stop.crl;
	const int step = SI().crlRange().step;
	start = stop = engineplane.hrg.start.crl + ( inc ? step : -step );
	newplane.setMotion( 0, inc ? step : -step, 0 );
    }
    else 
    {
	const bool inc = planebox.zrg.start>engineplane.zrg.start;
	float& start = planebox.zrg.start;
	float& stop =  planebox.zrg.stop;
	const double step = SI().zRange().step;
	start = stop = engineplane.zrg.start + ( inc ? step : -step );
	newplane.setMotion( 0, 0, inc ? step : -step );
    }

    engine.setTrackPlane(newplane,true);
}


void MPEDisplay::rectangleStartCB( CallBacker* )
{}


void MPEDisplay::rectangleStopCB( CallBacker* )
{
    EM::History& history = EM::EMM().history();
    const int currentevent = history.currentEventNr();
    if ( currentevent!=-1 )
	history.setLevel(currentevent,mEMHistoryUserInteractionLevel);
}


void MPEDisplay::mouseClickCB( CallBacker* cb )
{
    if ( sceneeventcatcher_->isEventHandled() || !isOn() ) return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    if ( eventinfo.type != visBase::MouseClick )
	return;

    if ( eventinfo.pickedobjids.indexOf(id())==-1 )
	return;

    if ( !eventinfo.mousebutton && 
	    eventinfo.shift && !eventinfo.ctrl && !eventinfo.alt )
    {
	if ( eventinfo.pressed )
	{
	    int dim = dragger_->getDim();
	    if ( ++dim>=3 )
		dim = 0;

	    dragger_->setDim( dim );
	    MPE::TrackPlane ntp;
	    getPlanePosition(ntp.boundingBox());
	    MPE::engine().setTrackPlane( ntp, false );
	}

	sceneeventcatcher_->eventIsHandled();
    }
}


void MPEDisplay::updateBoxPosition( CallBacker* )
{
    NotifyStopper stop( dragger_->changed );
    const CubeSampling cube = MPE::engine().activeVolume();
    const Coord3 newwidth( cube.hrg.stop.inl-cube.hrg.start.inl,
			   cube.hrg.stop.crl-cube.hrg.start.crl,
			   cube.zrg.stop-cube.zrg.start );
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
}


void MPEDisplay::updateDraggerPosition( CallBacker* )
{
    NotifyStopper stop( dragger_->changed );
    const CubeSampling& cs = MPE::engine().trackPlane().boundingBox();
    if ( cs.hrg.start.inl==cs.hrg.stop.inl && dragger_->getDim()!=0 )
	dragger_->setDim(0);
    else if ( cs.hrg.start.crl==cs.hrg.stop.crl && dragger_->getDim()!=1 )
	dragger_->setDim(1);
    else if ( !cs.zrg.width() && dragger_->getDim()!=2 ) dragger_->setDim(2);

    const Coord3 newcenter((cs.hrg.stop.inl+cs.hrg.start.inl)/2,
			   (cs.hrg.stop.crl+cs.hrg.start.crl)/2,
			   cs.zrg.center() );
    if ( newcenter!=dragger_->center() )
	dragger_->setCenter( newcenter );
}


}; // namespace visSurvey
