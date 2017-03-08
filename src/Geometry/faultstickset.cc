/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : J.C. Glas
 * DATE     : November 2008
-*/


#include "faultstickset.h"

#include "trigonometry.h"
#include <math.h>

namespace Geometry
{

FaultStick::FaultStick()
    : curknotidnr_(0)
    , firstcol_(0)
    , planenormal_(Coord3::udf())
    , stickstatus_(0)
{}


FaultStick::FaultStick( int firstcol, Coord3 plnormal, unsigned int status,
			int sz )
    : curknotidnr_(0)
    , firstcol_(firstcol)
    , planenormal_(plnormal)
    , stickstatus_(status)
{
    stickcoords_.setSize( sz );
    knotstatus_.setSize( sz );
    knotids_.setSize( sz );
}


FaultStick::~FaultStick()
{
    stickcoords_.setEmpty();
    knotstatus_.setEmpty();
    knotids_.setEmpty();
}


void FaultStick::addKnot( const Coord3& knot, unsigned int status )
{
    stickcoords_.add( knot );
    knotstatus_.add( status );
    knotids_.add( KnotID::get(curknotidnr_++) );
}


void FaultStick::setKnot( int idx, const Coord3& knot, unsigned int status )
{
    if ( idx<size() )
    {
	stickcoords_[idx] = knot;
	knotstatus_[idx] = status;
	knotids_[idx] = KnotID::get(curknotidnr_++);
	return;
    }

    addKnot( knot, status );
}


void FaultStick::removeKnot( int idx )
{
    if ( idx >= size() )
	return;

    stickcoords_.removeSingle( idx );
    knotstatus_.removeSingle( idx );
    knotids_.removeSingle( idx );
}


Coord3 FaultStick::getKnot( int idx ) const
{
    return idx < size() ? stickcoords_[idx] : Coord3::udf();
}


unsigned int FaultStick::knotStat( int idx ) const
{
    return idx < knotstatus_.size() ? knotstatus_[idx] : mUdf(int);
}


void FaultStick::setKnotStat( int idx, unsigned int status )
{
    if ( idx<knotstatus_.size() )
    {
	knotstatus_[idx] = status;
    }
}


int FaultStick::size() const
{
    return stickcoords_.size();
}


#define mGetValidStickIdx( stickidx, sticknr, extra, errorres ) \
\
    int stickidx = sticknr - firstrow_; \
    if ( stickidx<-extra || stickidx>=sticks_.size()+extra ) \
	return errorres;

#define mGetValidKnotIdx( knotidx, knotnr, stickidx, extra, errorres ) \
\
    if ( sticks_[stickidx]->isEmpty() ) return errorres; \
    int knotidx = knotnr - sticks_[stickidx]->firstCol(); \
    if ( knotidx<-extra || knotidx>=sticks_[stickidx]->size()+extra ) \
	return errorres;

#define mMaxSceneIdx 7
#define mGetValidHiddenMask( hiddenmask, sceneidx, errorres ) \
\
    if ( sceneidx<-1 || sceneidx>mMaxSceneIdx ) \
	return errorres; \
\
    const unsigned int hiddenmask = HiddenLowestBit<<(sceneidx+1);


FaultStickSet::FaultStickSet()
    : firstrow_(0)
    , curstickidnr_(0)
{}


FaultStickSet::~FaultStickSet()
{
    deepErase( sticks_ );
}


Element* FaultStickSet::clone() const
{
    FaultStickSet* res = new FaultStickSet;
    deepCopy( res->sticks_, sticks_ );
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

    FaultStick* newstick = new FaultStick( firstcol, normvec, NoStatus );
    if ( stickidx==sticks_.size() )
    {
	sticks_ += newstick;
	stickids_ += StickID::get(curstickidnr_++);
    }
    else
    {
	sticks_.insertAt( newstick, stickidx );
	stickids_.insert( stickidx, StickID::get(curstickidnr_++) );
    }

    newstick->setKnot( 0, firstpos, NoStatus );
    triggerNrPosCh( GeomPosID::getFromRowCol(stickidx,StickInsert) );
    if ( blocksCallBacks() )
	blockCallBacks( true, true );

    return true;
}


bool FaultStickSet::removeStick( int sticknr )
{
    mGetValidStickIdx( stickidx, sticknr, 0, false );

    delete sticks_.removeSingle( stickidx );

    if ( !stickidx )
	firstrow_++;

    triggerNrPosCh( GeomPosID::getFromRowCol(stickidx,StickRemove) );
    if ( blocksCallBacks() )
	blockCallBacks( true, true );

    return true;
}


bool FaultStickSet::insertKnot( const RowCol& rc, const Coord3& pos )
{
    if ( !pos.isDefined() )
	return false;

    mGetValidStickIdx( stickidx, rc.row(), 0, false );
    mGetValidKnotIdx( knotidx, rc.col(), stickidx, 1, false );
    if ( knotidx==-1 )
    {
	sticks_[stickidx]->firstCol()--;
	knotidx++;
    }

    if ( knotidx==sticks_[stickidx]->size() )
	sticks_[stickidx]->addKnot( pos, NoStatus );
    else
	sticks_[stickidx]->setKnot( knotidx, pos, NoStatus );

    triggerNrPosCh( GeomPosID::getFromRowCol(stickidx,StickChange) );
    return true;
}


bool FaultStickSet::removeKnot( const RowCol& rc )
{
    mGetValidStickIdx( stickidx, rc.row(), 0, false );
    mGetValidKnotIdx( knotidx, rc.col(), stickidx, 0, false );

    if ( sticks_[stickidx]->size() <= 1 )
	return removeStick( rc.row() );

    sticks_[stickidx]->removeKnot( knotidx );

    if ( !knotidx )
	sticks_[stickidx]->firstCol()++;

    triggerNrPosCh( GeomPosID::getFromRowCol(stickidx,StickChange) );

    return true;
}


int FaultStickSet::nrSticks() const
{ return sticks_.size(); }

const TypeSet<Coord3>* FaultStickSet::getStick( int stickidx ) const
{
   if ( stickidx<0 || stickidx>=sticks_.size() )
      return 0;

    return &sticks_[stickidx]->stickCoords();
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

    const int& firstcol = sticks_[stickidx]->firstCol();
    return StepInterval<int>(firstcol, firstcol+sticks_[stickidx]->size()-1, 1);
}


bool FaultStickSet::setKnot( const RowCol& rc, const Coord3& pos )
{
    if ( !pos.isDefined() )
	return removeKnot( rc );

    mGetValidStickIdx( stickidx, rc.row(), 0, false );
    mGetValidKnotIdx( knotidx, rc.col(), stickidx, 0, false );

    sticks_[stickidx]->setKnot( knotidx, pos );
    triggerMovement( GeomPosID::getFromRowCol(stickidx,StickChange) );
    return true;
}


Coord3 FaultStickSet::getKnot( const RowCol& rc ) const
{
    mGetValidStickIdx( stickidx, rc.row(), 0, Coord3::udf() );
    mGetValidKnotIdx( knotidx, rc.col(), stickidx, 0, Coord3::udf() );

    return sticks_[stickidx]->getKnot( knotidx );
}


bool FaultStickSet::isKnotDefined( const RowCol& rc ) const
{
    mGetValidStickIdx( stickidx, rc.row(), 0, false );
    mGetValidKnotIdx( knotidx, rc.col(), stickidx, 0, false );

    return true;
}


Coord3 FaultStickSet::getEditPlaneNormal( int sticknr ) const
{
    mGetValidStickIdx( stickidx, sticknr, 0, Coord3::udf() );
    if ( stickidx < sticks_.size() )
	return  sticks_[stickidx]->planeNormal();

    return Coord3::udf();
}


void FaultStickSet::setEditPlaneNormal( int sticknr, const Coord3& editnormal )
{
    mGetValidStickIdx( stickidx, sticknr, 0, );
    if ( stickidx < sticks_.size() )
	sticks_[stickidx]->setPlaneNormal( editnormal );
}


void FaultStickSet::addUdfRow( int sticknr, int firstknotnr, int nrknots )
{
    if ( isEmpty() )
	firstrow_ = sticknr;

    FaultStick* newstick = new FaultStick( firstknotnr, Coord3::udf(),
					    NoStatus, nrknots );
    sticks_ += newstick;
    stickids_ += StickID::get(curstickidnr_++);
}


static double pointToSegmentDist( const Coord3& point,
				 const Coord3& end1, const Coord3& end2 )
{
    double d01 = point.distTo<double>( end1 );
    double d02 = point.distTo<double>( end2 );
    double d12 = end1.distTo<double>( end2 );

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
	a0.x_=0; a0.y_=0; a1.x_=0; a1.y_=0; b0.x_=0; b0.y_=0; b1.x_=0; b1.y_=0;\
    } \
    else \
    { \
	a0.z_ *= zscale; a1.z_ *= zscale; b0.z_ *= zscale; b1.z_ *= zscale; \
    } \

double FaultStickSet::interStickDist( int sticknr1, int sticknr2,
				      double zscale ) const
{
    mGetEndPoints( sticknr1, sticknr2, zscale, a0, a1, b0, b1, mUdf(double) );
    if ( knotrg1.start==knotrg1.stop )
    {
	Line3 stick( b0, b1 );
	return stick.distanceToPoint( a0 );
    }
    else if ( knotrg2.start==knotrg2.stop )
    {
	Line3 stick( a0, a1 );
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
    return (a0.distTo<float>(b0)+a1.distTo<float>(b1) >
	    a0.distTo<float>(b1)+a1.distTo<float>(b0));
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
    unsigned int stat = sticks_[stickidx]->stickStat();
    if ( yn )
	stat |= Selected;
    else
	stat &= ~Selected;

    sticks_[stickidx]->setStickStat( stat );
}


bool FaultStickSet::isStickSelected( int sticknr ) const
{
    mGetValidStickIdx( stickidx, sticknr, 0, false );
    return sticks_[stickidx]->stickStat() & Selected;
}


void FaultStickSet::hideStick( int sticknr, bool yn, int sceneidx )
{
    if ( yn == isStickHidden(sticknr,sceneidx) )
	return;

    mGetValidStickIdx( stickidx, sticknr, 0, );
    mGetValidHiddenMask( hiddenmask, sceneidx, );

    unsigned int stat = sticks_[stickidx]->stickStat();
    if ( yn )
	stat |= hiddenmask;
    else
	stat &= ~hiddenmask;

    sticks_[stickidx]->setStickStat( stat );
    triggerNrPosCh( GeomPosID::getFromRowCol(stickidx,StickHide) );
}


bool FaultStickSet::isStickHidden( int sticknr, int sceneidx ) const
{
    mGetValidStickIdx( stickidx, sticknr, 0, false );
    mGetValidHiddenMask( hiddenmask, sceneidx, false );
    return sticks_[stickidx]->stickStat() & hiddenmask;
}


void FaultStickSet::preferStick( int sticknr )
{
    for ( int idx=0; idx<sticks_.size(); idx++ )
    {
	unsigned int stat = sticks_[idx]->stickStat();
	stat &= ~Preferred;
	sticks_[idx]->setStickStat( stat );
    }

    mGetValidStickIdx( stickidx, sticknr, 0, );
    unsigned int stt = sticks_[stickidx]->stickStat();
    stt |= Preferred;
    sticks_[stickidx]->setStickStat( stt );
}


int FaultStickSet::preferredStickNr() const
{
    for ( int idx=0; idx<sticks_.size(); idx++ )
    {
	if ( sticks_[idx]->stickStat() & Preferred )
	    return firstrow_+idx;
    }

    return  mUdf(int);
}


void FaultStickSet::hideKnot( const RowCol& rc, bool yn, int sceneidx )
{
    if ( yn == isKnotHidden(rc,sceneidx) )
	return;

    mGetValidStickIdx( stickidx, rc.row(), 0, );
    mGetValidKnotIdx( knotidx, rc.col(), stickidx, 0, );
    mGetValidHiddenMask( hiddenmask, sceneidx, );

    unsigned int knotstat = sticks_[stickidx]->knotStat( knotidx );
    if ( yn )
	knotstat |= hiddenmask;
    else
	knotstat &= ~hiddenmask;

    sticks_[stickidx]->setKnotStat( knotidx, knotstat );
    // TODO if also sticks of faults are pruned for display only at sections:
    // triggerNrPosCh( GeomPosID::getFromRowCol(stickidx,KnotHide) );
}


bool FaultStickSet::isKnotHidden( const RowCol& rc, int sceneidx ) const
{
    mGetValidStickIdx( stickidx, rc.row(), 0, false );
    mGetValidKnotIdx( knotidx, rc.col(), stickidx, 0, false );
    mGetValidHiddenMask( hiddenmask, sceneidx, false );
    return sticks_[stickidx]->knotStat( knotidx ) & hiddenmask;
}


void FaultStickSet::hideAllSticks( bool yn, int sceneidx )
{
    for ( int idx=0; idx<nrSticks(); idx++ )
	hideStick( idx, yn, sceneidx );
}


void FaultStickSet::hideAllKnots( bool yn, int sceneidx )
{
    for ( int idx=0; idx<nrSticks(); idx++ )
    {
	for( int idy=0; idy<nrKnots(idx); idy++ )
	    hideKnot( RowCol(idx,idy), yn, sceneidx );
    }
}


} // namespace Geometry
