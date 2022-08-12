/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2008
________________________________________________________________________

-*/

#include "faulttrace.h"

#include "binidvalset.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emfaultset3d.h"
#include "emhorizon.h"
#include "emhorizon2d.h"
#include "emsurfacegeometry.h"
#include "emmanager.h"
#include "emsurfaceiodata.h"
#include "envvars.h"
#include "executor.h"
#include "explfaultsticksurface.h"
#include "explplaneintersection.h"
#include "faultsticksurface.h"
#include "ioman.h"
#include "ioobj.h"
#include "posinfo2d.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "uistrings.h"

static bool allowTrackingAcrossCrossFaults()
{
    mDefineStaticLocalObject( bool, val,
			= GetEnvVarYN("TRACK_ACROSS_CROSSTRACKED_FAULTS") );
    return val;
}


int FaultTrace::nextID( int previd ) const
{ return previd >= -1 && previd < coords_.size()-1 ? previd + 1 : -1; }

void FaultTrace::setIndices( const TypeSet<int>& indices )
{ coordindices_ = indices; }

const TypeSet<int>& FaultTrace::getIndices() const
{ return coordindices_; }

int FaultTrace::add( const Coord3& pos )
{
    Threads::Locker locker( lock_ );
    coords_ += pos;
    return coords_.size() - 1;
}


int FaultTrace::add( const Coord3& pos, float trcnr )
{
    Threads::Locker locker( lock_ );
    coords_ += pos;
    trcnrs_ += trcnr;
    return coords_.size() - 1;
}


Coord3 FaultTrace::get( int idx ) const
{ return idx >= 0 && idx < coords_.size() ? coords_[idx] : Coord3::udf(); }

float FaultTrace::getTrcNr( int idx ) const
{ return idx >= 0 && idx < trcnrs_.size() ? trcnrs_[idx] : -1; }

void FaultTrace::set( int idx, const Coord3& pos )
{
    if ( coords_.validIdx(idx) )
	coords_[idx] = pos;
}


void FaultTrace::set( int idx, const Coord3& pos, float trcnr )
{
    if ( coords_.validIdx(idx) )
    {
	coords_[idx] = pos;
	if ( trcnrs_.validIdx(idx) )
	    trcnrs_[idx] = trcnr;
    }
}


void FaultTrace::remove( int idx )
{
    Threads::Locker locker( lock_ );
    coords_.removeSingle( idx );
}


bool FaultTrace::isDefined( int idx ) const
{ return coords_.validIdx(idx) && coords_[idx].isDefined(); }


FaultTrace* FaultTrace::clone() const
{
    Threads::Locker locker( lock_ );
    FaultTrace* newobj = new FaultTrace;
    newobj->isinl_ = isinl_;
    newobj->nr_ = nr_;
    newobj->editedoncrl_ = editedoncrl_;
    newobj->coords_ = coords_;
    newobj->coordindices_ = coordindices_;
    newobj->trcnrs_ = trcnrs_;
    newobj->trcrange_ = trcrange_;
    newobj->zrange_ = zrange_;
    newobj->tracesegs_ = tracesegs_;

    return newobj;
}


bool FaultTrace::getImage( const BinID& bid, float z,
			   const Interval<float>& ztop,
			   const Interval<float>& zbot,
			   const StepInterval<int>& trcrg,
			   BinID& bidimg, float& zimg, bool posdir ) const
{
    if ( isinl_ == editedoncrl_ && !allowTrackingAcrossCrossFaults() )
	return false;

    float z1 = posdir ? ztop.start : ztop.stop;
    float z2 = posdir ? zbot.start : zbot.stop;

    const float frac = mIsEqual(z1,z2,1e-6) ? 0 : ( z - z1 ) / ( z2 - z1 );
    z1 = posdir ? ztop.stop : ztop.start;
    z2 = posdir ? zbot.stop : zbot.start;
    zimg = z1 + frac * ( z2 - z1 );
    BinID start( isinl_ ? nr_ : trcrange_.start,
		 isinl_ ? trcrange_.start : nr_ );
    BinID stop( isinl_ ? nr_ : trcrange_.stop, isinl_ ? trcrange_.stop : nr_ );
    Coord intsectn = getIntersection( start,zimg, stop, zimg );
    if ( intsectn == Coord::udf() )
	return false;

    const float fidx = trcrg.getfIndex( intsectn.x );
    const int index = posdir ? mNINT32( Math::Ceil(fidx) )
			     : mNINT32( Math::Floor(fidx) );
    bidimg.inl() = isinl_ ? nr_ : trcrg.atIndex( index );
    bidimg.crl() = isinl_ ? trcrg.atIndex( index ) : nr_;
    return true;
}


