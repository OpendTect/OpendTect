/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "faultstickset.h"

#include "survinfo.h"
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

#define mMaxSceneIdx 7
#define mGetValidHiddenMask( hiddenmask, sceneidx, errorres ) \
\
    if ( sceneidx<-1 || sceneidx>mMaxSceneIdx ) \
	return errorres; \
\
    const unsigned int hiddenmask = HiddenLowestBit<<(sceneidx+1);


// FaultStick
FaultStick::FaultStick( int idx )
    : stickidx_(idx)
{
    int x = 0;
}


FaultStick::~FaultStick()
{}


int FaultStick::getStickIdx() const
{
    return stickidx_;
}


void FaultStick::setNormal( const Coord3 crd )
{
    normal_ = crd;
}


const Coord3& FaultStick::getNormal() const
{
    // TODO: Determine edit normal for sticks picked on 2D lines

    if ( !normal_.isUdf() )
	return normal_;

    const int maxdist = 5;
    int oninl = 0; int oncrl = 0; int ontms = 0;

    for ( int idx=0; idx<locs_.size()-1; idx++ )
    {

	const BinID bid0 = locs_[idx].trcKey().position();
	for ( int idy=idx+1; idy<locs_.size(); idy++ )
	{
	    const BinID bid1 = locs_[idy].trcKey().position();
	    const int inldist = abs( bid0.inl()-bid1.inl() );
	    if ( inldist < maxdist )
		oninl += maxdist - inldist;
	    const int crldist = abs( bid0.crl()-bid1.crl() );
	    if ( crldist < maxdist )
		oncrl += maxdist - crldist;
	    const int zdist =
		mNINT32( fabs(locs_[idx].pos().z-locs_[idy].pos().z) /
		    fabs(SI().zStep()) );
	    if ( zdist < maxdist )
		ontms += maxdist - zdist;
	}
    }

    const bool is2d = geomid_.is2D();
    if ( ontms>oncrl && ontms>oninl && !is2d )
	normal_ = Coord3( 0, 0, 1 );
    else if ( oncrl > oninl )
    {
	normal_ = Coord3( SI().binID2Coord().crlDir(), 0 );
    }
    else
	normal_ = Coord3( SI().binID2Coord().inlDir(), 0 );

    return normal_;
}


void FaultStick::setLocationsFromCrds( const Coord3* crds, int sz,
						    const Pos::GeomID& geomid )
{
    for ( int idx=0; idx<sz; idx++ )
	locs_.add( LocationBase(crds[idx],geomid) );
}


int FaultStick::size() const
{
    return locs_.size();
}


const Coord3& FaultStick::getCoordAtIndex( int idx ) const
{
    if ( idx >= locs_.size() || idx < 0 )
	return Coord3::udf();

    return locs_[idx].pos();
}


//FaultStickSet
FaultStickSet::FaultStickSet()
    : firstrow_(0)
{}


FaultStickSet::~FaultStickSet()
{
    deepErase( sticks_ );
    deepErase( knotstatus_ );
}


Element* FaultStickSet::clone() const
{
    FaultStickSet* res = new FaultStickSet;
    deepCopy( res->sticks_, sticks_ );
    res->firstcols_ = firstcols_;
    res->firstrow_ = firstrow_;
    res->stickstatus_ = stickstatus_;
    deepCopy( res->knotstatus_, knotstatus_ );

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

    auto* fs = new FaultStick( stickidx );
    fs->setNormal( normvec );
    if ( stickidx==sticks_.size() )
    {

	sticks_ += fs;
	stickstatus_ += NoStatus;
	firstcols_ += firstcol;
	knotstatus_ += new TypeSet<unsigned int>;
    }
    else
    {
	sticks_.insertAt( fs, stickidx );
	stickstatus_.insert( stickidx, NoStatus );
	firstcols_.insert( stickidx, firstcol );
	knotstatus_.insertAt( new TypeSet<unsigned int>, stickidx );
    }

    sticks_[stickidx]->locs_.insert( 0, firstpos );
    knotstatus_[stickidx]->insert( 0, NoStatus );

    triggerNrPosCh( RowCol(stickidx,StickInsert).toInt64() );
    if ( blocksCallBacks() )
	blockCallBacks( true, true );

    return true;
}


bool FaultStickSet::removeStick( int sticknr )
{
    mGetValidStickIdx( stickidx, sticknr, 0, false );

    delete sticks_.removeSingle( stickidx );
    stickstatus_.removeSingle( stickidx );
    firstcols_.removeSingle( stickidx );
    delete knotstatus_.removeSingle( stickidx );

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

    mGetValidStickIdx( stickidx, rc.row(), 0, false );
    mGetValidKnotIdx( knotidx, rc.col(), stickidx, 1, false );
    if ( knotidx==-1 )
    {
	firstcols_[stickidx]--;
	knotidx++;
    }

    if ( knotidx==sticks_[stickidx]->size() )
    {
	(*sticks_[stickidx]).locs_.add( pos );
	(*knotstatus_[stickidx]) += NoStatus;
    }
    else
    {
	sticks_[stickidx]->locs_.insert( knotidx, pos );
	knotstatus_[stickidx]->insert( knotidx, NoStatus );
    }

    triggerNrPosCh( RowCol(stickidx,StickChange).toInt64() );

    return true;
}


