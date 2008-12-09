/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : J.C. Glas
 * DATE     : November 2008
-*/

static const char* rcsID = "$Id: faultstickset.cc,v 1.2 2008-12-09 09:45:18 cvsjaap Exp $";

#include "faultstickset.h"

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

    
FaultStickSet::FaultStickSet()
    : firstrow_(0)
{}


FaultStickSet::~FaultStickSet()
{
    deepErase( sticks_ );
}


Element* FaultStickSet::clone() const
{
    FaultStickSet* res = new FaultStickSet;
    deepCopy( res->sticks_, sticks_ );
    res->firstcols_ = firstcols_;
    res->firstrow_ = firstrow_;
    res->editplanenormals_ = editplanenormals_;

    return res;
}



bool FaultStickSet::insertStick( const Coord3& firstpos, 
				     const Coord3& editnormal, int sticknr,
				     int firstcol )
{
    if ( !firstpos.isDefined() )
	return false;
    if ( !editnormal.isDefined() || mIsZero(editnormal.sqAbs(),mDefEps) )
	return false;

    const Coord3 normvec = editnormal.normalize();

    if ( sticks_.isEmpty() )
	firstrow_ = sticknr;

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


bool FaultStickSet::removeStick( int sticknr )
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


bool FaultStickSet::insertKnot( const RCol& rc, const Coord3& pos )
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


bool FaultStickSet::removeKnot( const RCol& rc )
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


#define mEmptyInterval() StepInterval<int>( mUdf(int), -mUdf(int), mUdf(int) )

StepInterval<int> FaultStickSet::rowRange() const
{
    if ( sticks_.isEmpty() )
	return mEmptyInterval();
    
    return StepInterval<int>( firstrow_, firstrow_+sticks_.size()-1, 1 ); 
}


StepInterval<int> FaultStickSet::colRange( int sticknr ) const
{
    mGetValidStickIdx( stickidx, sticknr, 0, mEmptyInterval() );

    const int firstcol = firstcols_[stickidx];
    return StepInterval<int>(firstcol, firstcol+sticks_[stickidx]->size()-1, 1);
}


bool FaultStickSet::setKnot( const RCol& rc, const Coord3& pos )
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


Coord3 FaultStickSet::getKnot( const RCol& rc ) const
{
    mGetValidStickIdx( stickidx, rc.r(), 0, Coord3::udf() );
    mGetValidKnotIdx( knotidx, rc.c(), stickidx, 0, Coord3::udf() );
    
    return (*sticks_[stickidx])[knotidx];
}


bool FaultStickSet::isKnotDefined( const RCol& rc ) const
{
    mGetValidStickIdx( stickidx, rc.r(), 0, false );
    mGetValidKnotIdx( knotidx, rc.c(), stickidx, 0, false );

    return true;
}


const Coord3& FaultStickSet::getEditPlaneNormal( int sticknr ) const
{
    mGetValidStickIdx( stickidx, sticknr, 0, Coord3::udf() );
    if ( stickidx < editplanenormals_.size() )
	return editplanenormals_[stickidx];

    return Coord3::udf();
}


void FaultStickSet::addEditPlaneNormal( const Coord3& editnormal )
{
    editplanenormals_ += editnormal;

    if ( editplanenormals_.size() > 1 )
	return;
    if ( !editnormal.isDefined() || mIsZero(editnormal.sqAbs(),mDefEps) )
	return;

    const Coord3 normvec = editnormal.normalize();
}


void FaultStickSet::addUdfRow( int sticknr, int firstknotnr, int nrknots )
{
    if ( isEmpty() )
	firstrow_ = sticknr;
    firstcols_ += firstknotnr;
    sticks_ += new TypeSet<Coord3>( nrknots, Coord3::udf() );
}


} // namespace Geometry
