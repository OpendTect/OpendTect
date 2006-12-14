/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		December 2006
 RCS:		$Id: randomlinegeom.cc,v 1.1 2006-12-14 21:44:35 cvsnanne Exp $
________________________________________________________________________

-*/

#include "randomlinegeom.h"

namespace Geometry
{

RandomLine::RandomLine()
    : nodeAdded(this)
    , nodeInserted(this)
    , nodeReoved(this)
    , zrangeChanged(this)
{}


int RandomLine::addNode( const BinID& bid )
{
    nodes_ += bid;
    return nodes_.size()-1;
}


void RandomLine::insertNode( int idx, const BinID& bid )
{ nodes_.insert( idx, bid ); }

void RandomLine::setNodePosition( int idx, const BinID& bid )
{ nodes_[idx] = bid; }

void RandomLine::removeNode( int idx )
{ nodes_.remove( idx ); }

void RandomLine::removeNode( const BinID& bid )
{ nodes_ -= bid; }

int RandomLine::nodeIndex( const BinID& bid ) const
{ return nodes_.indexOf( bid ); }

int RandomLine::nrNodes() const
{ return nodes_.size(); }

const BinID& RandomLine::nodePosition( int idx ) const
{ return nodes_[idx]; }

void RandomLine::getAllNodes( TypeSet<BinID>& bids ) const
{ bids = nodes_; }

} // namespace