bool FaultTrace::getHorizonIntersectionInfo(
	const EM::Horizon& hor, Pos::GeomID geomid,
	TypeSet<BinID>& pos1bids, TypeSet<float>& pos1zs,
	TypeSet<BinID>& pos2bids, TypeSet<float>& pos2zs,
	TypeSet<Coord>& intersections, bool firstonly, bool allowextend ) const
{
    mDynamicCastGet(const EM::Horizon2D*, hor2d, &hor);
    const bool use2d = hor2d && geomid!=Survey::GM().cUndefGeomID();

    StepInterval<int> hortrcrg = isinl_ ? hor.geometry().colRange()
					: hor.geometry().rowRange();
    if ( use2d )
	hortrcrg = isinl_ ? hor2d->geometry().colRange( geomid )
			  : hor2d->geometry().rowRange();

    StepInterval<int> trcrg = hortrcrg;
    trcrg.limitTo( trcrange_ );

    /*If a horizon has a gap which covers the fault, extend the range to the
     nearby defined trace on the horizon. */
    for ( int idx=1; ; idx++ )
    {
	const int trcnr = trcrg.start-trcrg.step*idx;
	if ( trcnr < hortrcrg.start ) break;

	const BinID curbid( isinl_ ? nr_ : trcnr, isinl_ ? trcnr : nr_ );
	const float curz = use2d ?
	    (float) hor2d->getPos( geomid, trcnr ).z :
	    (float) hor.getPos( curbid.toInt64() ).z;

	if ( !mIsUdf(curz) )
	{
	    trcrg.start = trcnr;
	    break;
	}
    }

    for ( int idx=1; ; idx++ )
    {
	const int trcnr = trcrg.stop+trcrg.step*idx;
	if ( trcnr > hortrcrg.stop ) break;

	const BinID curbid( isinl_ ? nr_ : trcnr, isinl_ ? trcnr : nr_ );
	const float curz = use2d ?
	    (float) hor2d->getPos( geomid, trcnr ).z :
	    (float) hor.getPos( curbid.toInt64() ).z;
	if ( !mIsUdf(curz) )
	{
	    trcrg.stop = trcnr;
	    break;
	}
    }

    int prevtrc = -1;
    float firstz=mUdf(float), prevz = mUdf(float);
    BinID firstbid, prevbid;

    for ( int trcnr=trcrg.start; trcnr<=trcrg.stop; trcnr+=trcrg.step )
    {
	const BinID curbid( isinl_ ? nr_ : trcnr, isinl_ ? trcnr : nr_ );
	const float curz = use2d ?
	    (float) hor2d->getPos( geomid, trcnr ).z :
	    (float) hor.getPos( curbid.toInt64() ).z;
	if ( mIsUdf(curz) ) continue;

	if ( prevtrc==-1 )
	{
	    firstz = curz;
	    firstbid = curbid;
	}

	if ( !mIsUdf(prevz) )
	{
	    const int nrsteps = (trcnr-prevtrc)/hortrcrg.step;
	    Coord intsect = getIntersection(prevbid,prevz,curbid,curz);
	    if ( intsect.isDefined() )
	    {
		if ( nrsteps==1 )
		{
		    pos1bids += prevbid; pos1zs += prevz;
		    pos2bids += curbid; pos2zs += curz;
		    intersections += intsect;
		    if ( firstonly )
			return true;
		}
		else
		{
		    intsect = getIntersection(prevbid,prevz,curbid,prevz);
		    if ( intsect.isDefined() )
		    {
			const bool havetwointersects =
			    fabs(prevz-curz) >= SI().zStep();

			Coord intsect2 = Coord::udf();
			if ( havetwointersects )
			    intsect2 =getIntersection(prevbid,curz,curbid,curz);

			if ( !havetwointersects || !intsect2.isUdf() )
			{
			    pos1bids += prevbid; pos1zs += prevz;
			    pos2bids += curbid; pos2zs += prevz;
			    intersections += intsect;
			    if ( firstonly ) return true;
			}

			if ( !intsect2.isUdf() )
			{
			    pos2bids += prevbid; pos2zs += curz;
			    pos1bids += curbid; pos1zs += curz;
			    intersections += intsect2;
			    if ( firstonly ) return true;
			}
		    }
		}
	    }
	}
	prevtrc = trcnr;
	prevz = curz;
	prevbid = curbid;
    }

    if ( !intersections.isEmpty() )
	return true;

    if ( !allowextend )
	return false;

    const BinID extbid0( isinl_ ? nr_ : trcrange_.start,
			 isinl_ ? trcrange_.start : nr_ );
    const BinID extbid1( isinl_ ? nr_:trcrange_.stop,
			 isinl_ ? trcrange_.stop:nr_ );
    const Coord intsect0 = mIsUdf(firstz) ? Coord::udf() :
	getIntersection(extbid0, firstz, firstbid, firstz);
    const Coord intsect1 = mIsUdf(prevz) ? Coord::udf() :
	getIntersection(prevbid,prevz,extbid1,prevz);
    const bool defined0 = intsect0.isDefined();
    const bool defined1 = intsect1.isDefined();
    if ( !defined0 && !defined1 )
	return false;

    const bool usethefirst = (defined0 && !defined1) ||
	(defined0 && defined1 && trcrg.stop <= hortrcrg.start);

    pos1bids += usethefirst ? extbid0 : prevbid;
    pos1zs += usethefirst ? firstz : prevz;
    pos2bids += usethefirst ? firstbid : extbid1;
    pos2zs += usethefirst ? firstz : prevz;
    intersections += usethefirst ? intsect0 : intsect1;

    return intersections.size();
}


bool FaultTrace::getHorIntersection( const EM::Horizon& hor, BinID& bid ) const
{
    TypeSet<BinID> pos1bids, pos2bids;
    TypeSet<float> pos1zs, pos2zs;
    TypeSet<Coord> intersects;
    if ( !getHorizonIntersectionInfo( hor, Survey::GM().cUndefGeomID(),
		pos1bids, pos1zs, pos2bids, pos2zs, intersects, true ) )
	return false;

    const StepInterval<int> trcrg = isinl_ ? hor.geometry().colRange()
					   : hor.geometry().rowRange();
    bid.inl() = isinl_ ? nr_ : trcrg.snap( mNINT32(intersects[0].x) );
    bid.crl() = isinl_ ? trcrg.snap( mNINT32(intersects[0].x) ) : nr_;
    return true;
}


bool FaultTrace::getCoordsBetween( int starttrc, float startz, int stoptrc,
				   float stopz, TypeSet<Coord>& coords ) const
{
    if ( trcnrs_.isEmpty() )
    return false;

    const int maxtrc = mMAX(starttrc,stoptrc);
    const int mintrc = mMIN(starttrc,stoptrc);
    const float maxz = mMAX(startz,stopz);
    const float minz = mMIN(startz,stopz);
    const Coord pos0( starttrc, startz * SI().zDomain().userFactor() );
    const Coord pos1( stoptrc, stopz * SI().zDomain().userFactor() );

    int startsegidx = -1, stopsegidx = -1;
    double minsqrdist0 = -1, minsqrdist1 = -1;
    for ( int idx=0; idx<tracesegs_.size(); idx++ )
    {
	const double y0 =
	mMIN(tracesegs_[idx].start_.y,tracesegs_[idx].stop_.y);
	const double y1 =
	mMAX(tracesegs_[idx].start_.y,tracesegs_[idx].stop_.y);

	const double x0 =
	mMIN(tracesegs_[idx].start_.x,tracesegs_[idx].stop_.x);
	const double x1 =
	mMAX(tracesegs_[idx].start_.x,tracesegs_[idx].stop_.x);
	if ( (pos0.y>=y0 && pos0.y<=y1) || (starttrc>=x0 && starttrc<=x1) )
	{
	    const Coord projpos = tracesegs_[idx].closestPoint(pos0);
	    if ( !projpos.isUdf() )
	    {
		const Coord diff = projpos - pos0;
		const double sqrdist = diff.dot( diff );
		if ( minsqrdist0<0 || minsqrdist0>sqrdist )
		{
		    minsqrdist0 = sqrdist;
		    startsegidx = idx;
		}
	    }
	}

	if ( (pos1.y>=y0 && pos1.y<=y1) || (stoptrc>=x0 && stoptrc<=x1) )
	{
	    const Coord projpos = tracesegs_[idx].closestPoint(pos1);
	    if ( !projpos.isUdf() )
	    {
		const Coord diff = projpos - pos1;
		const double sqrdist = diff.dot( diff );
		if ( minsqrdist1<0 || minsqrdist1>sqrdist )
		{
		    minsqrdist1 = sqrdist;
		    stopsegidx = idx;
		}
	    }
	}
    }

    if ( (startsegidx==-1 && stopsegidx==-1) || (startsegidx==stopsegidx) )
	return false;

    if ( startsegidx==-1 || stopsegidx==-1 )
    {
	const int segidx = startsegidx==-1  ? stopsegidx : startsegidx;
	if ( (tracesegs_[segidx].start_.y>=minz &&
	      tracesegs_[segidx].start_.y<=maxz) ||
	     (tracesegs_[segidx].start_.x>=mintrc &&
	      tracesegs_[segidx].start_.x<=maxtrc) )
	    coords += tracesegs_[segidx].start_;
	else
	    coords += tracesegs_[segidx].stop_;
	return true;
    }

    if ( startsegidx < stopsegidx )
    {
	for ( int idx=startsegidx+1; idx<=stopsegidx; idx++ )
	    coords += tracesegs_[idx].start_;
    }
    else
    {
	for ( int idx=startsegidx-1; idx>=stopsegidx; idx-- )
	    coords += tracesegs_[idx].stop_;
    }

    return true;
}


