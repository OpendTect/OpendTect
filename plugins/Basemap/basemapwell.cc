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

namespace Basemap
{

WellObject::WellObject( const MultiID& mid )
    : BaseMapObject(0)
    , wellmid_(mid)
    , wd_(*new Well::Data)
{
    PtrMan<IOObj> ioobj = IOM().get( wellmid_ );
    if ( ioobj ) setName( ioobj->name() );

    Well::Reader rdr( ioobj->fullUserExpr(true), wd_ );
    rdr.getInfo();
}


WellObject::~WellObject()
{
    delete &wd_;
}


void WellObject::updateGeometry()
{ changed.trigger(); }


int WellObject::nrShapes() const	{ return 1; }

const char* WellObject::getShapeName( int ) const
{ return name(); }


void WellObject::getPoints( int shapeidx, TypeSet<Coord>& pts ) const
{ pts.add( wd_.info().surfacecoord ); }


const MarkerStyle2D* WellObject::getMarkerStyle( int ) const
{ return &ms_; }

void WellObject::setMarkerStyle( int, const MarkerStyle2D& ms )
{ ms_ = ms; }

} // namespace Basemap
