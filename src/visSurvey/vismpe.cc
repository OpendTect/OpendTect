/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vismpe.cc,v 1.3 2005-01-20 08:41:32 kristofer Exp $";

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


mCreateFactoryEntry( visSurvey::MPEDisplay );

namespace visSurvey {

MPEDisplay::MPEDisplay()
    : VisualObjectImpl( true )
    , boxdragger( visBase::BoxDragger::create() )
    , rectangle( visBase::FaceSet::create() )
    , draggerrect( visBase::DataObjectGroup::create() )
    , dragger( visBase::DepthTabPlaneDragger::create() )
    , engine( MPE::engine() )
    , sceneeventcatcher( 0 )
{
    addChild( boxdragger->getInventorNode() );
    boxdragger->ref();
    boxdragger->finished.notify(
	mCB(this,MPEDisplay,boxDraggerFinishCB) );
    boxdragger->turnOn(false);

    const HorSampling& hs = SI().sampling(true).hrg;
    const Interval<float> survinlrg( hs.start.inl, hs.stop.inl );
    const Interval<float> survcrlrg( hs.start.crl, hs.stop.crl );
    const Interval<float> survzrg( SI().zRange(true).start,
	    			   SI().zRange(true).stop );

    boxdragger->setSpaceLimits( survinlrg, survcrlrg, survzrg );

    draggerrect->setSeparate(true);
    draggerrect->ref();

    rectangle->setVertexOrdering(1);
//  rectangle->setFaceType(1);
    rectangle->setMaterial(0);
    rectangle->getCoordinates()->addPos( Coord3( -1, -1, 0 ) );
    rectangle->getCoordinates()->addPos( Coord3( 1, -1, 0 ) );
    rectangle->getCoordinates()->addPos( Coord3( 1, 1, 0 ) );
    rectangle->getCoordinates()->addPos( Coord3( -1, 1, 0 ) );
    rectangle->setCoordIndex( 0, 0 );
    rectangle->setCoordIndex( 1, 1 );
    rectangle->setCoordIndex( 2, 2 );
    rectangle->setCoordIndex( 3, 3 );
    rectangle->setCoordIndex( 4, -1 );
    rectangle->setTextureCoords(visBase::TextureCoords::create());
    rectangle->getTextureCoords()->addCoord( Coord3(0,0,0) );
    rectangle->getTextureCoords()->addCoord( Coord3(1,0,0) );
    rectangle->getTextureCoords()->addCoord( Coord3(1,1,0) );
    rectangle->getTextureCoords()->addCoord( Coord3(0,1,0) );
    rectangle->setTextureCoordIndex( 0, 0 );
    rectangle->setTextureCoordIndex( 1, 1 );
    rectangle->setTextureCoordIndex( 2, 2 );
    rectangle->setTextureCoordIndex( 3, 3 );
    rectangle->setTextureCoordIndex( 4, -1 );
    draggerrect->addObject( rectangle );

    visBase::IndexedPolyLine* polyline = visBase::IndexedPolyLine::create();
    polyline->setCoordinates( rectangle->getCoordinates() );
    polyline->setCoordIndex( 0, 0 );
    polyline->setCoordIndex( 1, 1 );
    polyline->setCoordIndex( 2, 2 );
    polyline->setCoordIndex( 3, 3 );
    polyline->setCoordIndex( 4, 0 );
    polyline->setCoordIndex( 5, -1 );
    draggerrect->addObject( polyline );

    dragger->ref();
    addChild( dragger->getInventorNode() );
    dragger->setOwnShape( draggerrect->getInventorNode() );

    dragger->setDim(0);
    dragger->changed.notify( mCB(this,MPEDisplay,rectangleMovedCB) );
    dragger->started.notify( mCB(this,MPEDisplay,rectangleStartCB) );
    dragger->finished.notify( mCB(this,MPEDisplay,rectangleStopCB) );


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

    if ( dragger ) dragger->unRef();
    draggerrect->unRef();
    boxdragger->finished.remove( mCB(this,MPEDisplay,boxDraggerFinishCB) );
    boxdragger->unRef();
}


CubeSampling MPEDisplay::getBoxPosition() const
{
    Coord3 center = boxdragger->center();
    Coord3 width = boxdragger->width();

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
    const Coord3 center = dragger->center();
    const Coord3 size = dragger->size();
    
    const int dim = dragger->getDim();
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


/*
int MPEDisplay::getDim() const
{
    return dragger ? dragger->getDim() : 0;
}


void MPEDisplay::setDim( int dim )
{
    if ( dragger )
	dragger->setDim( dim );
}
*/

void MPEDisplay::setSceneEventCatcher( visBase::EventCatcher* nevc )
{
    if ( sceneeventcatcher )
    {
	sceneeventcatcher->eventhappened.remove(
					mCB(this,MPEDisplay,mouseClickCB) );
	sceneeventcatcher->unRef();
    }

    sceneeventcatcher = nevc;

    if ( sceneeventcatcher )
    {
	sceneeventcatcher->ref();
	sceneeventcatcher->eventhappened.notify(
	    mCB(this,MPEDisplay,mouseClickCB) );
    }
}


void MPEDisplay::boxDraggerFinishCB(CallBacker*)
{
    const CubeSampling newcube = getCubeSampling();
    if ( newcube!=engine.activeVolume() )
	engine.setActiveVolume( newcube );
}


void MPEDisplay::showManipulator( bool yn )
{
    boxdragger->turnOn(yn);
}


bool MPEDisplay::isManipulatorShown() const
{
    return boxdragger->isOn();
}


void MPEDisplay::rectangleMovedCB( CallBacker* cb )
{
    if ( isSelected() ) return;

    MPE::TrackPlane newplane;
    CubeSampling& planebox = newplane.boundingBox();
    getPlanePosition( planebox );

    if ( planebox==engine.trackPlane().boundingBox() )
	return;

    const CubeSampling& engineplane = engine.trackPlane().boundingBox();

    const int dim = dragger->getDim();
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
{ }


void MPEDisplay::rectangleStopCB( CallBacker* )
{
    EM::History& history = EM::EMM().history();
    const int currentevent = history.currentEventNr();
    if ( currentevent!=-1 )
	history.setLevel(currentevent,mEMHistoryUserInteractionLevel);
}


void MPEDisplay::mouseClickCB( CallBacker* cb )
{
    if ( sceneeventcatcher->isEventHandled() || !isOn() ) return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );
    if ( eventinfo.type!=visBase::MouseClick )
	return;

    if ( eventinfo.pickedobjids.indexOf(id())==-1 )
	return;

    if ( !eventinfo.mousebutton && 
	    eventinfo.shift && !eventinfo.ctrl && !eventinfo.alt )
    {
	if ( eventinfo.pressed )
	{
	    int dim = dragger->getDim();
	    if ( ++dim>=3 )
		dim = 0;

	    dragger->setDim( dim );
	    MPE::TrackPlane ntp;
	    getPlanePosition(ntp.boundingBox());
	    MPE::engine().setTrackPlane(ntp,false);
	}

	sceneeventcatcher->eventIsHandled();
    }
}


void MPEDisplay::updateBoxPosition( CallBacker* )
{
    NotifyStopper stop( dragger->changed );
    const CubeSampling cube = MPE::engine().activeVolume();
    const Coord3 newwidth( cube.hrg.stop.inl-cube.hrg.start.inl,
			   cube.hrg.stop.crl-cube.hrg.start.crl,
			   cube.zrg.stop-cube.zrg.start );
    boxdragger->setWidth( newwidth );
    dragger->setSize( newwidth );

    const Coord3 newcenter( (cube.hrg.stop.inl+cube.hrg.start.inl)/2,
			    (cube.hrg.stop.crl+cube.hrg.start.crl)/2,
			    cube.zrg.center());

    boxdragger->setCenter( newcenter );

    dragger->setSpaceLimits(
	    Interval<float>(cube.hrg.start.inl,cube.hrg.stop.inl),
	    Interval<float>(cube.hrg.start.crl,cube.hrg.stop.crl),
	    Interval<float>(cube.zrg.start, cube.zrg.stop) );
}


void MPEDisplay::updateDraggerPosition( CallBacker* )
{
    NotifyStopper stop( dragger->changed );
    const CubeSampling& cs = MPE::engine().trackPlane().boundingBox();
    if ( cs.hrg.start.inl==cs.hrg.stop.inl && dragger->getDim()!=0 )
	dragger->setDim(0);
    else if ( cs.hrg.start.crl==cs.hrg.stop.crl && dragger->getDim()!=1 )
	dragger->setDim(1);
    else if ( !cs.zrg.width() && dragger->getDim()!=2 ) dragger->setDim(2);

    const Coord3 newcenter((cs.hrg.stop.inl+cs.hrg.start.inl)/2,
			   (cs.hrg.stop.crl+cs.hrg.start.crl)/2,
			   cs.zrg.center() );
    if ( newcenter!=dragger->center() )
	dragger->setCenter( newcenter );
}


}; // namespace visSurvey
