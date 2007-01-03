/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visvolorthoslice.cc,v 1.1 2007-01-03 18:24:26 cvskris Exp $";


#include "visvolorthoslice.h"

#include "iopar.h"
#include "ranges.h"
#include "visdataman.h"
#include "visdepthtabplanedragger.h"
#include "vispickstyle.h"
#include "visselman.h"
#include "vistexture3.h"

#include "VolumeViz/nodes/SoOrthoSlice.h"

const char* visBase::OrthogonalSlice::dimstr = "Dim";
const char* visBase::OrthogonalSlice::slicestr = "Slice";

mCreateFactoryEntry( visBase::OrthogonalSlice );

visBase::OrthogonalSlice::OrthogonalSlice()
    : slice(new SoOrthoSlice)
    , dragger(visBase::DepthTabPlaneDragger::create())
    , pickstyle(visBase::PickStyle::create())
    , motion(this)
    , xdatasz(0), ydatasz(0), zdatasz(0)
{
    dragger->ref();
    dragger->setMaterial(0);
    dragger->removeScaleTabs();
    dragger->motion.notify( mCB(this,OrthogonalSlice,draggerMovementCB) );
    addChild( dragger->getInventorNode() );
    
    pickstyle->ref();
    pickstyle->setStyle( visBase::PickStyle::Unpickable );
    addChild( pickstyle->getInventorNode() );

    slice->alphaUse = SoOrthoSlice::ALPHA_AS_IS;
    
    addChild( slice );
}


visBase::OrthogonalSlice::~OrthogonalSlice()
{
    dragger->motion.remove( mCB(this, OrthogonalSlice, draggerMovementCB ));
    dragger->unRef();

    pickstyle->unRef();
}


void visBase::OrthogonalSlice::setVolumeDataSize(int xsz, int ysz, int zsz)
{
    xdatasz = xsz; ydatasz = ysz; zdatasz = zsz;
    draggerMovementCB(0);
}


void visBase::OrthogonalSlice::setSpaceLimits( const Interval<float>& x,
						    const Interval<float>& y,
						    const Interval<float>& z )
{
    dragger->setSpaceLimits( x,y,z );
    dragger->setCenter( Coord3(x.center(),y.center(),z.center()) );
    dragger->setSize( Coord3(x.width(),y.width(),z.width()) );
    draggerMovementCB(0);
}


int visBase::OrthogonalSlice::getDim() const
{
    return slice->axis.getValue();
}


void visBase::OrthogonalSlice::setDim( int dim )
{
    if ( !dim )
	slice->axis = SoOrthoSlice::X;
    else if ( dim==1 )
	slice->axis = SoOrthoSlice::Y;
    else
	slice->axis = SoOrthoSlice::Z;

    dragger->setDim(dim);
    draggerMovementCB(0);
}


float visBase::OrthogonalSlice::getPosition() const
{
    int nrslices;
    Interval<float> range;
    getSliceInfo( nrslices, range );

    return (float)getSliceNr()/nrslices*range.width()+range.start;
}


void visBase::OrthogonalSlice::setSliceNr(int nr) 
{
    slice->sliceNumber = nr;
}


int  visBase::OrthogonalSlice::getSliceNr() const
{ return slice->sliceNumber.getValue(); }


void visBase::OrthogonalSlice::fillPar( IOPar& par,
					     TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    par.set( dimstr, getDim() );
    par.set( slicestr, getSliceNr() );
}


int visBase::OrthogonalSlice::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res != 1 ) return res;

    int dim;
    if ( par.get(dimstr,dim) )
	setDim(dim);

    int slicenr;
    if ( par.get(slicestr,slicenr) )
	setSliceNr(slicenr);

    return 1;
}


void visBase::OrthogonalSlice::draggerMovementCB( CallBacker* cb )
{
    const int dim = getDim();
    float draggerpos = dragger->center()[dim];

    int nrslices;
    Interval<float> range;
    getSliceInfo( nrslices, range );
    if ( !nrslices ) return;

    float slicenrf = (draggerpos-range.start)/range.width()*(nrslices-1);
    const int slicenr = mNINT(slicenrf);
    if ( slicenr != getSliceNr() )
	setSliceNr( slicenr );

    if ( cb )
	motion.trigger();
}


void visBase::OrthogonalSlice::getSliceInfo( int& nrslices,
						  Interval<float>& range ) const
{
    Interval<float> xrange, yrange, zrange;
    dragger->getSpaceLimits( xrange, yrange, zrange );

    const int dim = getDim();
    if ( dim == 0 )
    {
	nrslices = xdatasz;
	range = xrange;
    }
    else if ( dim == 1 )
    {
	nrslices = ydatasz;
	range = yrange;
    }
    else
    {
	nrslices = zdatasz;
	range = zrange;
    }
}
