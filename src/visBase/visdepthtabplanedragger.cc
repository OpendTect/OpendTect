/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: visdepthtabplanedragger.cc,v 1.3 2003-11-07 12:22:02 bert Exp $";

#include "visdepthtabplanedragger.h"

#include "SoDepthTabPlaneDragger.h"
#include "Inventor/nodes/SoSeparator.h"
#include "vistransform.h"
#include "position.h"
#include "ranges.h"

mCreateFactoryEntry( visBase::DepthTabPlaneDragger );

visBase::DepthTabPlaneDragger::DepthTabPlaneDragger()
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

    dragger->addStartCallback( visBase::DepthTabPlaneDragger::startCB, this );
    dragger->addMotionCallback( visBase::DepthTabPlaneDragger::motionCB, this );
    dragger->addFinishCallback( visBase::DepthTabPlaneDragger::finishCB, this );
    dragger->addValueChangedCallback(
	    		visBase::DepthTabPlaneDragger::valueChangedCB, this );

}


visBase::DepthTabPlaneDragger::~DepthTabPlaneDragger()
{
    if ( rotation ) rotation->unRef();
    if ( transform ) transform->unRef();

    dragger->removeStartCallback(visBase::DepthTabPlaneDragger::startCB, this );
    dragger->removeMotionCallback(visBase::DepthTabPlaneDragger::motionCB,this);
    dragger->removeFinishCallback(visBase::DepthTabPlaneDragger::finishCB,this);
    dragger->removeValueChangedCallback(
	    		visBase::DepthTabPlaneDragger::valueChangedCB, this );
}


void visBase::DepthTabPlaneDragger::setCenter( const Coord3& center,
					       bool alldims )
{
    const Coord3 dcenter = world2Dragger( center, true );
    dragger->translation.setValue( SbVec3f(dcenter.x,dcenter.y,dcenter.z) );

    centers[dim] = center;

    if ( alldims )
    {
	centers[0] = center; centers[1] = center; centers[2] = center;
    }
}


Coord3 visBase::DepthTabPlaneDragger::center() const
{
    const SbVec3f res = dragger->translation.getValue();
    return dragger2World( Coord3(res[0],res[1],res[2]), true );
}


void visBase::DepthTabPlaneDragger::setSize( const Coord3& scale, bool alldims )
{
    const Coord3 dscale = world2Dragger(scale, false);
    dragger->scaleFactor.setValue(SbVec3f( dscale.x/2, dscale.y/2, dscale.z/2));

    sizes[dim] = scale;

    if ( alldims )
    {
	sizes[0] = scale; sizes[1] = scale; sizes[2] = scale;
    }
}


Coord3 visBase::DepthTabPlaneDragger::size() const
{
    const SbVec3f res = dragger->scaleFactor.getValue();
    return dragger2World( Coord3(res[0]*2,res[1]*2,res[2]*2), false );
}


void visBase::DepthTabPlaneDragger::setDim( int newdim )
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
	    rotation = visBase::Transformation::create();
	    rotation->ref();

	    dragger->ref();
	    removeChild( dragger );
	    addChild( rotation->getData() );
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
	    rotation = visBase::Transformation::create();
	    rotation->ref();

	    dragger->ref();
	    removeChild( dragger );
	    addChild( rotation->getData() );
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


int visBase::DepthTabPlaneDragger::getDim() const
{
    return dim;
}


void visBase::DepthTabPlaneDragger::setSpaceLimits( const Interval<float>& x,
						    const Interval<float>& y,
						    const Interval<float>& z )
{
    const Coord3 start = world2Dragger( Coord3(x.start,y.start,z.start),true );
    const Coord3 stop = world2Dragger( Coord3(x.stop,y.stop,z.stop),true );
    dragger->minPos.setValue( start.x, start.y, start.z);
    dragger->maxPos.setValue( stop.x, stop.y, stop.z);
}


void visBase::DepthTabPlaneDragger::getSpaceLimits( Interval<float>& x,
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


void visBase::DepthTabPlaneDragger::setWidthLimits( const Interval<float>& x,
						    const Interval<float>& y,
						    const Interval<float>& z )
{
    const Coord3 start = world2Dragger( Coord3(x.start,y.start,z.start),true );
    const Coord3 stop = world2Dragger( Coord3(x.stop,y.stop,z.stop),true );
    dragger->minSize.setValue( start.x, start.y, start.z);
    dragger->maxSize.setValue( stop.x, stop.y, stop.z);
}


void visBase::DepthTabPlaneDragger::getWidthLimits( Interval<float>& x,
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


void visBase::DepthTabPlaneDragger::setTransformation( Transformation* nt )
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
	removeChild( transform->getData() );
	transform->unRef();
    }

    transform = nt;

    if ( transform )
    {
	insertChild(0, transform->getData() );
	transform->ref();
    }

    setSpaceLimits( xlim, ylim, zlim );
    setWidthLimits( xsizelim, ysizelim, zsizelim );
    setSize( savedsize );
    setCenter( centerpos );
}


visBase::Transformation* visBase::DepthTabPlaneDragger::getTransformation()
{
    return transform;
}


void visBase::DepthTabPlaneDragger::setOwnShape(SoNode* newnode)
{
    SoSeparator* newsep = dynamic_cast<SoSeparator*>(newnode);
    if ( !newsep )
    {
	newsep = new SoSeparator;
	newsep->addChild( newnode );
    }

    dragger->setPart("translator", newsep );
}


Coord3 visBase::DepthTabPlaneDragger::world2Dragger( const Coord3& world,
						     bool ispos ) const
{
    const Coord3 tpos = transform&&ispos ? transform->transform(world) : world;
    if ( !dim )
	return Coord3( tpos.z, tpos.y, tpos.x );
    if ( dim==1 )
	return Coord3( tpos.x, tpos.z, tpos.y );

    return tpos;
}


Coord3 visBase::DepthTabPlaneDragger::dragger2World( const Coord3& drag,
						     bool ispos ) const
{
    const Coord3 tpos = transform&&ispos ? transform->transformBack(drag) :drag;
    if ( !dim )
	return Coord3( tpos.z, tpos.y, tpos.x );
    if ( dim==1 )
	return Coord3( tpos.x, tpos.z, tpos.y );

    return tpos;
}


void visBase::DepthTabPlaneDragger::startCB(void* obj, SoDragger* )
{
    ((visBase::DepthTabPlaneDragger*) obj)->started.trigger();
}


void visBase::DepthTabPlaneDragger::motionCB(void* obj, SoDragger* )
{
    ((visBase::DepthTabPlaneDragger*) obj)->motion.trigger();
}


void visBase::DepthTabPlaneDragger::valueChangedCB(void* obj, SoDragger* d )
{
    ((visBase::DepthTabPlaneDragger*) obj)->changed.trigger();
}


void visBase::DepthTabPlaneDragger::finishCB(void* obj, SoDragger* )
{
    ((visBase::DepthTabPlaneDragger*) obj)->finished.trigger();
}