bool FaultTrace::getIntersectionTraces( float zval, TypeSet<int>& trcs ) const
{
    if ( !zrange_.includes(zval,false) )
	return mUdf(int);

    const float z = zval * SI().zDomain().userFactor();
    const float eps = SI().zStep() * SI().zDomain().userFactor() * 0.2f;

    bool res = false;
    for ( int idx=0; idx<tracesegs_.size(); idx++ )
    {
	const double z0 = tracesegs_[idx].start_.y;
	const double z1 = tracesegs_[idx].stop_.y;
	if ( (z<z0 && z<z1) || (z>z0 && z>z1) )
	    continue;

	const double t0 = tracesegs_[idx].start_.x;
	const double t1 = tracesegs_[idx].stop_.x;
	const int curtrc = mIsEqual(z0,z1,eps) ?
	    mNINT32(t0) : mNINT32(t0+(z-z0)/(z1-z0)*(t1-t0));
	trcs += curtrc;
	res = true;
    }

    return res;
}


bool FaultTrace::getIntersectionZs( int trc, TypeSet<float>& zs ) const
{
    if ( trcnrs_.isEmpty() || !trcrange_.includes(trc,false) )
	return mUdf(float);

    bool res = false;
    for ( int idx=0; idx<tracesegs_.size(); idx++ )
    {
	const int trc0 = mNINT32(tracesegs_[idx].start_.x);
	const int trc1 = mNINT32(tracesegs_[idx].stop_.x);
	if ( (trc<trc0 && trc<trc1) || (trc>trc0 && trc>trc1) )
	    continue;

	const float z0 = mCast(float,tracesegs_[idx].start_.y) /
	    SI().showZ2UserFactor();
	const float z1 = mCast(float,tracesegs_[idx].stop_.y) /
	    SI().showZ2UserFactor();
	zs += trc1 ? z0 : z0+(z1-z0)*(trc-trc0)/mCast(float,trc1-trc0);
	res = true;
    }

    return res;
}


bool FaultTrace::getFaultTraceIntersection( const FaultTrace& flttrc,
					    int& trace, float& zval ) const
{
    if ( trcrange_.stop < flttrc.trcrange_.start ||
	 trcrange_.start > flttrc.trcrange_.stop ||
	 zrange_.stop < flttrc.zrange_.start ||
	 zrange_.start > flttrc.zrange_.stop )
	return false;

    for ( int idx=0; idx<flttrc.tracesegs_.size(); idx++ )
    {
	const int trc0 = mNINT32(flttrc.tracesegs_[idx].start_.x);
	const BinID bid0( isinl_ ? nr_ : trc0, isinl_ ? trc0 : nr_ );
	const int trc1 = mNINT32(flttrc.tracesegs_[idx].stop_.x);
	const BinID bid1( isinl_ ? nr_ : trc1, isinl_ ? trc1 : nr_ );

	const float z0 = mCast(float,flttrc.tracesegs_[idx].start_.y) /
	    SI().showZ2UserFactor();
	const float z1 = mCast(float,flttrc.tracesegs_[idx].stop_.y) /
	    SI().showZ2UserFactor();

	const Coord intsect = getIntersection( bid0, z0, bid1, z1 );
	if ( intsect.isDefined() )
	{
	    trace = mCast(int,intsect.x);
	    zval = mCast(float,intsect.y);
	    return true;
	}
    }

    return false;
}


bool FaultTrace::handleUntrimmed( const BinIDValueSet& bvs,
			Interval<float>& zintv, const BinID& negbid,
			const BinID& posbid, bool istop ) const
{
    BinID bid = negbid;
    float prevz = mUdf(float);
    int& trcvar = isinl_ ? bid.crl() : bid.inl();
    BinID origin( isinl_ ? nr_ : trcrange_.stop+10,
		  isinl_ ? trcrange_.stop+10 : nr_ );
    for ( int idx=0; idx<1024; idx++,trcvar-- )
    {
	BinIDValueSet::SPos pos = bvs.find( bid );
	if ( !pos.isValid() )
	    continue;

	BinID dummy;
	float topz, botz;
	bvs.get( pos, dummy, topz, botz );
	const float z = istop ? topz : botz;
	if ( mIsUdf(z) )
	    continue;

	Coord intersectn = getIntersection( origin, z, bid, z );
	if ( !intersectn.isDefined() )
	    return false;

	if ( fabs(intersectn.x - (float)trcvar) > 10 )
	    break;

	prevz = z;
    }

    zintv.start = prevz;
    bid = posbid;
    prevz = mUdf(float);
    origin = BinID( isinl_ ? nr_ : trcrange_.start-10,
		    isinl_ ? trcrange_.start-10 : nr_ );
    for ( int idx=0; idx<1024; idx++,trcvar++ )
    {
	BinIDValueSet::SPos pos = bvs.find( bid );
	if ( !pos.isValid() )
	    continue;

	BinID dummy;
	float topz, botz;
	bvs.get( pos, dummy, topz, botz );
	const float z = istop ? topz : botz;
	if ( mIsUdf(z) )
	    continue;

	Coord intersectn = getIntersection( origin, z, bid, z );
	if ( !intersectn.isDefined() )
	    return false;

	if ( fabs(intersectn.x - (float)trcvar) > 10 )
	    break;

	prevz = z;
    }

    zintv.stop = prevz;
    return true;
}


