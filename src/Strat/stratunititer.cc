/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/

static const char* rcsID = "$Id: stratunititer.cc,v 1.4 2011/07/13 12:34:08 cvsbert Exp $";

#include "stratunitrefiter.h"
#include "stratunitref.h"


Strat::UnitRefIter::UnitRefIter( const NodeUnitRef& ur, Pol p )
	: itnode_(const_cast<NodeUnitRef*>(&ur))
    	, pol_(p)
{
    reset();
}

Strat::UnitRefIter& Strat::UnitRefIter::operator =(
				const Strat::UnitRefIter& oth )
{
    if ( this != &oth )
    {
	itnode_ = oth.itnode_;
    	pol_ = oth.pol_;
	curidx_ = oth.curidx_;
	curnode_ = oth.curnode_;
    }
    return *this;
}


void Strat::UnitRefIter::reset()
{
    curidx_ = -1;
    curnode_ = itnode_;
}


Strat::UnitRefIter::Pol Strat::UnitRefIter::polOf( UnitRef::Type typ )
{
    return typ == UnitRef::NodeOnly ?	NodesOnly
	: (typ == UnitRef::Leaved ?	LeavedNodes
				  :	Leaves);
}


Strat::UnitRefIter::Pol Strat::UnitRefIter::polOf( const UnitRef* un )
{
    return un ? polOf(un->type()) : All;
}


Strat::UnitRef* Strat::UnitRefIter::gtUnit() const
{
    const Strat::UnitRef* ret = curnode_;
    if ( curnode_ && curidx_ >= 0 )
	ret = &curnode_->ref( curidx_ );

    return const_cast<Strat::UnitRef*>( ret );
}


bool Strat::UnitRefIter::isValid( const UnitRef& ur,
				  Strat::UnitRefIter::Pol pol )
{
    if ( pol == All )
	return true;

    switch ( ur.type() )
    {
    case UnitRef::Leaf:
	if ( pol == Leaves )
	    return true;
    break;
    case UnitRef::NodeOnly:
	if ( pol == AllNodes || pol == NodesOnly )
	    return true;
    break;
    case UnitRef::Leaved:
	if ( pol == AllNodes || pol == LeavedNodes )
	    return true;
    break;
    }

    return false;
}


bool Strat::UnitRefIter::next()
{
    while ( toNext() )
    {
	if ( isValid(*unit(),pol_) )
	    return true;
    }

    return false;
}


bool Strat::UnitRefIter::toNext()
{
    if ( !curnode_ ) return false; // At end

    UnitRef* curun = gtUnit();
    if ( curun->isLeaf() )
    {
	// If there is a next leaf, take it
	if ( curidx_ < curnode_->nrRefs() - 1 )
	    { curidx_++; return true; }
	// No: this leavednode is done
    }
    else
    {
	if ( curun != curnode_ )
	{
	    curnode_ = (NodeUnitRef*)curun;
	    if ( curnode_->nrRefs() > 0 )
	    {
		curidx_ = 0;
		return true;
	    }
	}
	else if ( curnode_->nrRefs() > 0 )
	    { curidx_ = 0; return true; }
    }

    // This node (and everything below) is done.
    if ( curnode_ != itnode_ )
    {
	while ( true )
	{
	    Strat::NodeUnitRef* par = curnode_->upNode();
	    if ( !par ) break;

	    curidx_ = par->indexOf( curnode_ );
	    curnode_ = par;
	    if ( curidx_ < par->nrRefs() - 1 )
		{ curidx_++; return true; }

	    if ( curnode_ == itnode_ ) break;
	}
    }

    // We're all done
    curnode_ = 0; return false;
}


Interval<int> Strat::UnitRefIter::levelRange() const
{
    Strat::UnitRefIter it( *this );
    while ( !isValid(*it.unit(),pol_) )
    {
	if ( !it.next() )
	    return Interval<int>( itnode_->level(), itnode_->level() );
    }
    Interval<int> rg( it.unit()->level(), it.unit()->level() );
    while ( it.next() )
	rg.include( it.unit()->level() );

    return rg;
}
