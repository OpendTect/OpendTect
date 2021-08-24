/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/

#include "emrowcoliterator.h"

#include "trckeyzsampling.h"
#include "emsurface.h"
#include "rowcolsurface.h"
#include "survinfo.h"
#include "typeset.h"

namespace EM
{

RowColIterator::RowColIterator( const Surface& surf, const SectionID& sectionid,
				const TrcKeyZSampling* cs	)
    : surf_( surf )
    , sid_( sectionid )
    , cursection_( 0 )
    , allsids_( sectionid==-1 )
    , csbound_( cs )
    , rowcolbounded_( false )
{
    if ( allsids_ )
	sid_ = surf_.sectionID(0);

    fillPosIDs();
}


RowColIterator::RowColIterator( const Surface& surf, const SectionID& sectionid,
				const StepInterval<int> rowbnd,
				const StepInterval<int> colbnd )
    : surf_( surf )
    , sid_( sectionid )
    , cursection_( 0 )
    , allsids_( sectionid==-1 )
    , csbound_( 0 )
    , rowcolbounded_( true )
    , rowbound_( rowbnd )
    , colbound_( colbnd )
{
    if ( allsids_ )
	sid_ = surf_.sectionID(0);

    fillPosIDs();
}


RowColIterator::~RowColIterator()
{
    deepErase( posids_ );
}


void RowColIterator::fillPosIDs()
{
    if ( !posids_.isEmpty() )
	posids_.setEmpty();

    if ( allsids_ )
    {
	for ( int ids=0; ids<surf_.nrSections(); ids++ )
	{
	    TypeSet<GeomPosID>* posids = new TypeSet<GeomPosID>;
	    posids_ += posids;

	    mDynamicCastGet( const Geometry::RowColSurface*, rcs,
			     surf_.sectionGeometry( surf_.sectionID( ids ) ) );
	    if ( !rcs ) return;

	    rcs->getPosIDs( *posids );
	}
    }
    else
    {
	int idx = surf_.sectionIndex( sid_ );
	while ( posids_.size()<idx+1 )
	{
	    TypeSet<GeomPosID>* posids = new TypeSet<GeomPosID>;
	    posids_ += posids;
	}
	mDynamicCastGet( const Geometry::RowColSurface*, rcs,
			 surf_.sectionGeometry( sid_ ) );
	if ( !rcs ) return;

	rcs->getPosIDs( *posids_[idx] );
    }
}


PosID RowColIterator::next()
{
    while ( true )
    {
	if ( !cursection_ )
	{
	    if ( !initSection() )
		return PosID(-1,-1,-1);
	}
	else
	{
	    rc_.col() += colrg_.step;
	    if ( !colrg_.includes(rc_.col(),true) )
	    {
		rc_.row() += rowrg_.step;
		if ( !rowrg_.includes(rc_.row(),true) )
		{
		    cursection_ = 0;
		    if ( !nextSection() )
			return PosID(-1,-1,-1);
		}

		colrg_ = cursection_->colRange( rc_.row() );
		if ( rowcolbounded_ )
		    colrg_.limitTo( colbound_ );
		rc_.col() = colrg_.start;
	    }
	}
	if ( !cursection_->isKnotDefined( rc_ ) )
	    continue;

	if ( !csbound_ )
	    break;

	pos_ = surf_.getPos( sid_, rc_.toInt64() );
	bid_ = SI().transform( pos_ );

	if ( csbound_->hsamp_.includes(bid_) &&
             csbound_->zsamp_.includes(pos_.z,false) )
	    break;
    }

    return PosID( surf_.id(), sid_, rc_.toInt64() );
}


PosID RowColIterator::fromIndex( int idx ) const
{
    if ( allsids_ )
    {
	for ( int ids=0; ids<posids_.size(); ids++ )
	{
	    const TypeSet<GeomPosID>& posids = *posids_[ids];
	    if ( idx > posids.size() )
		idx -= posids.size();
	    else
		return PosID( surf_.id(), surf_.sectionID( ids ),
			      posids[idx] );
	}
    }
    else
    {
	const int sectIdx = surf_.sectionIndex( sid_ );
	const TypeSet<GeomPosID>& posids = *posids_[sectIdx];
	if ( idx < posids.size() )
	    return PosID( surf_.id(), sid_, posids[idx] );
    }
    return PosID( -1, -1, -1 );
}


int RowColIterator::maxIndex() const
{
    if ( allsids_ )
    {
	int sum = 0;
	for ( int idx=0; idx<posids_.size(); idx++ )
	{
	    const TypeSet<GeomPosID>& posids = *posids_[idx];
	    sum += posids.size();
	}
	return sum;
    }
    const int sectIdx = surf_.sectionIndex( sid_ );
    const TypeSet<GeomPosID>& posids = *posids_[sectIdx];
    return posids.size();
}


int RowColIterator::maximumSize() const
{
    if ( allsids_ )
    {
	int sum = 0;
	for ( int idx=0; idx<surf_.nrSections(); idx++ )
	    sum += maximumSize( surf_.sectionID(idx) );

	return sum;
    }

    return maximumSize( sid_ );
}


int RowColIterator::maximumSize( const SectionID& cursid ) const
{
    mDynamicCastGet( const Geometry::RowColSurface*, rcs,
		     surf_.sectionGeometry(cursid) );
    if ( !rcs ) return 0;

    StepInterval<int> rowrg = rcs->rowRange();
    if ( rowcolbounded_ )
	rowrg.limitTo( rowbound_ );

    int res = 0;
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	StepInterval<int> colrg = rcs->colRange( row );
	if ( rowcolbounded_ )
	    colrg.limitTo( colbound_ );
	res += colrg.nrSteps()+1;
    }

    return res;
}


bool RowColIterator::initSection()
{
    mDynamicCastGet( const Geometry::RowColSurface*, rcs,
		     surf_.sectionGeometry(sid_) );
    if ( !rcs )
	return nextSection();

    cursection_ = rcs;

    rowrg_ = rcs->rowRange();
    if ( rowrg_.stop < rowrg_.start )
	return false;
    colrg_ = rcs->colRange( rowrg_.start );
    if ( colrg_.stop < colrg_.start )
	return false;

    if ( rowcolbounded_ )
    {
	rowrg_.limitTo( rowbound_ );
	colrg_.limitTo( colbound_ );
    }

    rc_.row() = rowrg_.start;
    rc_.col() = colrg_.start;
    return true;
}


bool RowColIterator::nextSection()
{
    if ( !allsids_ ) return false;

    cursection_ = 0;
    int idx=0;
    for ( ; idx<surf_.nrSections(); idx++ )
    {
	if ( surf_.sectionID(idx)==sid_ )
	    break;
    }

    if ( idx<surf_.nrSections()-1 )
    {
	sid_ = surf_.sectionID(++idx);
	return initSection();
    }

    return false;
}

} // namespace EM
