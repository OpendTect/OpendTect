/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Jul 2003
 RCS:           $Id: visdepthtabplanedragger.cc,v 1.10 2005-02-07 12:45:40 nanne Exp $
________________________________________________________________________

-*/

#include "visdepthtabplanedragger.h"

#include "SoDepthTabPlaneDragger.h"
#include "vistransform.h"
#include "position.h"
#include "ranges.h"
#include "iopar.h"

#include <Inventor/nodes/SoSeparator.h>

mCreateFactoryEntry( visBase::DepthTabPlaneDragger );

namespace visBase
{

const char* DepthTabPlaneDragger::dimstr    = "Dimension";
const char* DepthTabPlaneDragger::sizestr   = "Size.";
const char* DepthTabPlaneDragger::centerstr = "Center.";

DepthTabPlaneDragger::DepthTabPlaneDragger()
    : VisualObjectImpl( false )
    , ownshape( 0 )
    , dragger( new SoDepthTabPlaneDragger )
    , rotation( 0 )
    , transform( 0 )
    , dim( 2 )
    , started( this )
    , motion( this )
    , changed( this )
    , finished( this )
{
    centers += center(); centers += center(); centers += center();
    sizes += size(); sizes += size(); sizes += size();

    addChild( dragger );

    setDim(dim);

    dragger->addStartCallback( DepthTabPlaneDragger::startCB, this );
    dragger->addMotionCallback( DepthTabPlaneDragger::motionCB, this );
    dragger->addFinishCallback( DepthTabPlaneDragger::finishCB, this );
    dragger->addValueChangedCallback(
	    		DepthTabPlaneDragger::valueChangedCB, this );

}


DepthTabPlaneDragger::~DepthTabPlaneDragger()
{
    if ( rotation ) rotation->unRef();
    if ( transform ) transform->unRef();

    dragger->removeStartCallback( DepthTabPlaneDragger::startCB, this );
    dragger->removeMotionCallback( DepthTabPlaneDragger::motionCB, this );
    dragger->removeFinishCallback( DepthTabPlaneDragger::finishCB, this );
    dragger->removeValueChangedCallback(
	    		DepthTabPlaneDragger::valueChangedCB, this );
}


void DepthTabPlaneDragger::setCenter( const Coord3& newcenter, bool alldims )
{
    const Coord3 dcenter = world2Dragger( newcenter, true );
    dragger->translation.setValue( SbVec3f(dcenter.x,dcenter.y,dcenter.z) );

    centers[dim] = newcenter;

    if ( alldims )
    {
	centers[0] = newcenter; centers[1] = newcenter; centers[2] = newcenter;
    }
}


Coord3 DepthTabPlaneDragger::center() const
{
    const SbVec3f res = dragger->translation.getValue();
    return dragger2World( Coord3(res[0],res[1],res[2]), true );
}


void DepthTabPlaneDragger::setSize( const Coord3& scale, bool alldims )
{
    const Coord3 dscale = world2Dragger(scale, false);
    dragger->scaleFactor.setValue(SbVec3f( dscale.x/2, dscale.y/2, dscale.z/2));

    sizes[dim] = scale;

    if ( alldims )
    {
	sizes[0] = scale; sizes[1] = scale; sizes[2] = scale;
    }
}


void DepthTabPlaneDragger::removeScaleTabs()
{
    dragger->setPart("scaleTabs", 0 );
}


Coord3 DepthTabPlaneDragger::size() const
{
    const SbVec3f res = dragger->scaleFactor.getValue();
    return dragger2World( Coord3(res[0]*2,res[1]*2,res[2]*2), false );
}


void DepthTabPlaneDragger::setDim( int newdim )
{
    centers[dim] = center();
    sizes[dim] = size();

    Interval<float> xlim, ylim, zlim;
    getSpaceLimits( xlim, ylim, zlim );
    Interval<float> xsizelim, ysizelim, zsizelim;
    getWidthLimits( xsizelim, ysizelim, zsizelim );

    if ( !newdim )
    {
	if ( !rotation )
	{
	    rotation = Transformation::create();
	    rotation->ref();

	    dragger->ref();
	    removeChild( dragger );
	    addChild( rotation->getInventorNode() );
	    addChild( dragger );
	    dragger->unref();
	}

	rotation->setRotation( Coord3(0,1,0), -M_PI/2 );
	rotation->setScale( Coord3( 1, 1, -1 ) );
    }
    else if ( newdim==1 )
    {
	if ( !rotation )
	{
	    rotation = Transformation::create();
	    rotation->ref();

	    dragger->ref();
	    removeChild( dragger );
	    addChild( rotation->getInventorNode() );
	    addChild( dragger );
	    dragger->unref();
	}

	rotation->setRotation( Coord3(1,0,0), M_PI/2 );
	rotation->setScale( Coord3( 1, 1, -1 ) );
    }
    else
    {
        if ( rotation ) rotation->reset();
    }

    dim = newdim;

    setSpaceLimits( xlim, ylim, zlim );
    setWidthLimits( xsizelim, ysizelim, zsizelim );
    setSize( sizes[dim], false );
    setCenter( centers[dim], false );
}


int DepthTabPlaneDragger::getDim() const
{
    return dim;
}


void DepthTabPlaneDragger::setSpaceLimits( const Interval<float>& x,
					   const Interval<float>& y,
					   const Interval<float>& z )
{
    const Coord3 start = world2Dragger( Coord3(x.start,y.start,z.start),true );
    const Coord3 stop = world2Dragger( Coord3(x.stop,y.stop,z.stop),true );
    dragger->minPos.setValue( start.x, start.y, start.z);
    dragger->maxPos.setValue( stop.x, stop.y, stop.z);
}


void DepthTabPlaneDragger::getSpaceLimits( Interval<float>& x,
					   Interval<float>& y,
					   Interval<float>& z ) const
{
    const SbVec3f dstart = dragger->minPos.getValue();
    const SbVec3f dstop = dragger->maxPos.getValue();
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
    dragger->minSize.setValue( start.x, start.y, start.z);
    dragger->maxSize.setValue( stop.x, stop.y, stop.z);
}


void DepthTabPlaneDragger::getWidthLimits( Interval<float>& x,
					   Interval<float>& y,
					   Interval<float>& z ) const
{
    const SbVec3f dstart = dragger->minSize.getValue();
    const SbVec3f dstop = dragger->maxSize.getValue();
    const Coord3 start = dragger2World( Coord3(dstart[0],dstart[1],dstart[2]),
	    				false );
    const Coord3 stop = dragger2World( Coord3(dstop[0],dstop[1],dstop[2]),
	    				false );
    x.start = start.x; x.stop = stop.x;
    y.start = start.y; y.stop = stop.y;
    z.start = start.z; z.stop = stop.z;
}


void DepthTabPlaneDragger::setDisplayTransformation( Transformation* nt )
{
    if ( transform==nt ) return;

    const Coord3 centerpos = center();
    const Coord3 savedsize = size();

    Interval<float> xlim, ylim, zlim;
    getSpaceLimits( xlim, ylim, zlim );
    Interval<float> xsizelim, ysizelim, zsizelim;
    getWidthLimits( xsizelim, ysizelim, zsizelim );


    if ( transform )
    {
	removeChild( transform->getInventorNode() );
	transform->unRef();
    }

    transform = nt;

    if ( transform )
    {
	insertChild(0, transform->getInventorNode() );
	transform->ref();
    }

    setSpaceLimits( xlim, ylim, zlim );
    setWidthLimits( xsizelim, ysizelim, zsizelim );
    setSize( savedsize );
    setCenter( centerpos );
}


Transformation* DepthTabPlaneDragger::getDisplayTransformation()
{
    return transform;
}


void DepthTabPlaneDragger::setOwnShape( SoNode* newnode )
{
    SoSeparator* newsep = dynamic_cast<SoSeparator*>(newnode);
    if ( !newsep )
    {
	newsep = new SoSeparator;
	newsep->addChild( newnode );
    }

    dragger->setPart("translator", newsep );
}


Coord3 DepthTabPlaneDragger::world2Dragger( const Coord3& world,
					    bool ispos ) const
{
    const Coord3 tpos = transform&&ispos ? transform->transform(world) : world;
    if ( !dim )
	return Coord3( tpos.z, tpos.y, tpos.x );
    if ( dim==1 )
	return Coord3( tpos.x, tpos.z, tpos.y );

    return tpos;
}


Coord3 DepthTabPlaneDragger::dragger2World( const Coord3& drag,
					    bool ispos ) const
{
    const Coord3 tpos = transform&&ispos ? transform->transformBack(drag) :drag;
    if ( !dim )
	return Coord3( tpos.z, tpos.y, tpos.x );
    if ( dim==1 )
	return Coord3( tpos.x, tpos.z, tpos.y );

    return tpos;
}


void DepthTabPlaneDragger::startCB( void* obj, SoDragger* )
{
    ((DepthTabPlaneDragger*)obj)->started.trigger();
}


void DepthTabPlaneDragger::motionCB( void* obj, SoDragger* )
{
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

    par.set( dimstr, getDim() );

    centers[dim] = center();
    for ( int idx=0; idx<3; idx++ )
    {
	BufferString str( centerstr );
	str += idx;
	par.set( str, centers[idx] );

	str = sizestr;
	str += idx;
	par.set( str, sizes[idx] );
    }
}


int DepthTabPlaneDragger::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    for ( int idx=0; idx<3; idx++ )
    {
	BufferString str( centerstr );
	str += idx;
	par.get( str, centers[idx] );

	str = sizestr;
	str += idx;
	par.get( str, sizes[idx] );
    }

    setSize( sizes[dim], false );
    setCenter( centers[dim], false );

    int dim_ = 0;
    par.get( dimstr, dim_ );
    setDim( dim_ );

    return 1;
}


}; // namespace visBase
