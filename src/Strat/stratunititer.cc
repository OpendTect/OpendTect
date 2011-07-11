/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/

static const char* rcsID = "$Id: stratunititer.cc,v 1.2 2011-07-11 13:37:50 cvsbert Exp $";

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


bool Strat::UnitRefIter::next()
{
    while ( toNext() )
    {
	if ( pol_ == All )
	    return true;

	switch ( unit()->type() )
	{
	case UnitRef::Leaf:
	    if ( pol_ == Leaves )
		return true;
	break;
	case UnitRef::NodeOnly:
	    if ( pol_ == AllNodes || pol_ == NodesOnly )
		return true;
	break;
	case UnitRef::Leaved:
	    if ( pol_ == AllNodes || pol_ == LeavedNodes )
		return true;
	break;
	}
    }

    return false;
}


bool Strat::UnitRefIter::toNext()
{
    if ( !curnode_ ) return false; // At end

    // First see if we can simply take next ref or go down
    UnitRef* curun = gtUnit();
    if ( curun->isLeaf() )
    {
	if ( curidx_ < curnode_->nrRefs() - 1 )
	    { curidx_++; return true; }
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

    // OK so this node (and everything below) is done.
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

    curnode_ = 0;
    return false;
}
