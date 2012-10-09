/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jul 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "visdepthtabplanedragger.h"

#include "SoDepthTabPlaneDragger.h"
#include "vistransform.h"
#include "position.h"
#include "ranges.h"
#include "iopar.h"
#include "keyenum.h"
#include "mouseevent.h"

#include <Inventor/nodes/SoSeparator.h>

mCreateFactoryEntry( visBase::DepthTabPlaneDragger );

namespace visBase
{

const char* DepthTabPlaneDragger::dimstr()	{ return "Dimension"; }
const char* DepthTabPlaneDragger::sizestr()	{ return "Size."; }
const char* DepthTabPlaneDragger::centerstr()	{ return "Center."; }

DepthTabPlaneDragger::DepthTabPlaneDragger()
    : VisualObjectImpl( false )
    , dragger_( new SoDepthTabPlaneDragger )
    , rotation_( 0 )
    , transform_( 0 )
    , dim_( 2 )
    , started( this )
    , motion( this )
    , changed( this )
    , finished( this )
{
    centers_ += center(); centers_ += center(); centers_ += center();
    sizes_ += size(); sizes_ += size(); sizes_ += size();

    addChild( dragger_ );

    setDim(dim_);

    dragger_->addStartCallback( DepthTabPlaneDragger::startCB, this );
    dragger_->addMotionCallback( DepthTabPlaneDragger::motionCB, this );
    dragger_->addFinishCallback( DepthTabPlaneDragger::finishCB, this );
    dragger_->addValueChangedCallback(
	    		DepthTabPlaneDragger::valueChangedCB, this );

}


DepthTabPlaneDragger::~DepthTabPlaneDragger()
{
    if ( rotation_ ) rotation_->unRef();
    if ( transform_ ) transform_->unRef();

    dragger_->removeStartCallback( DepthTabPlaneDragger::startCB, this );
    dragger_->removeMotionCallback( DepthTabPlaneDragger::motionCB, this );
    dragger_->removeFinishCallback( DepthTabPlaneDragger::finishCB, this );
    dragger_->removeValueChangedCallback(
	    		DepthTabPlaneDragger::valueChangedCB, this );
}


void DepthTabPlaneDragger::setCenter( const Coord3& newcenter, bool alldims )
{
    const Coord3 dcenter = world2Dragger( newcenter, true );
    dragger_->translation.setValue( SbVec3f(dcenter.x,dcenter.y,dcenter.z) );

    centers_[dim_] = newcenter;

    if ( alldims )
    {
	centers_[0] = newcenter;
	centers_[1] = newcenter;
	centers_[2] = newcenter;
    }
}


Coord3 DepthTabPlaneDragger::center() const
{
    const SbVec3f res = dragger_->translation.getValue();
    return dragger2World( Coord3(res[0],res[1],res[2]), true );
}


void DepthTabPlaneDragger::setSize( const Coord3& scale, bool alldims )
{
    const float abs = scale.abs();
    Coord3 newscale( scale[0] ? scale[0] : abs,
    		     scale[1] ? scale[1] : abs,
    		     scale[2] ? scale[2] : abs );
    const Coord3 dscale = world2Dragger( newscale, false);
    dragger_->scaleFactor.setValue(SbVec3f( dscale.x/2, dscale.y/2,dscale.z/2));

    sizes_[dim_] = newscale;

    if ( alldims )
    {
	sizes_[0] = newscale; sizes_[1] = newscale; sizes_[2] = newscale;
    }
}


void DepthTabPlaneDragger::removeScaleTabs()
{
    dragger_->setPart("greenTabsSep", 0 );
}


Coord3 DepthTabPlaneDragger::size() const
{
    const SbVec3f res = dragger_->scaleFactor.getValue();
    return dragger2World( Coord3(res[0]*2,res[1]*2,res[2]*2), false );
}


void DepthTabPlaneDragger::setDim( int newdim )
{
    centers_[dim_] = center();
    sizes_[dim_] = size();

    Interval<float> xlim, ylim, zlim;
    getSpaceLimits( xlim, ylim, zlim );
    Interval<float> xsizelim, ysizelim, zsizelim;
    getWidthLimits( xsizelim, ysizelim, zsizelim );

    if ( !newdim )
    {
	if ( !rotation_ )
	{
	    rotation_ = Transformation::create();
	    rotation_->ref();

	    dragger_->ref();
	    removeChild( dragger_ );
	    addChild( rotation_->getInventorNode() );
	    addChild( dragger_ );
	    dragger_->unref();
	}

	rotation_->setRotation( Coord3(0,1,0), -M_PI/2 );
	rotation_->setScale( Coord3( 1, 1, -1 ) );
    }
    else if ( newdim==1 )
    {
	if ( !rotation_ )
	{
	    rotation_ = Transformation::create();
	    rotation_->ref();

	    dragger_->ref();
	    removeChild( dragger_ );
	    addChild( rotation_->getInventorNode() );
	    addChild( dragger_ );
	    dragger_->unref();
	}

	rotation_->setRotation( Coord3(1,0,0), M_PI/2 );
	rotation_->setScale( Coord3( 1, 1, -1 ) );
    }
    else
    {
        if ( rotation_ ) rotation_->reset();
    }

    dim_ = newdim;

    setSpaceLimits( xlim, ylim, zlim );
    setWidthLimits( xsizelim, ysizelim, zsizelim );
    NotifyStopper stopper( changed );
    setSize( sizes_[dim_], false );
    setCenter( centers_[dim_], false );
    stopper.restore();
}


int DepthTabPlaneDragger::getDim() const
{
    return dim_;
}


void DepthTabPlaneDragger::setSpaceLimits( const Interval<float>& x,
					   const Interval<float>& y,
					   const Interval<float>& z )
{
    const Coord3 start = world2Dragger( Coord3(x.start,y.start,z.start),true );
    const Coord3 stop = world2Dragger( Coord3(x.stop,y.stop,z.stop),true );
    dragger_->minPos.setValue( start.x, start.y, start.z);
    dragger_->maxPos.setValue( stop.x, stop.y, stop.z);
}


void DepthTabPlaneDragger::getSpaceLimits( Interval<float>& x,
					   Interval<float>& y,
					   Interval<float>& z ) const
{
    const SbVec3f dstart = dragger_->minPos.getValue();
    const SbVec3f dstop = dragger_->maxPos.getValue();
    const Coord3 start = dragger2World( Coord3(dstart[0],dstart[1],dstart[2]),
	    				true );
    const Coord3 stop = dragger2World( Coord3(dstop[0],dstop[1],dstop[2]),
	    				true );
    x.start = start.x; x.stop = stop.x;
    y.start = start.y; y.stop = stop.y;
    z.start = start.z; z.stop = stop.z;
}


void DepthTabPlaneDragger::setWidthLimits( const Interval<float>& x,
					   const Interval<float>& y,
					   const Interval<float>& z )
{
    const Coord3 start = world2Dragger( Coord3(x.start,y.start,z.start),true );
    const Coord3 stop = world2Dragger( Coord3(x.stop,y.stop,z.stop),true );
    dragger_->minSize.setValue( start.x, start.y, start.z);
    dragger_->maxSize.setValue( stop.x, stop.y, stop.z);
}


void DepthTabPlaneDragger::getWidthLimits( Interval<float>& x,
					   Interval<float>& y,
					   Interval<float>& z ) const
{
    const SbVec3f dstart = dragger_->minSize.getValue();
    const SbVec3f dstop = dragger_->maxSize.getValue();
    const Coord3 start = dragger2World( Coord3(dstart[0],dstart[1],dstart[2]),
	    				false );
    const Coord3 stop = dragger2World( Coord3(dstop[0],dstop[1],dstop[2]),
	    				false );
    x.start = start.x; x.stop = stop.x;
    y.start = start.y; y.stop = stop.y;
    z.start = start.z; z.stop = stop.z;
}


void DepthTabPlaneDragger::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transform_==nt ) return;

    const Coord3 centerpos = center();
    const Coord3 savedsize = size();

    Interval<float> xlim, ylim, zlim;
    getSpaceLimits( xlim, ylim, zlim );
    Interval<float> xsizelim, ysizelim, zsizelim;
    getWidthLimits( xsizelim, ysizelim, zsizelim );


    if ( transform_ )
    {
	removeChild( const_cast<mVisTrans*>(transform_)->getInventorNode() );
	transform_->unRef();
    }

    transform_ = nt;

    if ( transform_ )
    {
	insertChild(0, const_cast<mVisTrans*>(transform_)->getInventorNode() );
	transform_->ref();
    }

    setSpaceLimits( xlim, ylim, zlim );
    setWidthLimits( xsizelim, ysizelim, zsizelim );
    setSize( savedsize );
    setCenter( centerpos );
}


