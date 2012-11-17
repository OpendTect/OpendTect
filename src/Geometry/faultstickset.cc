/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : J.C. Glas
 * DATE     : November 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "faultstickset.h"

#include "trigonometry.h"
#include <math.h>

namespace Geometry
{


#define mGetValidStickIdx( stickidx, sticknr, extra, errorres ) \
\
    int stickidx = sticknr - firstrow_; \
    if ( stickidx<-extra || stickidx>=sticks_.size()+extra ) \
	return errorres;

#define mGetValidKnotIdx( knotidx, knotnr, stickidx, extra, errorres ) \
\
    if ( !firstcols_.size() ) return errorres; \
    int knotidx = knotnr - firstcols_[stickidx]; \
    if ( knotidx<-extra || knotidx>=sticks_[stickidx]->size()+extra ) \
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
    res->stickstatus_ = stickstatus_;

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
	stickstatus_ += NoStatus;
	firstcols_ += firstcol;
    }
    else
    {
	sticks_.insertAt( new TypeSet<Coord3>, stickidx );
	editplanenormals_.insert( stickidx, normvec );
	stickstatus_.insert( stickidx, NoStatus );
	firstcols_.insert( stickidx, firstcol );
    }

    sticks_[stickidx]->insert( 0, firstpos );

    triggerNrPosCh( RowCol(stickidx,StickInsert).toInt64() );
    if ( blocksCallBacks() )
	blockCallBacks( true, true );

    return true;
}


bool FaultStickSet::removeStick( int sticknr )
{
    mGetValidStickIdx( stickidx, sticknr, 0, false );

    sticks_.removeSingle( stickidx );
    editplanenormals_.removeSingle( stickidx );
    stickstatus_.removeSingle( stickidx );
    firstcols_.removeSingle( stickidx );

    if ( !stickidx )
	firstrow_++;

    triggerNrPosCh( RowCol(stickidx,StickRemove).toInt64() );
    if ( blocksCallBacks() )
	blockCallBacks( true, true );
    
    return true;
}


bool FaultStickSet::insertKnot( const RowCol& rc, const Coord3& pos )
{
    if ( !pos.isDefined() )
	return false;

    mGetValidStickIdx( stickidx, rc.row, 0, false );
    mGetValidKnotIdx( knotidx, rc.col, stickidx, 1, false );
    if ( knotidx==-1 )
    {
	firstcols_[stickidx]--;
	knotidx++;
    }

    if ( knotidx==sticks_[stickidx]->size() )
	(*sticks_[stickidx]) += pos;
    else
	sticks_[stickidx]->insert( knotidx, pos );

    triggerNrPosCh( RowCol(stickidx,StickChange).toInt64() );

    return true;
}


bool FaultStickSet::removeKnot( const RowCol& rc )
{
    mGetValidStickIdx( stickidx, rc.row, 0, false );
    mGetValidKnotIdx( knotidx, rc.col, stickidx, 0, false );

    if ( sticks_[stickidx]->size() <= 1 )
	return removeStick( rc.row );

    sticks_[stickidx]->removeSingle( knotidx );

    if ( !knotidx )
	firstcols_[stickidx]++;

    triggerNrPosCh( RowCol(stickidx,StickChange).toInt64() );
    
    return true;
}


int FaultStickSet::nrSticks() const
{ return sticks_.size(); }

const TypeSet<Coord3>* FaultStickSet::getStick( int stickidx ) const
{
   if ( stickidx<0 || stickidx>=sticks_.size() )
      return 0;

    return sticks_[stickidx];
}

