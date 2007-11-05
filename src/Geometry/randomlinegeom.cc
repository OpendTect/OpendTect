/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		December 2006
 RCS:		$Id: randomlinegeom.cc,v 1.3 2007-11-05 15:20:06 cvsbert Exp $
________________________________________________________________________

-*/

#include "randomlinegeom.h"
#include "iopar.h"

namespace Geometry
{

RandomLine::RandomLine()
    : nodeAdded(this)
    , nodeInserted(this)
    , nodeRemoved(this)
    , zrangeChanged(this)
    , lset_(0)
{
}


int RandomLine::addNode( const BinID& bid )
{
    nodes_ += bid;
    nodeAdded.trigger();
    return nodes_.size()-1;
}


void RandomLine::insertNode( int idx, const BinID& bid )
{ nodes_.insert( idx, bid ); nodeInserted.trigger(); }

void RandomLine::setNodePosition( int idx, const BinID& bid )
{ nodes_[idx] = bid; }

void RandomLine::removeNode( int idx )
{ nodes_.remove( idx ); nodeRemoved.trigger(); }

void RandomLine::removeNode( const BinID& bid )
{ nodes_ -= bid; nodeRemoved.trigger(); }

int RandomLine::nodeIndex( const BinID& bid ) const
{ return nodes_.indexOf( bid ); }

int RandomLine::nrNodes() const
{ return nodes_.size(); }

const BinID& RandomLine::nodePosition( int idx ) const
{ return nodes_[idx]; }

void RandomLine::allNodePositions( TypeSet<BinID>& bids ) const
{ bids = nodes_; }


RandomLineSet::RandomLineSet()
    : pars_(*new IOPar)
{
}


RandomLineSet::~RandomLineSet()
{
    deepErase(lines_);
    delete &pars_;
}


} // namespace
