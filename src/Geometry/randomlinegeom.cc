/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		December 2006
 RCS:		$Id: randomlinegeom.cc,v 1.5 2007-12-01 15:34:57 cvsbert Exp $
________________________________________________________________________

-*/

#include "randomlinegeom.h"
#include "survinfo.h"
#include "iopar.h"

namespace Geometry
{

RandomLine::RandomLine( const char* nm )
    : NamedObject(nm)
    , nodeAdded(this)
    , nodeInserted(this)
    , nodeRemoved(this)
    , zrangeChanged(this)
    , lset_(0)
{
    assign( zrange_, SI().zRange(true) );
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