bool FaultTrace::getHorCrossings( const BinIDValueSet& bvs,
				  Interval<float>& ztop,
				  Interval<float>& zbot ) const
{
    int step = isinl_ ? SI().crlStep() : SI().inlStep();
    BinID start( isinl_ ? nr_ : trcrange_.start - 10*step,
	         isinl_ ? trcrange_.start - 10*step : nr_ );
    float starttopz=mUdf(float), startbotz=mUdf(float);
    int& startvar = isinl_ ? start.crl() : start.inl();
    const od_int64 bvssz = bvs.totalSize();
    od_int64 idx = 0;
    for ( idx=0; idx<bvssz; idx++,startvar += step )
    {
	BinIDValueSet::SPos pos = bvs.find( start );
	if ( !pos.isValid() )
	    continue;

	BinID dummy;
	bvs.get( pos, dummy, starttopz, startbotz );
	if ( !mIsUdf(starttopz) && !mIsUdf(startbotz) )
	    break;
    }

    if ( idx == bvssz )
	return false;

    BinID stop( isinl_ ? nr_ : trcrange_.stop + 10*step,
		isinl_ ? trcrange_.stop + 10*step: nr_ );
    int& stopvar = isinl_ ? stop.crl() : stop.inl();
    step = -step;
    float stoptopz = mUdf(float), stopbotz = mUdf(float);
    float prevstoptopz = mUdf(float), prevstopbotz = mUdf(float);
    BinID stoptopbid, stopbotbid, prevstoptopbid, prevstopbotbid;
    bool foundtop = false, foundbot = false;
    bool tophasisect = false, bothasisect = false;
    for ( idx=0; idx<bvssz; idx++,stopvar += step )
    {
	if ( foundtop && foundbot )
	    break;

	BinIDValueSet::SPos pos = bvs.find( stop );
	if ( !pos.isValid() )
	    continue;

	BinID dummy;
	float z1=mUdf(float), z2=mUdf(float);
	bvs.get( pos, dummy, z1, z2 );
	if ( !foundtop && !mIsUdf(z1) )
	{
	    stoptopbid = stop;
	    stoptopz = z1;
	    Coord intsectn = getIntersection( start, starttopz,
					      stop, stoptopz );
	    if ( !tophasisect )
		tophasisect = intsectn != Coord::udf();

	    if ( tophasisect && intsectn == Coord::udf() )
		foundtop = true;
	    else
	    {
		prevstoptopbid = stop;
		prevstoptopz = stoptopz;
	    }
	}

	if ( !foundbot && !mIsUdf(z2) )
	{
	    stopbotbid = stop;
	    stopbotz = z2;
	    Coord intsectn = getIntersection( start, startbotz,
					      stop, stopbotz );

	    if ( !bothasisect )
		bothasisect = intsectn != Coord::udf();

	    if ( bothasisect && intsectn == Coord::udf() )
		foundbot = true;
	    else
	    {
		prevstopbotbid = stop;
		prevstopbotz = stopbotz;
	    }
	}
    }

    if ( !tophasisect && !bothasisect )
	return false;

    if ( !tophasisect && !mIsUdf(zrange_.start) )
	prevstoptopz = stoptopz = zrange_.start;
    if ( !bothasisect && !mIsUdf(zrange_.stop) )
	prevstopbotz = stopbotz = zrange_.stop;

    ztop.set( stoptopz, prevstoptopz );
    zbot.set( stopbotz, prevstopbotz );
    if ( trcnrs_.size() ) // that is 2D.
    {
	BinID stepbid( isinl_ ? 0 : step, isinl_ ? step : 0 );
	bool isuntrimmed = (stoptopbid - prevstoptopbid) == stepbid;
	if ( tophasisect && isuntrimmed && !handleUntrimmed(bvs,ztop,stoptopbid,
							  prevstoptopbid,true) )
	    return false;
	isuntrimmed = (stopbotbid - prevstopbotbid) == stepbid;
	if ( bothasisect && isuntrimmed && !handleUntrimmed(bvs,zbot,stopbotbid,
							 prevstopbotbid,false) )
	    return false;
    }

    return true;
}


float FaultTrace::getZValFor( const BinID& bid ) const
{
    const StepInterval<float>& zrg = SI().zRange( false );
    Coord intersectn = getIntersection( bid, zrg.start, bid, zrg.stop );
    return (float) intersectn.y;
}


bool FaultTrace::isOnPosSide( const BinID& bid, float z ) const
{
    if ( ( isinl_ && bid.inl() != nr_ ) || ( !isinl_ && bid.crl() != nr_ ) )
	return false;

    const int trcnr = isinl_ ? bid.crl() : bid.inl();
    if ( trcnr >= trcrange_.stop )
	return true;
    if ( trcnr <= trcrange_.start )
	return true;

    const BinID startbid( isinl_ ? nr_ : trcrange_.start,
			  isinl_ ? trcrange_.start : nr_ );
    const BinID stopbid( isinl_ ? nr_ : trcrange_.stop,
			  isinl_ ? trcrange_.stop : nr_ );
    const float zmid = ( zrange_.start + zrange_.stop ) / 2;
    Coord startintersectn = getIntersection( startbid, zmid, bid, z );
    if ( startintersectn.isDefined() )
	return true;

    Coord stopintersectn = getIntersection( stopbid, zmid, bid, z );
    if ( stopintersectn.isDefined() )
	return false;

    return false;
}


void FaultTrace::getAllActNames( BufferStringSet& actnms )
{
    actnms.erase();
    actnms.add( "AllowCross (Default)" );
    actnms.add( "ForbidCross (DataDriven)" );
    actnms.add( "ForbidCrossHigh (ModelDriven)" );
    actnms.add( "ForbidCrossLow (ModelDriven)" );
}


void FaultTrace::getActNames( BufferStringSet& actnms, bool is2d )
{
    actnms.erase();
    actnms.add( "AllowCross" );
    actnms.add( "ForbidCross (DataDriven)" );

    if ( is2d )
    {
	actnms.add( "AllowMinTraceToFault (ModelDriven)" );
	actnms.add( "AllowMaxTraceToFault (ModelDriven)" );
    }
    else
    {
	actnms.add( "AllowMinInlToFault (ModelDriven)" );
	actnms.add( "AllowMaxInlToFault (ModelDriven)" );
	actnms.add( "AllowMinCrlToFault (ModelDriven)" );
	actnms.add( "AllowMaxCrlToFault (ModelDriven)" );
    }
}


bool FaultTrace::includes( const BinID& bid ) const
{
    return isinl_ ? bid.inl() == nr_ && trcrange_.includes( bid.crl(), true )
		  : bid.crl() == nr_ && trcrange_.includes( bid.inl(), true );
}


void FaultTrace::computeTraceSegments()
{
    tracesegs_.erase();
    for ( int idx=1; idx<coordindices_.size(); idx++ )
    {
	const int curidx = coordindices_[idx];
	const int previdx = coordindices_[idx-1];
	if ( curidx < 0 || previdx < 0 )
	    continue;

	const Coord3& pos1 = get( previdx );
	const Coord3& pos2 = get( curidx );
	Coord nodepos1, nodepos2;
	const bool is2d = !trcnrs_.isEmpty();
	if ( is2d )
	{
	    nodepos1.setXY( getTrcNr(previdx),
			    pos1.z * SI().zDomain().userFactor() );
	    nodepos2.setXY( getTrcNr(curidx),
			    pos2.z * SI().zDomain().userFactor() );
	}
	else
	{
	    Coord posbid1 = SI().binID2Coord().transformBackNoSnap( pos1 );
	    Coord posbid2 = SI().binID2Coord().transformBackNoSnap( pos2 );
	    nodepos1.setXY( isinl_ ? posbid1.y : posbid1.x,
			    pos1.z * SI().zDomain().userFactor() );
	    nodepos2.setXY( isinl_ ? posbid2.y : posbid2.x,
			    pos2.z * SI().zDomain().userFactor() );
	}

	tracesegs_ += Line2( nodepos1, nodepos2 );
    }
}