int FaultStickSet::nrKnots( int sticknr ) const
{
    mGetValidStickIdx( stickidx, sticknr, 0, 0 );
    return sticks_[stickidx]->size();
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


bool FaultStickSet::setKnot( const RowCol& rc, const Coord3& pos )
{
    if ( !pos.isDefined() )
    {
	return removeKnot( rc );
    }

    mGetValidStickIdx( stickidx, rc.row, 0, false );
    mGetValidKnotIdx( knotidx, rc.col, stickidx, 0, false );

    (*sticks_[stickidx])[knotidx] = pos;
    triggerMovement( RowCol(stickidx,StickChange).toInt64() );
    return true;
}


Coord3 FaultStickSet::getKnot( const RowCol& rc ) const
{
    mGetValidStickIdx( stickidx, rc.row, 0, Coord3::udf() );
    mGetValidKnotIdx( knotidx, rc.col, stickidx, 0, Coord3::udf() );
    
    return (*sticks_[stickidx])[knotidx];
}


bool FaultStickSet::isKnotDefined( const RowCol& rc ) const
{
    mGetValidStickIdx( stickidx, rc.row, 0, false );
    mGetValidKnotIdx( knotidx, rc.col, stickidx, 0, false );

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
}


void FaultStickSet::addUdfRow( int sticknr, int firstknotnr, int nrknots )
{
    if ( isEmpty() )
	firstrow_ = sticknr;
    firstcols_ += firstknotnr;
    stickstatus_ += NoStatus;
    sticks_ += new TypeSet<Coord3>( nrknots, Coord3::udf() );
}


static double pointToSegmentDist( const Coord3& point,
				 const Coord3& end1, const Coord3& end2 )
{
    double d01 = point.distTo( end1 );
    double d02 = point.distTo( end2 );
    double d12 = end1.distTo( end2 );

    if ( mIsZero(d12,mDefEps) )
	return d01;
    if ( d01<d02 && d01*d01+d12*d12<=d02*d02 )
	return d01;
    if ( d01>d02 && d02*d02+d12*d12<=d01*d01 )
	return d02;

    double sp = 0.5 *( d01+d02+d12 );
    double area = Math::Sqrt( sp*(sp-d01)*(sp-d02)*(sp-d12) );  
    return 2.0*area/d12;
}


#define mGetEndPoints( sticknr1, sticknr2, zscale, a0, a1, b0, b1, errres ) \
\
    StepInterval<int> knotrg1 = colRange( sticknr1 ); \
    StepInterval<int> knotrg2 = colRange( sticknr2 ); \
    if ( knotrg1.isUdf() || knotrg2.isUdf() ) \
	return errres; \
\
    Coord3 a0 = getKnot( RowCol(sticknr1,knotrg1.start) ); \
    Coord3 a1 = getKnot( RowCol(sticknr1,knotrg1.stop) ); \
    Coord3 b0 = getKnot( RowCol(sticknr2,knotrg2.start) ); \
    Coord3 b1 = getKnot( RowCol(sticknr2,knotrg2.stop) ); \
\
    if ( zscale==MAXDOUBLE ) \
    { \
	a0.x=0; a0.y=0; a1.x=0; a1.y=0; b0.x=0; b0.y=0; b1.x=0; b1.y=0; \
    } \
    else \
    { \
	a0.z *= zscale; a1.z *= zscale; b0.z *= zscale; b1.z *= zscale; \
    } \

double FaultStickSet::interStickDist( int sticknr1, int sticknr2,
				      double zscale ) const
{
    mGetEndPoints( sticknr1, sticknr2, zscale, a0, a1, b0, b1, mUdf(double) );
    if ( knotrg1.start==knotrg1.stop )
    {
	Line3 stick( b0, b1-b0 );
	return stick.distanceToPoint( a0 );
    }
    else if ( knotrg2.start==knotrg2.stop )
    {
	Line3 stick( a0, a1-a0 );
	return stick.distanceToPoint( b0 );
    }

    const double dista0 = pointToSegmentDist( a0, b0, b1 );
    const double dista1 = pointToSegmentDist( a1, b0, b1 );
    const double distb0 = pointToSegmentDist( b0, a0, a1 );
    const double distb1 = pointToSegmentDist( b1, a0, a1 );

    return mMIN( mMIN(dista0,dista1), mMIN(distb0,distb1) );
}


bool FaultStickSet::isTwisted( int sticknr1, int sticknr2, double zscale ) const
{
    mGetEndPoints( sticknr1, sticknr2, zscale, a0, a1, b0, b1, false );
    return (a0.distTo(b0)+a1.distTo(b1) > a0.distTo(b1)+a1.distTo(b0));
}


void FaultStickSet::geometricStickOrder( TypeSet<int>& sticknrs,
					 double zscale, bool orderall ) const
{
    StepInterval<int> rowrg = rowRange();
    if ( rowrg.isUdf() )
    {
	sticknrs.erase();
	return;
    }

    if ( orderall )
    {
	sticknrs.erase();
	for (int sticknr=rowrg.start; sticknr<=rowrg.stop; sticknr+=rowrg.step)
	    sticknrs += sticknr;
    }
    else
    {
	for ( int idx=sticknrs.size()-1; idx>=0; idx-- )
	{
	    if ( !rowrg.includes(sticknrs[idx],false) )
		sticknrs.removeSingle( idx );
	}
    }

    if ( sticknrs.size() < 3 )
	return;

    double mindist = MAXDOUBLE;
    int minidx0 = -1, minidx1 = -1;

    for ( int idx=0; idx<sticknrs.size()-1; idx++ )
    {
	for ( int idy=idx+1; idy<sticknrs.size(); idy++ )
	{
	    const double dist = interStickDist( sticknrs[idx],
						sticknrs[idy], zscale );

	    if ( dist < mindist )
	    {
		mindist = dist;
		minidx0 = idx;
		minidx1 = idy;
	    }
	}
    }
    sticknrs.swap( 0, minidx0 );
    sticknrs.swap( 1, minidx1 );

    for ( int tailidx=1; tailidx<sticks_.size()-1; tailidx++ )
    {
	mindist = MAXDOUBLE;
	bool reverse = false;
	for ( int idy=tailidx+1; idy<sticknrs.size(); idy++ )
	{
	    const double dist0 = interStickDist( sticknrs[0],
		    				 sticknrs[idy], zscale );
	    const double dist1 = interStickDist( sticknrs[tailidx],
		    				 sticknrs[idy], zscale );

	    if ( mMIN(dist0,dist1) < mindist )
	    {
		mindist = mMIN( dist0, dist1 );
		minidx0 = idy;
		reverse = dist0 < dist1;
	    }
	}
	for ( int idx=0; reverse && idx<tailidx*0.5; idx++ )
	    sticknrs.swap( idx, tailidx-idx );

	sticknrs.swap( tailidx+1, minidx0 );
    }
}


void FaultStickSet::selectStick( int sticknr, bool yn )
{
    mGetValidStickIdx( stickidx, sticknr, 0, );
    if ( yn )
	stickstatus_[stickidx] |= Selected;
    else
	stickstatus_[stickidx] &= ~Selected;
}


bool FaultStickSet::isStickSelected( int sticknr ) const
{
    mGetValidStickIdx( stickidx, sticknr, 0, false );
    return stickstatus_[stickidx] & Selected;
}


void FaultStickSet::hideStick( int sticknr, bool yn )
{
    if ( yn == isStickHidden(sticknr) )
	return;

    mGetValidStickIdx( stickidx, sticknr, 0, );

    if ( yn )
	stickstatus_[stickidx] |= Hidden;
    else
	stickstatus_[stickidx] &= ~Hidden;

    triggerNrPosCh( RowCol(stickidx,StickHide).toInt64() );
}


bool FaultStickSet::isStickHidden( int sticknr ) const
{
    mGetValidStickIdx( stickidx, sticknr, 0, false );
    return stickstatus_[stickidx] & Hidden;
}


void FaultStickSet::preferStick( int sticknr )
{
    for ( int idx=0; idx<stickstatus_.size(); idx++ )
	stickstatus_[idx] &= ~Preferred;

    mGetValidStickIdx( stickidx, sticknr, 0, );
    stickstatus_[stickidx] |= Preferred;
}


int FaultStickSet::preferredStickNr() const
{
    for ( int idx=0; idx<stickstatus_.size(); idx++ )
    {
	if ( stickstatus_[idx] & Preferred )
	    return firstrow_+idx;
    }

    return  mUdf(int);
}


} // namespace Geometry