const mVisTrans* DepthTabPlaneDragger::getDisplayTransformation() const
{
    return transform_;
}


void DepthTabPlaneDragger::setOwnShape( SoNode* newnode )
{
    SoSeparator* newsep = dynamic_cast<SoSeparator*>(newnode);
    if ( !newsep )
    {
	newsep = new SoSeparator;
	newsep->addChild( newnode );
    }

    dragger_->setPart("translator", newsep );
}


void DepthTabPlaneDragger::setTransDragKeys( bool depth, int ns )
{
    const bool control = ns & OD::ControlButton;
    const bool shift = ns & OD::ShiftButton;
    const bool alt = ns & OD::AltButton;

    SoDepthTabPlaneDragger::Key key;

    if ( shift )
    {
	if ( control )
	    key = alt ? SoDepthTabPlaneDragger::SHIFTCONTROLALT
		    : SoDepthTabPlaneDragger::SHIFTCONTROL;
	else
	    key = alt ? SoDepthTabPlaneDragger::SHIFTALT
		    : SoDepthTabPlaneDragger::SHIFT;
    }
    else
    {
	if ( control )
	    key = alt ? SoDepthTabPlaneDragger::CONTROLALT
		    : SoDepthTabPlaneDragger::CONTROL;
	else
	    key = alt ? SoDepthTabPlaneDragger::ALT
		    : SoDepthTabPlaneDragger::NONE;
    }

    if ( depth ) dragger_->depthKey.setValue( key );
    else dragger_->translateKey.setValue( key );
}