Coord FaultTrace::getIntersection( const BinID& bid1, float z1,
				   const BinID& bid2, float z2  ) const
{
    if ( size() == 0 )
	return Coord::udf();

    const bool is2d = trcnrs_.size();
    if ( is2d && trcnrs_.size() != size() )
	return Coord::udf();

    Interval<float> zrg( z1, z2 );
    zrg.sort();
    z1 *= SI().zDomain().userFactor();
    z2 *= SI().zDomain().userFactor();
    if ( ( isinl_ && (bid1.inl() != nr_ || bid2.inl() != nr_) )
	    || ( !isinl_ && (bid1.crl() != nr_ || bid2.crl() != nr_) ) )
	return Coord::udf();

    Coord pt1( isinl_ ? bid1.crl() : bid1.inl(), z1 );
    Coord pt2( isinl_ ? bid2.crl() : bid2.inl(), z2 );
    Line2 line( pt1, pt2 );

    for ( int idx=0; idx<tracesegs_.size(); idx++ )
    {
	const Line2& fltseg = tracesegs_[idx];
	if ( line.start_ == line.stop_ )
	{
	    if ( fltseg.isOnLine(line.start_) )
		return line.start_;
	    continue;
	}

	Coord interpos = line.intersection( fltseg );
	if ( interpos != Coord::udf() )
	{
	    interpos.y /= SI().zDomain().userFactor();
	    return interpos;
	}
    }

    return Coord::udf();
}


bool FaultTrace::getIntersection( const BinID& bid1, float z1,
				  const BinID& bid2, float z2,
				  BinID& bid, float& z,
				  const StepInterval<int>* intv,
                                  bool snappositive) const
{
    const Coord intersection = getIntersection( bid1, z1, bid2, z2 );
    if ( !intersection.isDefined() )
	return false;

    int trcnr = mNINT32( intersection.x );
    if ( intv )
    {
        const float fidx = intv->getfIndex( intersection.x );
	const int trcidx = mNINT32( snappositive ? Math::Ceil(fidx)
						 : Math::Floor(fidx) );
	trcnr = intv->atIndex( trcidx );
    }

    bid.inl() = isinl_ ? nr_ : trcnr;
    bid.crl() = isinl_ ? trcnr : nr_;
    z = (float) intersection.y;
    return !intv || intv->includes( trcnr, true );
}


bool FaultTrace::isOnFault( const BinID& bid, float z,
			    float thresholddist ) const
{
    const int trcnr = isinl_ ? bid.inl() : bid.crl();
    const int alttrcnr = !isinl_ ? bid.inl() : bid.crl();
    if ( trcnr != nr_ || !trcrange_.includes(alttrcnr,false) ) return false;
    BinID startbid( isinl_ ? nr_ : trcrange_.start,
		    isinl_ ? trcrange_.start : nr_ );

    BinID stopbid( isinl_ ? nr_ : trcrange_.stop,
		   isinl_ ? trcrange_.stop : nr_ );
    Coord intersection = getIntersection( bid, z, startbid, z );
    if ( !intersection.isDefined() )
	intersection = getIntersection( bid, z, stopbid, z );
    if ( !intersection.isDefined() )
	return false;
    Coord pt( isinl_ ? bid.crl() : bid.inl(), z );
    const double dist = pt.distTo( intersection );
    return mIsEqual(dist,0.0,thresholddist);
}


bool FaultTrace::isCrossing( const BinID& bid1, float z1,
			     const BinID& bid2, float z2  ) const
{
    if ( (isinl_ && bid1.inl() != bid2.inl()) ||
	 (!isinl_ && bid1.crl() != bid2.crl()) )
	return false;

    Interval<int> trcrg( isinl_ ? bid1.crl() : bid1.inl(),
			 isinl_ ? bid2.crl() : bid2.inl() );
    trcrg.sort();
    if ( !trcrg.overlaps(trcrange_,false) )
	return false;

    const Coord intersection = getIntersection( bid1, z1, bid2, z2 );
    return intersection.isDefined();
}


void FaultTrace::computeRange()
{
    trcrange_.set( mUdf(int), -mUdf(int) );
    zrange_.set( mUdf(float), -mUdf(float) );
    if ( trcnrs_.size() )
    {
	Interval<float> floattrcrg( mUdf(float), -mUdf(float) );
	for ( int idx=0; idx<trcnrs_.size(); idx++ )
	    floattrcrg.include( trcnrs_[idx], false );

	trcrange_.set( (int) floattrcrg.start, (int) ceil(floattrcrg.stop) );

	for ( int idx=0; idx<coords_.size(); idx++ )
	    zrange_.include( (float) coords_[idx].z, false );
    }
    else
    {
	for ( int idx=0; idx<coords_.size(); idx++ )
	{
	    const BinID bid = SI().transform( coords_[idx] );
	    trcrange_.include( isinl_ ? bid.crl() : bid.inl(), false );
	    zrange_.include( (float) coords_[idx].z, false );
	}
    }

    computeTraceSegments();
}


bool FaultTrace::isOK() const
{
    if ( coords_.isEmpty() ) return false;

    double prevz = coords_[0].z;
    for ( int idx=1; idx<coords_.size(); idx++ )
    {
	if ( coords_[idx].z < prevz )
	    return false;

	prevz = coords_[idx].z;
    }

    return true;
}


//FaultTrcHolder
FaultTrcHolder::FaultTrcHolder()
    : hs_(false)
{
    traces_.allowNull( true );
}


FaultTrcHolder::~FaultTrcHolder()
{ deepUnRef( traces_ ); }


const FaultTrace* FaultTrcHolder::getTrc( int linenr, bool isinl ) const
{
    if ( hs_.nrInl() == 1 || hs_.nrCrl()==1 )
    {
	const bool invalid = traces_.isEmpty() || !traces_[0] ||
	    traces_[0]->isInl()!=isinl || traces_[0]->lineNr()!=linenr;
	return invalid ? 0 : traces_[0];
    }

    const int idx = indexOf( linenr, isinl );
    return traces_.validIdx(idx) ? traces_[idx] : 0;
}


int FaultTrcHolder::indexOf( int linenr, bool isinl ) const
{
    if ( isinl )
	return hs_.lineRange().includes(linenr,false) ? hs_.inlIdx(linenr) : -1;

    const int crlidx = hs_.crlIdx(linenr);
    return crlidx < 0 ? crlidx : crlidx + (hs_.nrCrl()==1 ? 0 : hs_.nrInl());
}


