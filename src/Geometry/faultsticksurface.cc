/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : J.C. Glas
 * DATE     : September 2007
-*/

static const char* rcsID = "$Id: faultsticksurface.cc,v 1.12 2008-10-20 07:35:17 cvsjaap Exp $";

#include "faultsticksurface.h"

namespace Geometry
{


#define mGetValidStickIdx( stickidx, sticknr, extra, errorres ) \
\
    int stickidx = sticknr - firstrow_; \
    if ( stickidx<-extra || stickidx>sticks_.size()+extra ) \
	return errorres;

#define mGetValidKnotIdx( knotidx, knotnr, stickidx, extra, errorres ) \
\
    if ( !firstcols_.size() ) return errorres; \
    int knotidx = knotnr - firstcols_[stickidx]; \
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


Element* FaultStickSurface::clone() const
{
    FaultStickSurface* res = new FaultStickSurface;
    deepCopy( res->sticks_, sticks_ );
    res->firstcols_ = firstcols_;
    res->firstrow_ = firstrow_;
    res->sticksvertical_ = sticksvertical_;
    res->editplanenormals_ = editplanenormals_;

    return res;
}



bool FaultStickSurface::insertStick( const Coord3& firstpos, 
				     const Coord3& editnormal, int sticknr,
				     int firstcol )
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
    if ( stickidx==-1 )
    {
	firstrow_--;
	stickidx++;
    }

    if ( stickidx==sticks_.size() )
    {
	sticks_ += new TypeSet<Coord3>;
	editplanenormals_ += normvec;
	firstcols_ += firstcol;
    }
    else
    {
	sticks_.insertAt( new TypeSet<Coord3>, stickidx );
	editplanenormals_.insert( stickidx, normvec );
	firstcols_.insert( stickidx, firstcol );
    }

    sticks_[stickidx]->insert( 0, firstpos );

    triggerNrPosCh( RowCol(stickidx,StickInsert).getSerialized() );
    if ( blocksCallBacks() )
	blockCallBacks( true, true );

    return true;
}


bool FaultStickSurface::removeStick( int sticknr )
{
    mGetValidStickIdx( stickidx, sticknr, 0, false );

    sticks_.remove( stickidx );
    editplanenormals_.remove( stickidx );
    firstcols_.remove( stickidx );

    if ( !stickidx )
	firstrow_++;

    triggerNrPosCh( RowCol(stickidx,StickRemove).getSerialized() );
    if ( blocksCallBacks() )
	blockCallBacks( true, true );
    
    return true;
}


bool FaultStickSurface::insertKnot( const RCol& rc, const Coord3& pos )
{
    if ( !pos.isDefined() )
	return false;

    mGetValidStickIdx( stickidx, rc.r(), 0, false );
    mGetValidKnotIdx( knotidx, rc.c(), stickidx, 1, false );
    if ( knotidx==-1 )
    {
	firstcols_[stickidx]--;
	knotidx++;
    }

    if ( knotidx==sticks_[stickidx]->size() )
	(*sticks_[stickidx]) += pos;
    else
	sticks_[stickidx]->insert( knotidx, pos );

    triggerNrPosCh( RowCol(stickidx,StickChange).getSerialized() );

    return true;
}


bool FaultStickSurface::removeKnot( const RCol& rc )
{
    mGetValidStickIdx( stickidx, rc.r(), 0, false );
    mGetValidKnotIdx( knotidx, rc.c(), stickidx, 0, false );

    if ( sticks_[stickidx]->size() <= 1 )
	return removeStick( rc.r() );

    sticks_[stickidx]->remove( knotidx );

    if ( !knotidx )
	firstcols_[stickidx]++;

    triggerNrPosCh( RowCol(stickidx,StickChange).getSerialized() );
    
    return true;
}


#define mEmptyInterval() StepInterval<int>( mUdf(int), mUdf(int), mUdf(int) )

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
    return StepInterval<int>(firstcol, firstcol+sticks_[stickidx]->size()-1, 1);
}


bool FaultStickSurface::setKnot( const RCol& rc, const Coord3& pos )
{
    if ( !pos.isDefined() )
    {
	return removeKnot( rc );
    }

    mGetValidStickIdx( stickidx, rc.r(), 0, false );
    mGetValidKnotIdx( knotidx, rc.c(), stickidx, 0, false );

    (*sticks_[stickidx])[knotidx] = pos;
    triggerMovement( RowCol(stickidx,StickChange).getSerialized() );
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


const Coord3& FaultStickSurface::getEditPlaneNormal( int stick ) const
{
    mGetValidStickIdx( stickidx, stick, 0, Coord3::udf() );
    if ( stickidx < editplanenormals_.size() )
	return editplanenormals_[stickidx];

    return Coord3::udf();
}


void FaultStickSurface::addEditPlaneNormal( const Coord3& editnormal )
{
    editplanenormals_ += editnormal;

    if ( editplanenormals_.size() > 1 )
	return;
    if ( !editnormal.isDefined() || mIsZero(editnormal.sqAbs(),mDefEps) )
	return;

    const Coord3 normvec = editnormal.normalize();
    sticksvertical_ = fabs(normvec.z) < 0.5;
}


void FaultStickSurface::addUdfRow( int sticknr, int firstknotnr, int nrknots )
{
    if ( isEmpty() )
	firstrow_ = sticknr;
    firstcols_ += firstknotnr;
    sticks_ += new TypeSet<Coord3>( nrknots, Coord3::udf() );
}


} // namespace Geometry
