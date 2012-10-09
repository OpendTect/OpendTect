/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";


#include "visrandomtrackdragger.h"

#include "visevent.h"
#include "vistransform.h"

#include "SoRandomTrackLineDragger.h"
#include "Inventor/nodes/SoScale.h"

mCreateFactoryEntry( visBase::RandomTrackDragger );

namespace visBase
{

RandomTrackDragger::RandomTrackDragger()
    : VisualObjectImpl( true )
    , dragger_( new SoRandomTrackLineDragger )
    , motion(this)
    , rightclicknotifier_(this)
    , rightclickeventinfo_( 0 )
    , displaytrans_( 0 )
{
    addChild( dragger_ );
    dragger_->addMotionCallback( RandomTrackDragger::motionCB, this );

    setMaterial( 0 );
}


RandomTrackDragger::~RandomTrackDragger()
{
    dragger_->removeMotionCallback( RandomTrackDragger::motionCB, this );

    if ( displaytrans_ ) displaytrans_->unRef();
}


void RandomTrackDragger::setDisplayTransformation( const mVisTrans* nt )
{
    TypeSet<Coord> pos( nrKnots(), Coord3() );
    for ( int idx=nrKnots()-1; idx>=0; idx-- )
	pos[idx] = getKnot( idx );

    const Interval<float> zrg = getDepthRange();

    if ( displaytrans_ )
    {
	displaytrans_->unRef();
	displaytrans_ = 0;
    }

    displaytrans_ = nt;
    if ( displaytrans_ )
	displaytrans_->ref();

    for ( int idx=nrKnots()-1; idx>=0; idx-- )
	setKnot( idx, pos[idx] );

    setDepthRange( zrg );
}


const mVisTrans* RandomTrackDragger::getDisplayTransformation() const
{ return displaytrans_; }


void RandomTrackDragger::triggerRightClick( const EventInfo* eventinfo )
{
    rightclickeventinfo_ = eventinfo;
    rightclicknotifier_.trigger();
}


const TypeSet<int>* RandomTrackDragger::rightClickedPath() const
{ return rightclickeventinfo_ ? &rightclickeventinfo_->pickedobjids : 0; }


const EventInfo* RandomTrackDragger::rightClickedEventInfo() const
{ return rightclickeventinfo_; }


void RandomTrackDragger::motionCB( void* obj,
				   SoRandomTrackLineDragger* dragger )
{
    RandomTrackDragger* thisp = (RandomTrackDragger*)obj;
    thisp->motion.trigger(dragger->getMovingKnot(),thisp);
}


void RandomTrackDragger::setSize( const Coord3& nz )
{
    SoScale* size =
	    dynamic_cast<SoScale*>(dragger_->getPart(sKeyDraggerScale(), true));
    size->scaleFactor.setValue( nz.x, nz.y, nz.z );
    dragger_->knots.touch();
}


Coord3 RandomTrackDragger::getSize() const
{
    SoScale* size =
	   dynamic_cast<SoScale*>(dragger_->getPart( sKeyDraggerScale(), true));
    SbVec3f pos = size->scaleFactor.getValue();
    Coord3 res( pos[0], pos[1], pos[2] );
    return res;
}


int RandomTrackDragger::nrKnots() const
{ return dragger_->knots.getNum(); }


Coord RandomTrackDragger::getKnot( int idx ) const
{
    const SbVec2f sbvec = dragger_->knots[idx];
    Coord3 res( sbvec[0], sbvec[1], 0 );
    if ( displaytrans_ ) res = displaytrans_->transformBack( res );
    return res;
}


void RandomTrackDragger::setKnot( int idx, const Coord& knotpos )
{
    Coord3 pos( knotpos, 0 );
    if ( displaytrans_ ) displaytrans_->transform( pos );
    dragger_->knots.set1Value( idx, SbVec2f(pos.x, pos.y) );
}


void RandomTrackDragger::insertKnot( int idx, const Coord& knotpos )
{
    Coord3 pos( knotpos, 0 );
    if ( displaytrans_ ) displaytrans_->transform( pos );
    dragger_->knots.insertSpace( idx, 1 );
    dragger_->knots.set1Value( idx, SbVec2f(pos.x, pos.y) );
}


void RandomTrackDragger::removeKnot( int idx )
{
    if ( idx>=dragger_->knots.getNum() )
    {
	pErrMsg("Invalid index");
	return;
    }

    dragger_->knots.deleteValues( idx, 1 );
}


void RandomTrackDragger::setLimits( const Coord3& start, const Coord3& stop,
				    const Coord3& step )
{
    dragger_->xyzStart.setValue( SbVec3f(start.x,start.y,start.z) );
    dragger_->xyzStop.setValue( SbVec3f(stop.x,stop.y,stop.z) );
    dragger_->xyzStep.setValue( SbVec3f(step.x,step.y,step.z) );
}


void RandomTrackDragger::showFeedback( bool yn )
{
    dragger_->showFeedback( yn );
}


void RandomTrackDragger::setDepthRange( const Interval<float>& rg )
{
    Coord3 start( 0, 0, rg.start );
    Coord3 stop( 0, 0, rg.stop );

    if ( displaytrans_ )
    {
	start = displaytrans_->transform( start );
	stop = displaytrans_->transform( stop );
    }

    dragger_->z0 = start.z;
    dragger_->z1 = stop.z;
}


Interval<float> RandomTrackDragger::getDepthRange() const
{
    Coord3 start( 0, 0, dragger_->z0.getValue() );
    Coord3 stop( 0, 0, dragger_->z1.getValue() );

    if ( displaytrans_ )
    {
	start = displaytrans_->transformBack( start );
	stop = displaytrans_->transformBack( stop );
    }

    return Interval<float>( start.z, stop.z );
}


}; // namespace visBase