int DepthTabPlaneDragger::getTransDragKeys(bool depth) const
{
    SoDepthTabPlaneDragger::Key key = depth
	? (SoDepthTabPlaneDragger::Key) dragger_->depthKey.getValue()
	: (SoDepthTabPlaneDragger::Key) dragger_->translateKey.getValue();

    int state = 0;

    if ( key==SoDepthTabPlaneDragger::SHIFTCONTROLALT ||
	key==SoDepthTabPlaneDragger::SHIFTCONTROL ||
	key==SoDepthTabPlaneDragger::SHIFTALT ||
	key==SoDepthTabPlaneDragger::SHIFT )
	    state |= OD::ShiftButton;

    if ( key==SoDepthTabPlaneDragger::SHIFTCONTROLALT ||
	key==SoDepthTabPlaneDragger::SHIFTCONTROL ||
	key==SoDepthTabPlaneDragger::CONTROLALT ||
	key==SoDepthTabPlaneDragger::CONTROL )
	    state |= OD::ControlButton;

    if ( key==SoDepthTabPlaneDragger::SHIFTCONTROLALT ||
	key==SoDepthTabPlaneDragger::SHIFTALT ||
	key==SoDepthTabPlaneDragger::CONTROLALT ||
	key==SoDepthTabPlaneDragger::ALT )
	    state |= OD::AltButton;

    return (OD::ButtonState) state;
}



Coord3 DepthTabPlaneDragger::world2Dragger( const Coord3& world,
					    bool ispos ) const
{
    const Coord3 tpos = transform_ && ispos
	? transform_->transform(world) : world;

    if ( !dim_ )
	return Coord3( tpos.z, tpos.y, tpos.x );
    if ( dim_==1 )
	return Coord3( tpos.x, tpos.z, tpos.y );

    return tpos;
}


Coord3 DepthTabPlaneDragger::dragger2World( const Coord3& drag,
					    bool ispos ) const
{
    const Coord3 tpos = transform_ && ispos
	? transform_->transformBack(drag) : drag;
    if ( !dim_ )
	return Coord3( tpos.z, tpos.y, tpos.x );
    if ( dim_==1 )
	return Coord3( tpos.x, tpos.z, tpos.y );

    return tpos;
}

static SbVec3f startcenter_;

void DepthTabPlaneDragger::startCB( void* obj, SoDragger* sod )
{
    startcenter_ = ((SoDepthTabPlaneDragger*)sod)->translation.getValue();
    ((DepthTabPlaneDragger*)obj)->started.trigger();
}


void DepthTabPlaneDragger::motionCB( void* obj, SoDragger* sod )
{
    const TabletInfo* ti = TabletInfo::currentState();
    if ( ti && ti->maxPostPressDist()<5 )
	((SoDepthTabPlaneDragger*)sod)->translation.setValue( startcenter_ );
    else
	((DepthTabPlaneDragger*)obj)->motion.trigger();
}


void DepthTabPlaneDragger::valueChangedCB( void* obj, SoDragger* d )
{
    ((DepthTabPlaneDragger*)obj)->changed.trigger();
}


void DepthTabPlaneDragger::finishCB( void* obj, SoDragger* )
{
    ((DepthTabPlaneDragger*)obj)->finished.trigger();
}


void DepthTabPlaneDragger::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    par.set( dimstr(), getDim() );

    const_cast<Coord3&>(centers_[dim_]) = center();
    for ( int idx=0; idx<3; idx++ )
    {
	BufferString str( centerstr() );
	str += idx;
	par.set( str, centers_[idx] );

	str = sizestr();
	str += idx;
	par.set( str, sizes_[idx] );
    }
}


int DepthTabPlaneDragger::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    for ( int idx=0; idx<3; idx++ )
    {
	BufferString str( centerstr() );
	str += idx;
	par.get( str, centers_[idx] );

	str = sizestr();
	str += idx;
	par.get( str, sizes_[idx] );
    }

    setSize( sizes_[dim_], false );
    setCenter( centers_[dim_], false );

    int dim = 0;
    par.get( dimstr(), dim );
    setDim( dim );

    return 1;
}


}; // namespace visBase
