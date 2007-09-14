/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        J.C. Glas
 Date:          September 2007
 RCS:           $Id: faultsticksurface.cc,v 1.1 2007-09-14 15:37:59 cvsjaap Exp $
________________________________________________________________________

-*/

#include "faultsticksurface.h"

namespace Geometry
{


#define mGetValidStickIdx( stickidx, sticknr, extra, errorres ) \
\
    const int stickidx = sticknr - firstrow_; \
    if ( stickidx<0 || stickidx>sticks_.size()+extra ) \
	return errorres;

#define mGetValidKnotIdx( knotidx, knotnr, stickidx, extra, errorres ) \
\
    const int knotidx = knotnr - firstcols_[stickidx]; \
    if ( knotidx<0 && knotidx>sticks_[stickidx]->size()+extra ) \
	return errorres;

    
FaultStickSurface::FaultStickSurface()
    : sticksvertical_(true)
    , firstrow_(0)
{}


FaultStickSurface::~FaultStickSurface()
{
    deepErase( sticks_ );
}


#define mGetOldColRanges( oldcolrg ) \
\
    TypeSet<Interval<int> > oldcolrg; \
    for ( int row=firstrow_; row<=firstrow_+sticks_.size(); row++ ) \
	oldcolrg += colRange( row ); 


#define mSticksChgTrigger( stickidx, knotidx, oldcolrg, incr ) \
{ \
    const int maxnrsticks = incr>0 ? sticks_.size() : sticks_.size()+1; \
    const int dir = (stickidx < maxnrsticks/2) ? -1 : 1; \
    TypeSet<GeomPosID> movedpids; \
    TypeSet<GeomPosID> changedpids; \
\
    for ( int idx=stickidx; idx>=0 && idx<maxnrsticks; idx+=dir ) \
    { \
	const int row = firstrow_ + idx; \
	const Interval<int> newcolrg = colRange( row ); \
\
	for ( int col=oldcolrg[idx].start; col<=oldcolrg[idx].stop; col++ ) \
	{ \
	    if ( newcolrg.includes(col) ) \
		changedpids += RowCol(row,col).getSerialized(); \
	    else \
		movedpids += RowCol(row,col).getSerialized(); \
	} \
\
	for ( int col=newcolrg.start; col<=newcolrg.stop; col++ ) \
	{ \
	    if ( !oldcolrg[idx].includes(col) ) \
		changedpids += RowCol(row,col).getSerialized(); \
	} \
    } \
\
    if ( dir == -1 ) \
	firstrow_ -= incr; \
\
    triggerNrPosCh( changedpids ); \
    triggerMovement( movedpids ); \
} 

bool FaultStickSurface::insertStick( const Coord3& firstpos, 
				     const Coord3& editnormal, int sticknr )
{
    if ( !firstpos.isDefined() )
	return false;
    if ( !editnormal.isDefined() || mIsZero(editnormal.sqAbs(),mDefEps) )
	return false;

    const Coord3 normvec = editnormal.normalize();
    const bool newstickvert = fabs(normvec.z) < 0.5;

    if ( sticks_.isEmpty() )
    {
	sticksvertical_ = newstickvert;
	firstrow_ = sticknr;
    }

    if ( newstickvert != sticksvertical_ )
	return false;

    mGetValidStickIdx( stickidx, sticknr, 1, false );
    mGetOldColRanges( oldcolrg );
    
    sticks_.insertAt( new TypeSet<Coord3>, stickidx );
    editplanenormals_.insert( stickidx, normvec );
    firstcols_.insert( stickidx, 0 );
    insertKnot( RowCol(stickidx,0), firstpos );

    mSticksChgTrigger( stickidx, knotidx, oldcolrg, 1 );
    return true;
}


bool FaultStickSurface::removeStick( int sticknr )
{
    mGetValidStickIdx( stickidx, sticknr, 0, false );
    mGetOldColRanges( oldcolrg );

    sticks_.remove( stickidx );
    editplanenormals_.remove( stickidx );
    firstcols_.remove( stickidx );

    mSticksChgTrigger( stickidx, knotidx, oldcolrg, -1 );
    return true;
}


#define mKnotsChgTrigger( stickidx, knotidx, incr ) \
{ \
    const int nrknots = sticks_[stickidx]->size(); \
    const int maxnrknots = incr>0 ? nrknots : nrknots+1; \
    const int dir = (knotidx < maxnrknots/2) ? -1 : 1; \
    TypeSet<GeomPosID> pids; \
\
    for ( int idx=knotidx; idx>=0 && idx<maxnrknots; idx+=dir ) \
    { \
	const RowCol currc( firstrow_+stickidx, firstcols_[stickidx]+idx ); \
	pids += currc.getSerialized(); \
    } \
\
    if ( dir == -1 ) \
	firstcols_[stickidx] -= incr; \
\
    triggerNrPosCh( pids[pids.size()-1] ); \
    pids.remove( pids.size()-1 ); \
    triggerMovement( pids ); \
}

bool FaultStickSurface::insertKnot( const RCol& rc, const Coord3& pos )
{
    if ( !pos.isDefined() )
	return false;

    mGetValidStickIdx( stickidx, rc.r(), 0, false );
    mGetValidKnotIdx( knotidx, rc.c(), stickidx, 1, false );
    
    sticks_[stickidx]->insert( knotidx, pos );
    mKnotsChgTrigger( stickidx, knotidx, 1 );

    return true;
}


bool FaultStickSurface::removeKnot( const RCol& rc )
{
    mGetValidStickIdx( stickidx, rc.r(), 0, false );
    mGetValidKnotIdx( knotidx, rc.c(), stickidx, 0, false );

    if ( sticks_[stickidx]->size() <= 1 )
	return removeStick( rc.r() );

    sticks_[stickidx]->remove( knotidx );
    mKnotsChgTrigger( stickidx, knotidx, -1 );
    
    return true;
}


#define mEmptyInterval() StepInterval<int>( INT_MAX, INT_MIN, 1 )

StepInterval<int> FaultStickSurface::rowRange() const
{
    if ( sticks_.isEmpty() )
	return mEmptyInterval();
    
    return StepInterval<int>( firstrow_, firstrow_+sticks_.size()-1, 1 ); 
}


StepInterval<int> FaultStickSurface::colRange( int sticknr ) const
{
    mGetValidStickIdx( stickidx, sticknr, 0, mEmptyInterval() );

    const int firstcol = firstcols_[stickidx];
    return StepInterval<int>( firstcol, firstcol+sticks_[stickidx]->size(), 1 );
}


bool FaultStickSurface::setKnot( const RCol& rc, const Coord3& pos )
{
    if ( !pos.isDefined() )
	return false;

    mGetValidStickIdx( stickidx, rc.r(), 0, false );
    mGetValidKnotIdx( knotidx, rc.c(), stickidx, 0, false );

    (*sticks_[stickidx])[knotidx] = pos;
    triggerMovement( rc.getSerialized() );

    return true;
}


Coord3 FaultStickSurface::getKnot( const RCol& rc ) const
{
    mGetValidStickIdx( stickidx, rc.r(), 0, Coord3::udf() );
    mGetValidKnotIdx( knotidx, rc.c(), stickidx, 0, Coord3::udf() );
    
    return (*sticks_[stickidx])[knotidx];
}


bool FaultStickSurface::isKnotDefined( const RCol& rc ) const
{
    mGetValidStickIdx( stickidx, rc.r(), 0, false );
    mGetValidKnotIdx( knotidx, rc.c(), stickidx, 0, false );

    return true;
}


bool FaultStickSurface::areSticksVertical() const
{
    return sticksvertical_;
}


const Coord3& FaultStickSurface::getEditPlaneNormal( int sticknr ) const
{
    mGetValidStickIdx( stickidx, sticknr, 0, Coord3::udf() );

    return editplanenormals_[stickidx];
}


} // namespace Geometry
