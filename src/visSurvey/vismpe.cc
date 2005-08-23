/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vismpe.cc,v 1.19 2005-08-23 21:21:53 cvskris Exp $";

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
#include "vistexture3.h"
#include "vistexturecoords.h"
#include "attribsel.h"
#include "attribslice.h"
#include "iopar.h"
#include "visdataman.h"


mCreateFactoryEntry( visSurvey::MPEDisplay );

namespace visSurvey {

const char* MPEDisplay::draggerstr_ = "Dragger ID";
const char* MPEDisplay::transstr_   = "Transparency";

MPEDisplay::MPEDisplay()
    : VisualObjectImpl(true )
    , boxdragger_(visBase::BoxDragger::create())
    , rectangle_(visBase::FaceSet::create())
    , draggerrect_(visBase::DataObjectGroup::create())
    , dragger_(0)
    , engine_(MPE::engine())
    , sceneeventcatcher_(0)
    , as_(*new Attrib::SelSpec())
    , texture_(0)
    , manipulated_(false)
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
//  engine_.trackplanechange.notify( 
//				mCB(this,MPEDisplay,updateDraggerPosition) );
    setDraggerCenter( true );
    updateBoxPosition(0);
}


MPEDisplay::~MPEDisplay()
{
    engine_.activevolumechange.remove( mCB(this,MPEDisplay,updateBoxPosition) );
//  engine_.trackplanechange.remove( 
//				mCB(this,MPEDisplay,updateDraggerPosition) );

    setSceneEventCatcher( 0 );
    setDragger(0);

    draggerrect_->unRef();
    boxdragger_->finished.remove( mCB(this,MPEDisplay,boxDraggerFinishCB) );
    boxdragger_->unRef();
}


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


void MPEDisplay::updatePlaneColor()
{
    MPE::TrackPlane::TrackMode tm = engine_.trackPlane().getTrackMode();
    if ( tm == MPE::TrackPlane::ReTrack )
	rectangle_->getMaterial()->setColor( Color(0,255,0) );
    else if ( tm == MPE::TrackPlane::Erase )
	rectangle_->getMaterial()->setColor( Color(255,0,0) );
    else
	rectangle_->getMaterial()->setColor( Color::White );
}


void MPEDisplay::setCubeSampling( CubeSampling cs )
{
    cs.snapToSurvey();
    const Coord3 newwidth( cs.hrg.stop.inl-cs.hrg.start.inl,
			   cs.hrg.stop.crl-cs.hrg.start.crl,
			   cs.zrg.stop-cs.zrg.start );
    boxdragger_->setWidth( newwidth );
    
    const Coord3 newcenter( (cs.hrg.stop.inl+cs.hrg.start.inl)/2,
    			    (cs.hrg.stop.crl+cs.hrg.start.crl)/2,
    			    cs.zrg.center() );
    boxdragger_->setCenter( newcenter );
    setDraggerCenter( true );
}


CubeSampling MPEDisplay::getCubeSampling() const
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
    cube.snapToSurvey();
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

    return true;
}


void MPEDisplay::setSelSpec( const Attrib::SelSpec& as )
{ as_ = as; }


const Attrib::SelSpec* MPEDisplay::getSelSpec() const
{ return &as_; }


void MPEDisplay::updateTexture()
{
    const Attrib::SliceSet* sliceset = engine_.getAttribCache( as_ );
    if ( !sliceset )
    {
	setTexture( 0 );
	return;
    }

    if ( !texture_ )
	setTexture( visBase::Texture3::create() );

    const Array3D<float>* td = 0;
    if ( getCubeSampling() != sliceset->sampling )
	setCubeSampling( sliceset->sampling );
    td = sliceset->createArray( 0, 1, 2 );
    texture_->setData( td );
    delete td;
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
    center.x = SI().inlRange(true).snap( center.x );
    center.y = SI().crlRange(true).snap( center.y );
    center.z = SI().zRange(true).snap( center.z );

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
	    center.z += sign * SI().zStep();

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
    if ( newcube!=engine_.activeVolume() )
    {
	setTexture(0);
	engine_.setActiveVolume( newcube );
	manipulated_ = true;
    }

    setDraggerCenter( true );
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
    newplane.setTrackMode( engine_.trackPlane().getTrackMode() );

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
    if ( dim==2 && planebox.zrg.start==engineplane.zrg.start )
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
//	start = stop = engineplane.zrg.start + ( inc ? step : -step );
	newplane.setMotion( 0, 0, inc ? step : -step );
    }

    engine_.setTrackPlane( newplane, true );
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

    if ( eventinfo.mousebutton==visBase::EventInfo::leftMouseButton() && 
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
	    engine_.setTrackPlane( ntp, false );
	    updateTextureCoords();
	}

	sceneeventcatcher_->eventIsHandled();
    }
}


void MPEDisplay::updateBoxPosition( CallBacker* )
{
    NotifyStopper stop( dragger_->changed );
    const CubeSampling cube = engine_.activeVolume();
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


void MPEDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    as_.fillPar( par );
    
    CubeSampling cs = getCubeSampling();
    cs.fillPar( par );

    if ( dragger_ )
    {
	const int draggerid = dragger_->id();
	par.set( draggerstr_, draggerid );
	if ( saveids.indexOf(draggerid) == -1 ) saveids += draggerid;
    }

    par.set( transstr_, getDraggerTransparency() );
}


int MPEDisplay::usePar( const IOPar& par )
{
    const int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    CubeSampling cs;
    if ( !cs.usePar(par) )
	return -1;
    engine_.setActiveVolume( cs );

    int draggerid;
    if ( par.get(draggerstr_,draggerid) )
    {
	visBase::DataObject* dataobj = visBase::DM().getObject( draggerid );
	if ( !dataobj ) return 0;
	mDynamicCastGet(visBase::DepthTabPlaneDragger*,dr,dataobj)
	setDragger( dr );
    }
    else
	setDragger( visBase::DepthTabPlaneDragger::create() );

    float transparency = 0.5;
    par.get( transstr_, transparency );
    setDraggerTransparency( transparency );

    as_.usePar( par );

    return 1;
}


}; // namespace visSurvey
