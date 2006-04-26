/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: emrowcoliterator.cc,v 1.1 2006-04-26 21:15:37 cvskris Exp $
________________________________________________________________________

-*/

#include "emrowcoliterator.h"

#include "rowcolsurface.h"
#include "emsurface.h"

using namespace EM;

RowColIterator::RowColIterator( const Surface& surf,
				const SectionID& sectionid )
    : surf_( surf )
    , sid_( sectionid )
    , cursection_( 0 )
    , allsids_( sectionid==-1 )
{
    if ( allsids_ )
	sid_ = surf_.sectionID(0);
}


PosID RowColIterator::next()
{
    if ( !cursection_ )
    {
	if ( !initSection() )
	    return PosID(-1,-1,-1);
    }
    else
    {
	rc_.col += colrg_.step;
	if ( !colrg_.includes(rc_.col) )
	{
	    rc_.row += rowrg_.step;
	    if ( !rowrg_.includes(rc_.row) )
	    {
		cursection_ = 0;
		if ( !nextSection() )
		    return PosID(-1,-1,-1);
	    }

	    colrg_ = cursection_->colRange( rc_.row );
	    rc_.col = colrg_.start;
	}
    }

    if ( !cursection_->isKnotDefined( rc_ ) )
	return next();


    return PosID( surf_.id(), sid_, rc_.getSerialized() );
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

    const StepInterval<int> rowrg = rcs->rowRange();

    int res = 0;
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
	res += rcs->colRange( row ).nrSteps()+1;

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
    colrg_ = rcs->colRange( rowrg_.start );

    rc_.row = rowrg_.start;
    rc_.col = colrg_.start;
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