bool FaultTrcHolder::isEditedOnCrl() const
{
    for ( int idx=0; idx<traces_.size(); idx++ )
	if ( traces_[idx] )
	    return traces_[idx]->isEditedOnCrl();

    return false;
}


// FaultTraceExtractor

FaultTraceExtractor::FaultTraceExtractor( const EM::Fault& flt,
					  FaultTrcHolder& holder )
  : ParallelTask("Extracting Fault Traces")
  , fault_(flt)
  , holder_(holder)
  , totalnr_(0)
{ fault_.ref(); }

FaultTraceExtractor::~FaultTraceExtractor()
{ fault_.unRef(); }

od_int64 FaultTraceExtractor::nrIterations() const
{ return totalnr_; }

uiString FaultTraceExtractor::uiMessage() const
{ return tr("Extracting Fault Traces"); }

bool FaultTraceExtractor::doPrepare( int nrthreads )
{
    deepUnRef( holder_.traces_ );
    for ( int idx=0; idx<totalnr_; idx++ )
	holder_.traces_ += 0;

    return true;
}


bool FaultTraceExtractor::doWork( od_int64 start, od_int64 stop,
				  int nrthreads )
{
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	if ( !extractFaultTrace(idx) )
	    return false;

	addToNrDone( 1 );
    }

    return true;
}


FaultTraceExtractor3D::FaultTraceExtractor3D( const EM::Fault& flt,
					      FaultTrcHolder& holder )
  : FaultTraceExtractor(flt,holder)
  , fltsurf_(0)
{
    totalnr_ = holder_.hs_.nrInl() == 1 || holder_.hs_.nrCrl() == 1 ? 1
				: holder_.hs_.nrInl() + holder_.hs_.nrCrl();
}


FaultTraceExtractor3D::~FaultTraceExtractor3D()
{ delete fltsurf_; }

bool FaultTraceExtractor3D::doPrepare( int nrthreads )
{
    mDynamicCastGet(const EM::Fault3D*,cfault,&fault_)
    if ( !cfault )
	return false;

    editedoncrl_ = cfault && cfault->geometry().areEditPlanesMostlyCrossline();
    EM::Fault3D* fault3d = const_cast<EM::Fault3D*>(cfault);
    Geometry::FaultStickSurface* fss = fault3d->geometry().geometryElement();
    if ( !fss )
	return false;

    if ( !fltsurf_ )
    {
	fltsurf_ = new Geometry::ExplFaultStickSurface( fss, SI().zScale() );
	fltsurf_->setCoordList( new FaultTrace, new FaultTrace, 0 );
	fltsurf_->update( true, 0 );
    }

    return FaultTraceExtractor::doPrepare( nrthreads );
}


bool FaultTraceExtractor3D::extractFaultTrace( int idx )
{
    mDynamicCastGet(const EM::Fault3D*,cfault,&fault_)
    if ( !cfault )
	return false;

    const TrcKeySampling& hs = holder_.hs_;
    const bool isinl = hs.nrCrl() > 1 && idx < hs.nrInl();
    const int linenr = isinl
	? hs.start_.inl() + idx * hs.step_.inl()
	: hs.start_.crl() +
		(hs.nrCrl()==1 ? 0 : (idx-hs.nrInl()) * hs.step_.crl() );

    const StepInterval<float>& zrg = SI().zRange( false );
    BinID start( isinl ? linenr : holder_.hs_.start_.inl(),
		 isinl ? holder_.hs_.start_.crl() : linenr );
    BinID stop( isinl ? linenr : holder_.hs_.stop_.inl(),
		isinl ? holder_.hs_.stop_.crl() : linenr );
    Coord3 p0( SI().transform(start), zrg.start );
    Coord3 p1( SI().transform(start), zrg.stop );
    Coord3 p2( SI().transform(stop), zrg.stop );
    Coord3 p3( SI().transform(stop), zrg.start );
    TypeSet<Coord3> pts;
    pts += p0; pts += p1; pts += p2; pts += p3;
    const Coord3 normal = (p1-p0).cross(p3-p0).normalize();

    PtrMan<Geometry::ExplPlaneIntersection> insectn =
					new Geometry::ExplPlaneIntersection;
    insectn->setShape( *fltsurf_ );
    insectn->addPlane( normal, pts );
    Geometry::IndexedShape* idxdshape = insectn;
    RefMan<FaultTrace> clist = new FaultTrace;
    RefMan<FaultTrace> normallist = new FaultTrace;
    idxdshape->setCoordList( clist, normallist, 0 );
    if ( !idxdshape->update(true,0) )
	return false;

    Geometry::IndexedGeometry* idxgeom = idxdshape->getGeometry()[0];

    Geometry::PrimitiveSet* idxps = idxgeom->getCoordsPrimitiveSet();

    if ( !idxps )
	return false;

    TypeSet<int> coordindices;
    for ( int index=1; index<idxps->size(); index+=2 )
    {
	coordindices += idxps->get( index-1 );
	coordindices += idxps->get( index );
	coordindices += -1;
    }

    FaultTrace* flttrc = clist->clone();
    flttrc->ref();
    flttrc->setIndices( coordindices );
    flttrc->setIsInl( isinl );
    flttrc->setEditedOnCrl( editedoncrl_ );
    flttrc->setLineNr( linenr );
    flttrc->computeRange();
    holder_.traces_.replace( idx, flttrc );
    return true;
}


FaultTraceExtractor2D::FaultTraceExtractor2D( const EM::Fault& flt,
					  FaultTrcHolder& holder,
					  Pos::GeomID geomid )
  : FaultTraceExtractor(flt,holder)
  , geomid_(geomid)
{
    mDynamicCastGet(const EM::FaultStickSet*,fss,&fault_)
    totalnr_ = fss ? fss->geometry().nrSticks() : 0;
}


FaultTraceExtractor2D::~FaultTraceExtractor2D()
{}

bool FaultTraceExtractor2D::doPrepare( int nrthreads )
{
    return FaultTraceExtractor::doPrepare( nrthreads );
}


