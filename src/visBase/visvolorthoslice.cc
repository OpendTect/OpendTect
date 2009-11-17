/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visvolorthoslice.cc,v 1.12 2009-11-17 09:50:27 cvskarthika Exp $";


#include "visvolorthoslice.h"

#include "iopar.h"
#include "ranges.h"
#include "visdataman.h"
#include "visdepthtabplanedragger.h"
#include "vispickstyle.h"
#include "visselman.h"
#include "vistexture3.h"

#include "VolumeViz/nodes/SoOrthoSlice.h"

mCreateFactoryEntry( visBase::OrthogonalSlice );

namespace visBase
{

const char* OrthogonalSlice::dimstr = "Dim";
const char* OrthogonalSlice::slicestr = "Slice";

OrthogonalSlice::OrthogonalSlice()
    : VisualObjectImpl( false )
    , slice(new SoOrthoSlice)
    , dragger(DepthTabPlaneDragger::create())
    , pickstyle(PickStyle::create())
    , motion(this)
    , xdatasz(0), ydatasz(0), zdatasz(0)
{
    dragger->ref();
    dragger->setMaterial(0);
    dragger->removeScaleTabs();
    dragger->motion.notify( mCB(this,OrthogonalSlice,draggerMovementCB) );
    addChild( dragger->getInventorNode() );
    
    pickstyle->ref();
    pickstyle->setStyle( PickStyle::Unpickable );
    addChild( pickstyle->getInventorNode() );

    slice->alphaUse = SoOrthoSlice::ALPHA_AS_IS;
    
    addChild( slice );
}


OrthogonalSlice::~OrthogonalSlice()
{
    dragger->motion.remove( mCB(this, OrthogonalSlice, draggerMovementCB ));
    dragger->unRef();

    pickstyle->unRef();
}


void OrthogonalSlice::setVolumeDataSize(int xsz, int ysz, int zsz)
{
    xdatasz = xsz; ydatasz = ysz; zdatasz = zsz;
    draggerMovementCB(0);
}


void OrthogonalSlice::setSpaceLimits( const Interval<float>& x,
				      const Interval<float>& y,
				      const Interval<float>& z )
{
    dragger->setSpaceLimits( x,y,z );
    dragger->setCenter( Coord3(x.center(),y.center(),z.center()) );
    dragger->setSize( Coord3(x.width(),y.width(),z.width()) );
    draggerMovementCB(0);
}


void OrthogonalSlice::setCenter( const Coord3& newcenter, bool alldims )
{
    dragger->setCenter( newcenter, alldims );
    draggerMovementCB(0);
}


visBase::DepthTabPlaneDragger* OrthogonalSlice::getDragger() const
{
    return dragger;
}


int OrthogonalSlice::getDim() const
{
    return slice->axis.getValue();
}


void OrthogonalSlice::setDim( int dim )
{
    if ( !dim )
	slice->axis = SoOrthoSlice::X;
    else if ( dim==1 )
	slice->axis = SoOrthoSlice::Y;
    else
	slice->axis = SoOrthoSlice::Z;

    dragger->setDim( dim );
    draggerMovementCB(0);
}


float OrthogonalSlice::getPosition() const
{
    int nrslices;
    Interval<float> range;
    getSliceInfo( nrslices, range );

    if ( !nrslices ) return range.center();
    return (float)getSliceNr()/nrslices*range.width()+range.start;
}


void OrthogonalSlice::setSliceNr( int nr )
{ slice->sliceNumber = nr; }

int  OrthogonalSlice::getSliceNr() const
{ return slice->sliceNumber.getValue(); }


NotifierAccess& OrthogonalSlice::dragStart()
{ return dragger->started; }


NotifierAccess& OrthogonalSlice::dragFinished()
{ return dragger->finished; }


void OrthogonalSlice::draggerMovementCB( CallBacker* cb )
{
    const int dim = getDim();
    float draggerpos = dragger->center()[dim];

    int nrslices;
    Interval<float> range;
    getSliceInfo( nrslices, range );
    if ( !nrslices ) return;

    float slicenrf = (draggerpos-range.start)/range.width()*(nrslices-1);
    int slicenr = mNINT(slicenrf);
    if ( slicenr>=nrslices ) slicenr=nrslices-1;
    else if ( slicenr<0 ) slicenr=0;

    if ( slicenr != getSliceNr() )
	setSliceNr( slicenr );

    if ( cb )
	motion.trigger();
}


void OrthogonalSlice::getSliceInfo( int& nrslices, Interval<float>& range) const
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


void OrthogonalSlice::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    par.set( dimstr, getDim() );
    par.set( slicestr, getSliceNr() );
}


int OrthogonalSlice::usePar( const IOPar& par )
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

} // namespace visBase
