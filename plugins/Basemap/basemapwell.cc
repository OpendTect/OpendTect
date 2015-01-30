/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "basemapwell.h"

#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "welldata.h"
#include "wellreader.h"
#include "survinfo.h"

using namespace Basemap;


WellObject::WellObject( const MultiID& mid )
    : BaseMapObject(0)
    , key_(mid)
    , data_(*new Well::Data)
    , rdr_(0)
{
    setKey( mid );
    data_.ref();
}


WellObject::~WellObject()
{
    delete rdr_;
    data_.unRef();
}


void WellObject::setInvalid()
{
    delete rdr_; rdr_ = 0;
    data_.setEmpty();
}


void WellObject::setKey( const MultiID& mid )
{
    key_ = mid;
    setInvalid();

    PtrMan<IOObj> ioobj = IOM().get( key_ );
    if ( !ioobj )
    {
	setName( BufferString("<ID=",mid,">") );
	return;
    }
    setName( ioobj->name() );

    rdr_ = new Well::Reader( *ioobj, data_ );
    Coord coord;
    rdr_->getMapLocation( coord );
    if ( !SI().isReasonable(coord) )
	{ setInvalid(); return; }

    coord_ = coord;
    updateGeometry();
}


void WellObject::updateGeometry()
{
    changed.trigger();
}


int WellObject::nrShapes() const	{ return 1; }

const char* WellObject::getShapeName( int ) const
{
    return name();
}


void WellObject::getPoints( int shapeidx, TypeSet<Coord>& pts ) const
{
    if ( isOK() )
	pts.add( coord_ );
}


const MarkerStyle2D* WellObject::getMarkerStyle( int ) const
{
    return &ms_;
}


void WellObject::setMarkerStyle( int, const MarkerStyle2D& ms )
{
    ms_ = ms;
}