static float getFloatTrcNr( const PosInfo::Line2DData& linegeom,
			    const Coord& crd )
{
    const int index = linegeom.nearestIdx( crd );
    if ( index < 0 ) return mUdf(float);

    const TypeSet<PosInfo::Line2DPos>& posns = linegeom.positions();
    const PosInfo::Line2DPos& pos = posns[index];
    float closestdistfromnode = mUdf(float);
    int index2 = -1;
    if ( index > 0 )
    {
	const PosInfo::Line2DPos& prevpos = posns[index-1];
	const float distfromnode = (float) prevpos.coord_.distTo( crd );
	if ( distfromnode < closestdistfromnode )
	{
	    closestdistfromnode = distfromnode;
	    index2 = index - 1;
	}
    }

    if ( posns.validIdx(index+1) )
    {
	const PosInfo::Line2DPos& nextpos = posns[index+1];
	const float distfromnode = (float) nextpos.coord_.distTo( crd );
	if ( distfromnode < closestdistfromnode )
	{
	    closestdistfromnode = distfromnode;
	    index2 = index + 1;
	}
    }

    if ( index2 < 0 )
	return mUdf( float );

    const Coord linepos1 = pos.coord_;
    const Coord linepos2 = posns[index2].coord_;
    const Line2 line( linepos1, linepos2 );
    Coord posonline = line.closestPoint( crd );
    if ( posonline.distTo(crd) > 100 )
	return mUdf(float);

    const float frac =
	mCast(float,linepos1.distTo(posonline)/linepos1.distTo(linepos2));
    return (float)(pos.nr_ + frac * ( posns[index2].nr_ - pos.nr_ ));
}


bool FaultTraceExtractor2D::doFinish( bool success )
{
    if ( !success )
	return false;

    Interval<int> trcrg( mUdf(int), -mUdf(int) );
    for ( int idx=0; idx<totalnr_; idx++ )
	if ( holder_.traces_[idx] )
	    trcrg.include( holder_.traces_[idx]->trcRange(), false );

    holder_.hs_.setInlRange( Interval<int>(0,0) );
    holder_.hs_.setCrlRange( trcrg );
    return true;
}


bool FaultTraceExtractor2D::extractFaultTrace( int stickidx )
{
    mDynamicCastGet(const EM::FaultStickSet*,fss,&fault_)
    if ( !fss ) return false;

    const Geometry::FaultStickSet* fltgeom =
			fss->geometry().geometryElement();
    if ( !fltgeom ) return false;

    const int sticknr = fltgeom->rowRange().atIndex( stickidx );
    if ( !fss->geometry().pickedOn2DLine(sticknr) )
	return true;

    const Pos::GeomID geomid = fss->geometry().pickedGeomID( sticknr );
    if ( geomid != geomid_ ) return true;

    mDynamicCastGet( const Survey::Geometry2D*, geom2d,
		     Survey::GM().getGeometry(geomid_) );
    if ( !geom2d )
	return true;

    const int nrknots = fltgeom->nrKnots( sticknr );
    if ( nrknots < 2 ) return true;

    FaultTrace* flttrc = new FaultTrace;
    flttrc->ref();
    flttrc->setIsInl( true );
    flttrc->setLineNr( 0 );

    StepInterval<int> colrg = fltgeom->colRange( sticknr );
    TypeSet<int> indices;
    for ( int idx=colrg.start; idx<=colrg.stop; idx+=colrg.step )
    {
	const Coord3 knot = fltgeom->getKnot( RowCol(sticknr,idx) );
	const float trcnr = getFloatTrcNr( geom2d->data(), knot );
	if ( mIsUdf(trcnr) )
	    break;

	indices += flttrc->add( knot, trcnr );
    }

    indices += -1;
    flttrc->setIndices( indices );
    flttrc->computeRange();
    holder_.traces_.replace( stickidx, flttrc );
    return true;
}


//FaultTrcDataProvider
FaultTrcDataProvider::FaultTrcDataProvider()
    : is2d_(false)
{ holders_.allowNull(); }

FaultTrcDataProvider::FaultTrcDataProvider( const Pos::GeomID geomid )
    : geomid_(geomid)
    , is2d_(true)
{ holders_.allowNull(); }

FaultTrcDataProvider::~FaultTrcDataProvider()
{ clear(); }

uiString FaultTrcDataProvider::errMsg() const
{ return errmsg_; }

int FaultTrcDataProvider::nrFaults() const
{ return holders_.size(); }

bool FaultTrcDataProvider::isEmpty() const
{ return holders_.isEmpty(); }

TrcKeySampling FaultTrcDataProvider::range( int idx ) const
{
    return holders_.validIdx(idx) && holders_[idx] ? holders_[idx]->hs_
						   : TrcKeySampling(false);
}


bool FaultTrcDataProvider::isEditedOnCrl( int idx ) const
{
    return holders_.validIdx(idx) && holders_[idx]
	? holders_[idx]->isEditedOnCrl() : false;
}


void FaultTrcDataProvider::clear()
{ deepErase( holders_ ); }

const FaultTrace* FaultTrcDataProvider::getFaultTrace( int idx, int linenr,
						       bool isinl ) const
{
    return holders_.validIdx(idx) && holders_[idx] && !is2d_
	?  holders_[idx]->getTrc( linenr, isinl ) : 0;
}


int FaultTrcDataProvider::nrSticks( int idx ) const
{
    return holders_.validIdx(idx) && holders_[idx]
	? holders_[idx]->traces_.size() : 0;
}


const FaultTrace* FaultTrcDataProvider::getFaultTrace2D( int fltidx,
							 int stickidx ) const
{
    return holders_.validIdx(fltidx) && holders_[fltidx]
		&& holders_[fltidx]->traces_.validIdx(stickidx)
	? holders_[fltidx]->traces_[stickidx] : 0;
}


bool FaultTrcDataProvider::calcFaultBBox( const EM::Fault& flt,
					  TrcKeySampling& hs ) const
{
    mDynamicCastGet(const Geometry::FaultStickSet*,fss,
		    flt.geometry().geometryElement())
    if ( !fss )
	return false;

    const StepInterval<int> rowrg = fss->rowRange();
    for ( int idx=rowrg.start; idx<=rowrg.stop; idx+=rowrg.step )
    {
	const StepInterval<int> colrg = fss->colRange( idx );
	for ( int idy=colrg.start; idy<=colrg.stop; idy+=colrg.step )
	{
	    const Coord3 knot = fss->getKnot( RowCol(idx,idy) );
	    const BinID bid = SI().transform( knot );
	    hs.include( bid );
	}
    }

    return true;
}

# define mErrRet( str ) \
{ errmsg_.setEmpty(); errmsg_ = str; return false; }