bool FaultStickSet::removeKnot( const RowCol& rc )
{
    mGetValidStickIdx( stickidx, rc.row(), 0, false );
    mGetValidKnotIdx( knotidx, rc.col(), stickidx, 0, false );

    if ( sticks_[stickidx]->size() <= 1 )
	return removeStick( rc.row() );

    sticks_[stickidx]->locs_.removeSingle( knotidx );
    knotstatus_[stickidx]->removeSingle( knotidx );

    if ( !knotidx )
	firstcols_[stickidx]++;

    triggerNrPosCh( RowCol(stickidx,StickChange).toInt64() );
    
    return true;
}


int FaultStickSet::nrSticks() const
{ return sticks_.size(); }

const FaultStick* FaultStickSet::getStick( int stickidx ) const
{
   if ( stickidx<0 || stickidx>=sticks_.size() )
      return nullptr;

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
	return removeKnot( rc );

    mGetValidStickIdx( stickidx, rc.row(), 0, false );
    mGetValidKnotIdx( knotidx, rc.col(), stickidx, 0, false );

    (*sticks_[stickidx]).locs_[knotidx] = pos;
    triggerMovement( RowCol(stickidx,StickChange).toInt64() );
    return true;
}


Coord3 FaultStickSet::getKnot( const RowCol& rc ) const
{
    mGetValidStickIdx( stickidx, rc.row(), 0, Coord3::udf() );
    mGetValidKnotIdx( knotidx, rc.col(), stickidx, 0, Coord3::udf() );
    
    return (*sticks_[stickidx]).locs_[knotidx].pos();
}


bool FaultStickSet::isKnotDefined( const RowCol& rc ) const
{
    mGetValidStickIdx( stickidx, rc.row(), 0, false );
    mGetValidKnotIdx( knotidx, rc.col(), stickidx, 0, false );

    return true;
}


const Coord3& FaultStickSet::getEditPlaneNormal( int sticknr ) const
{
    mGetValidStickIdx( stickidx, sticknr, 0, Coord3::udf() );
    if ( stickidx < sticks_.size() )
	return sticks_[stickidx]->getNormal();

    return Coord3::udf();
}


void FaultStickSet::addEditPlaneNormal( const Coord3& editnormal, int sticknr )
{
    mGetValidStickIdx( stickidx, sticknr, 0,  );
    if ( stickidx < sticks_.size() )
	return sticks_[stickidx]->setNormal( editnormal );
}


void FaultStickSet::addUdfRow( int sticknr, int firstknotnr, int nrknots )
{
    if ( isEmpty() )
	firstrow_ = sticknr;

    firstcols_ += firstknotnr;
    stickstatus_ += NoStatus;
    FaultStick* stick = new FaultStick( sticks_.size() );
    stick->locs_.setSize( nrknots );
    sticks_ += stick;
    knotstatus_ += new TypeSet<unsigned int>( nrknots, NoStatus );
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

	if ( tailidx < sticknrs.size()-1 )
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


void FaultStickSet::hideStick( int sticknr, bool yn, int sceneidx )
{
    if ( yn == isStickHidden(sticknr,sceneidx) )
	return;

    mGetValidStickIdx( stickidx, sticknr, 0, );
    mGetValidHiddenMask( hiddenmask, sceneidx, );

    if ( yn )
	stickstatus_[stickidx] |= hiddenmask;
    else
	stickstatus_[stickidx] &= ~hiddenmask;

    triggerNrPosCh( RowCol(stickidx,StickHide).toInt64() );
}


bool FaultStickSet::isStickHidden( int sticknr, int sceneidx ) const
{
    mGetValidStickIdx( stickidx, sticknr, 0, false );
    mGetValidHiddenMask( hiddenmask, sceneidx, false );
    return stickstatus_[stickidx] & hiddenmask;
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


void FaultStickSet::hideKnot( const RowCol& rc, bool yn, int sceneidx )
{
    if ( yn == isKnotHidden(rc,sceneidx) )
	return;

    mGetValidStickIdx( stickidx, rc.row(), 0, );
    mGetValidKnotIdx( knotidx, rc.col(), stickidx, 0, );
    mGetValidHiddenMask( hiddenmask, sceneidx, );

    if ( yn )
	(*knotstatus_[stickidx])[knotidx] |= hiddenmask;
    else
	(*knotstatus_[stickidx])[knotidx] &= ~hiddenmask;

    // TODO if also sticks of faults are pruned for display only at sections:
    // triggerNrPosCh( RowCol(stickidx,KnotHide).toInt64() );
}


bool FaultStickSet::isKnotHidden( const RowCol& rc, int sceneidx ) const
{
    mGetValidStickIdx( stickidx, rc.row(), 0, false );
    mGetValidKnotIdx( knotidx, rc.col(), stickidx, 0, false );
    mGetValidHiddenMask( hiddenmask, sceneidx, false );
    return (*knotstatus_[stickidx])[knotidx] & hiddenmask;
}


} // namespace Geometry
