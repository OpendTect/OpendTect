/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2004
-*/

static const char* rcsID = "$Id: geomelement.cc,v 1.3 2005-02-20 13:42:26 cvskris Exp $";

#include "geomelement.h"
#include "survinfo.h"

namespace Geometry
{
Element::Element()
    : nrpositionnotifier( this )
    , movementnotifier( this )
    , ischanged( false )
    , errmsg_( 0 )
{ }


Element::~Element()
{ delete errmsg_; }


IntervalND<float> Element::boundingBox(bool) const
{
    TypeSet<GeomPosID> ids;
    getPosIDs( ids );

    IntervalND<float> bbox(3);
    Coord3 pos;
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	pos = getPosition(ids[idx]);
	//if ( !pos.isDefined() )
	    //continue;

	if ( !bbox.isSet() ) bbox.setRange(pos);
	else bbox.include(pos);
    }

    return bbox;
}



const char* Element::errMsg() const
{ return errmsg_ && errmsg_->size() ? (*errmsg_) : 0; }


BufferString& Element::errmsg()
{
    if ( !errmsg_ ) errmsg_ = new BufferString;
    return *errmsg_;
}


void Element::triggerMovement( const TypeSet<GeomPosID>& gpids )
{
    if ( !gpids.size() ) return;

    movementnotifier.trigger( &gpids, this );
    ischanged = true;
}


void Element::triggerMovement( const GeomPosID& gpid )
{
    TypeSet<GeomPosID> gpids( 1, gpid );
    movementnotifier.trigger( &gpids, this );
    ischanged = true;
}


void Element::triggerMovement()
{
    movementnotifier.trigger( 0, this );
    ischanged = true;
}


void Element::triggerNrPosCh( const TypeSet<GeomPosID>& gpids )
{
    if ( !gpids.size() ) return;
    nrpositionnotifier.trigger( &gpids, this );
    ischanged = true;
}


void Element::triggerNrPosCh( const GeomPosID& gpid )
{
    TypeSet<GeomPosID> gpids( 1, gpid );
    nrpositionnotifier.trigger( &gpids, this );
    ischanged = true;
}


void Element::triggerNrPosCh()
{
    nrpositionnotifier.trigger( 0, this );
    ischanged = true;
}
}; //Namespace