bool FaultTrcDataProvider::init( const TypeSet<MultiID>& faultids,
			    const TrcKeySampling& hrg, TaskRunner* taskrunner )
{
    clear();
    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sel( sd );
    sd.rg = hrg;
    ExecutorGroup loadergrp( "Loading Fault" );
    for (int idx = 0; idx < faultids.size(); idx++)
    {
	if ( !EM::EMM().getObjectID( faultids[idx] ).isValid() )
	    loadergrp.add( EM::EMM().objectLoader( faultids[idx], &sel ) );
    }

    if ( !TaskRunner::execute(taskrunner, loadergrp) )
	mErrRet( uiStrings::phrCannotRead(uiStrings::sFault()) )

    if ( is2d_ )
	return get2DTraces( faultids, taskrunner );

    TaskGroup taskgrp;

    for ( int idx=0; idx< faultids.size(); idx++ )
    {
	EM::EMObject* emobj = EM::EMM().getObject(
				    EM::EMM().getObjectID(faultids[idx]));
	mDynamicCastGet(EM::FaultSet3D*,fltset,emobj );
	mDynamicCastGet(EM::Fault*,flt,emobj);
	if ( !fltset && !flt )
	    return false;
	const int nrfaults = fltset ? fltset->nrFaults() : 1;
	for ( int fltidx=0; fltidx<nrfaults; fltidx++ )
	{
	    if ( fltset )
	    {
		const EM::ObjectID oid = fltset->getFaultID( fltidx );
		flt = fltset->getFault3D( oid );
	    }
	    if ( !flt )
	    {
		errmsg_ = uiStrings::phrCannotRead( uiStrings::sFault() );
		holders_ += 0;
		continue;
	    }

	    TrcKeySampling hs( false );
	    calcFaultBBox( *flt, hs );
	    hs.limitTo( hrg );
	    FaultTrcHolder* holder = new FaultTrcHolder();
	    holder->hs_ = hs;
	    holders_ += holder;
	    taskgrp.addTask( new FaultTraceExtractor3D( *flt, *holder ) );
	}
    }

    const bool ret = TaskRunner::execute( taskrunner, taskgrp );
    if ( !ret )
	mErrRet( tr("Failed to extract Fault traces") )

    return true;
}


bool FaultTrcDataProvider::get2DTraces( const TypeSet<MultiID>& faultids,
					TaskRunner* taskrunner )
{
    TaskGroup taskgrp;
    for ( int idx=0; idx<faultids.size(); idx++ )
    {
	const EM::ObjectID oid = EM::EMM().getObjectID( faultids[idx] );
	mDynamicCastGet(EM::Fault*,flt,EM::EMM().getObject(oid))
	if ( !flt )
	{
	    holders_ += 0;
	    continue;
	}

	FaultTrcHolder* holder = new FaultTrcHolder();
	holders_ += holder;
	taskgrp.addTask( new FaultTraceExtractor2D(*flt,*holder,geomid_) );
    }

    return TaskRunner::execute( taskrunner, taskgrp );
}


bool FaultTrcDataProvider::hasFaults( const BinID& bid ) const
{
    for ( int idx=0; idx<holders_.size(); idx++ )
    {
	if ( !holders_[idx] )
	    continue;

	if ( !is2d_ )
	{
	    const FaultTrace* inltrc = holders_[idx]->getTrc( bid.inl(), true );
	    const FaultTrace* crltrc = holders_[idx]->getTrc( bid.crl(), false);
	    if ( (inltrc && inltrc->includes(bid))
	      || (crltrc && crltrc->includes(bid)) )
		return true;
	}
	else
	{
	    for ( int sidx=0; sidx<holders_[idx]->traces_.size(); sidx++ )
	    {
		const FaultTrace* flttrc = holders_[idx]->traces_[sidx];
		if ( flttrc && flttrc->includes(bid) )
		    return true;
	    }
	}
    }

    return false;
}


bool FaultTrcDataProvider::isOnFault( const BinID& bid, float z,
				      float thresholddist ) const
{
    if ( is2d_ )
    {
	for ( int idx=0; idx<nrFaults(); idx++ )
	{
	    const int nrsticks = nrSticks( idx );
	    for ( int idy=0; idy<nrsticks; idy++ )
	    {
		const FaultTrace* flt = getFaultTrace2D( idx, idy );
		if ( flt && flt->isOnFault(bid,z,thresholddist) )
		    return true;
	    }
	}

	return false;
    }

    for ( int idx=0; idx<nrFaults(); idx++ )
    {
	const FaultTrace* flt = getFaultTrace( idx, bid.inl(), true );
	if ( !flt )
	    flt = getFaultTrace( idx, bid.crl(), false );

	if ( flt && flt->isOnFault(bid,z,thresholddist) )
	    return true;
    }

    return false;
}


bool FaultTrcDataProvider::isCrossingFault( const BinID& bid1, float z1,
					    const BinID& bid2, float z2 ) const
{
    if ( is2d_ )
    {
	for ( int idx=0; idx<nrFaults(); idx++ )
	{
	    const int nrsticks = nrSticks( idx );
	    for ( int idy=0; idy<nrsticks; idy++ )
	    {
		const FaultTrace* flt = getFaultTrace2D( idx, idy );
		if ( flt && flt->isCrossing(bid1,z1,bid2,z2) )
		    return true;
	    }
	}

	return false;
    }

    const int inldiff = abs( bid1.inl() - bid2.inl() );
    const int crldiff = abs( bid1.crl() - bid2.crl() );
    if ( !inldiff && !crldiff )
    {
	for ( int idx=0; idx<nrFaults(); idx++ )
	{
	    const FaultTrace* flt = getFaultTrace( idx, bid1.inl(), true );
	    if ( !flt )
		flt = getFaultTrace( idx, bid1.crl(), false );

	    if ( flt && flt->isCrossing(bid1,z1,bid2,z2) )
		return true;
	}

	return false;
    }

    const BinID anchorbid( bid2.inl(), bid1.crl() );
    const float anchorz = z1 + ( z2 - z1 ) * inldiff / ( inldiff + crldiff );
    for ( int idx=0; idx<nrFaults(); idx++ )
    {
	int nrcrossings = 0;
	if ( bid1.inl() != bid2.inl() )
	{
	    const FaultTrace* crlflt = getFaultTrace( idx, bid1.crl(), false );
	    if ( crlflt && crlflt->isCrossing(bid1,z1,anchorbid,anchorz) )
		nrcrossings++;
	}

	if ( bid1.crl() != bid2.crl() )
	{
	    const FaultTrace* inlflt = getFaultTrace( idx, bid2.inl(), true );
	    if ( inlflt && inlflt->isCrossing(bid2,z2,anchorbid,anchorz) )
		nrcrossings++;
	}

	if ( nrcrossings == 1 )
	    return true;
    }

    return false;
}


bool FaultTrcDataProvider::getFaultZVals( const BinID& bid,
					  TypeSet<float>& zvals ) const
{
    zvals.erase();
    for ( int idx=0; idx<nrFaults(); idx++ )
    {
	const int nrsticks = is2d_ ? nrSticks( idx ) : 1;
	for ( int idy=0; idy<nrsticks; idy++ )
	{
	    const FaultTrace* flt = is2d_ ? getFaultTrace2D( idx, idy ) :
		getFaultTrace( idx,bid.inl(),true);
	    if ( !is2d_ && !flt )
		flt = getFaultTrace( idx, bid.crl(), false );

	    if ( !flt ) continue;

	    const float z = flt->getZValFor( bid );
	    if ( !mIsUdf(z) )
		zvals += z;
	}
    }

    return !zvals.isEmpty();
}
