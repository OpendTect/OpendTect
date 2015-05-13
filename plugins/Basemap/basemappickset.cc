/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "basemappickset.h"

#include "ioman.h"
#include "ioobj.h"
#include "pickset.h"
#include "picksettr.h"
#include "ptrman.h"

namespace Basemap
{

PickSetObject::PickSetObject( const MultiID& mid )
    : BaseMapObject(0)
    , picksetmid_(mid)
    , ps_(*new Pick::Set)
{
    setMultiID( mid );
}


PickSetObject::~PickSetObject()
{
    delete &ps_;
}


void PickSetObject::setMultiID( const MultiID& mid )
{
    picksetmid_ = mid;
    PtrMan<IOObj> ioobj = IOM().get( picksetmid_ );
    if ( !ioobj ) return;

    setName( ioobj->name() );

    ps_.setEmpty();
    BufferString errmsg;
    PickSetTranslator::retrieve( ps_, ioobj, true, errmsg );
    ms_.color_ = ps_.disp_.color_;
    ms_.type_ = MarkerStyle2D::Circle;
}


void PickSetObject::updateGeometry()
{ changed.trigger(); }


int PickSetObject::nrShapes() const	{ return 1; }

const char* PickSetObject::getShapeName( int ) const
{ return name().buf(); }


void PickSetObject::getPoints( int shapeidx, TypeSet<Coord>& pts ) const
{
    for ( int idx=0; idx<ps_.size(); idx++ )
	pts += ps_[idx].pos_;
}


const MarkerStyle2D* PickSetObject::getMarkerStyle( int ) const
{ return &ms_; }

void PickSetObject::setMarkerStyle( int, const MarkerStyle2D& ms )
{ ms_ = ms; }

} // namespace Basemap
