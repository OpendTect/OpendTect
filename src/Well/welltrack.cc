/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/


#include "welltrack.h"

#include "idxable.h"
#include "survinfo.h"
#include "velocitycalc.h"
#include "welldata.h"
#include "wellinfo.h"
#include "welld2tmodel.h"

mDefineInstanceCreatedNotifierAccess(Well::Track)


Well::Track::Track( const char* nm )
    : DahObj(nm)
    , zistime_(false)
{
    mTriggerInstanceCreatedNotifier();
}


Well::Track::Track( const Track& oth )
{
    copyAll( oth );
    mTriggerInstanceCreatedNotifier();
}


Well::Track::~Track()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Well::Track, Well::DahObj )


void Well::Track::copyClassData( const Track& oth )
{
    pos_ = oth.pos_;
    zistime_ = oth.zistime_;
}


Coord3 Well::Track::pos( PointID id ) const
{
    mLock4Read();
    const IdxType idx = gtIdx( id );
    return idx < 0 ? Coord3::udf() : pos_[idx];
}


Coord3 Well::Track::posByIdx( IdxType idx ) const
{
    mLock4Read();
    return pos_.validIdx(idx) ? pos_[idx] : Coord3::udf();
}


Coord3 Well::Track::firstPos() const
{
    mLock4Read();
    return pos_.isEmpty() ? Coord3::udf() : pos_.first();
}


Coord3 Well::Track::lastPos() const
{
    mLock4Read();
    return pos_.isEmpty() ? Coord3::udf() : pos_.last();
}


Coord3 Well::Track::getPos( ZType dh ) const
{
    mLock4Read();
    if ( pos_.isEmpty() )
	return mUdf(Coord3);

    int idx1;
    if ( IdxAble::findFPPos(dahs_,dahs_.size(),dh,-1,idx1) )
	return pos_[idx1];
    else if ( idx1 < 0 || idx1 == dahs_.size()-1 )
    {
        Coord3 ret( idx1 < 0 ? pos_[0] : pos_[idx1] );
        if ( idx1 < 0 )
	    ret.z_ -= dahs_[0] - dh;
        else
	    ret.z_ += dh - dahs_[idx1];

        return ret;
    }

    return coordAfterIdx( dh, idx1 );
}


Well::Track::ZType Well::Track::getKbElev() const
{
    mLock4Read();
    return gtIsEmpty() ? 0.f : (ZType)(dahs_[0] - pos_[0].z_);
}


Well::Track::ZType Well::Track::td() const
{
    mLock4Read();
    return gtIsEmpty() ? 0.f : dahs_.last();
}


Interval<double> Well::Track::zRangeD() const
{
    mLock4Read();
    return gtZRangeD();
}


Well::Track::ZIntvType Well::Track::zRange() const
{
    const Interval<double> zrange = zRangeD();
    return ZIntvType( (ZType)zrange.start, (ZType)zrange.stop );
}


bool Well::Track::alwaysDownward() const
{
    mLock4Read();
    const int sz = pos_.size();
    if ( sz < 2 )
	return true;

    double prevz = pos_[0].z_;
    for ( int idx=1; idx<sz; idx++ )
    {
	double curz = pos_[idx].z_;
	if ( curz <= prevz )
	    return false;

	prevz = curz;
    }

    return true;
}


Well::Track::ZType Well::Track::getDahForTVD( ZType z, ZType prevdah ) const
{
    return getDahForTVD( (double)z, prevdah );
}


Well::Track::ZType Well::Track::getDahForTVD( double z, ZType prevdah ) const
{
    mLock4Read();
    const bool haveprevdah = !mIsUdf(prevdah);
    const int sz = dahs_.size();
    if ( sz < 1 )
	return mUdf(ZType);

    if ( zistime_ )
    {
	pErrMsg("getDahForTVD called for time well");
	const ZType res = haveprevdah ? prevdah : dahs_[0];
	return res;
    }

    static const double eps = 1e-3; // do not use lower for float precision
    static const double epsf = 1e-3f; // do not use lower for float precision
    if ( sz == 1 )
	return mIsEqual(z,pos_[0].z_,eps) ? dahs_[0] : mUdf(ZType);

    const Interval<double> zrange = gtZRangeD();
    if ( !zrange.includes(z,false) )
	return mUdf(ZType);

#define mZInRg() \
    (zrg.start-eps < z  && zrg.stop+eps  > z) \
 || (zrg.stop-eps  < z  && zrg.start+eps > z)

    Interval<double> zrg( zrange.start, 0 );
    int idxafter = -1;
    for ( int idx=1; idx<sz; idx++ )
    {
	if ( !haveprevdah || prevdah+epsf < dahs_[idx] )
	{
	    zrg.stop = pos_[idx].z_;
	    if ( mZInRg() )
		{ idxafter = idx; break; }
	}
	zrg.start = zrg.stop;
    }
    if ( idxafter < 1 )
	return mUdf(ZType);

    const int idx1 = idxafter - 1;
    const int idx2 = idxafter;
    const double z1 = pos_[idx1].z_;
    const double z2 = pos_[idx2].z_;
    const double dah1 = (double)dahs_[idx1];
    const double dah2 = (double)dahs_[idx2];
    const double zdiff = z2 - z1;
    const double res = ( (z-z1) * dah2 + (z2-z) * dah1 ) / zdiff;
    return mIsZero(zdiff,eps) ? dahs_[idx2] : (ZType)res;
}


Well::Track::ZType Well::Track::nearestDah( const Coord3& posin ) const
{
    mLock4Read();
    const int sz = gtSize();
    if ( sz < 1 )
	return 0;
    else if ( sz < 2 )
	return dahs_[0];

    const double zfac = zistime_ ? 2000. : 1.;
    Coord3 reqpos( posin ); reqpos.z_ *= zfac;
    Coord3 curpos( getPos( dahs_[0] ) ); curpos.z_ *= zfac;
    double sqneardist = curpos.sqDistTo( reqpos );
    double sqsecdist = sqneardist;
    int nearidx = 0; int secondidx = 0;
    curpos = getPos( dahs_[1] ); curpos.z_ *= zfac;
    double sqdist = curpos.sqDistTo( reqpos );
    if ( sqdist < sqneardist )
	{ nearidx = 1; sqneardist = sqdist; }
    else
	{ secondidx = 1; sqsecdist = sqdist; }

    for ( int idah=2; idah<sz; idah++ )
    {
	curpos = getPos( dahs_[idah] ); curpos.z_ *= zfac;
	sqdist = curpos.sqDistTo( reqpos );
	if ( sqdist < 0.1 ) return dahs_[idah];

	if ( sqdist < sqneardist )
	{
	    secondidx = nearidx; sqsecdist = sqneardist;
	    nearidx = idah; sqneardist = sqdist;
	}
	else if ( sqdist < sqsecdist )
	    { secondidx = idah; sqsecdist = sqdist; }

	if ( sqdist > 2 * sqneardist ) // safe for 'reasonable wells'
	    break;
    }

    const double neardist = Math::Sqrt( sqneardist );
    const double secdist = Math::Sqrt( sqsecdist );
    const double dahnear = (double)dahs_[nearidx];
    const double dahsec = (double)dahs_[secondidx];
    const double res = ( neardist*dahsec+secdist*dahnear )/( neardist+secdist );
    return (ZType)res;
}

#define mAddPtWithNotif(dh,c) addPt( dh, c, &mAccessLockHandler() )
#define mDahAfterIdx(idx,c2add)\
dahs_[idx]+c2add.distTo<ZType>(pos_[idx])


Well::Track::PointID Well::Track::addPoint( Coord3 c2add, ZType dh )
{
    mLock4Write();
    if ( mIsUdf(dh) )
    {
	const int previdx = dahs_.size() - 1;
	if ( previdx < 0 )
	    dh = 0.f;
	else
	{
	    float dist = c2add.distTo<float>( pos_[previdx] );
	    if ( mIsZero(dist,1e-3) )
		return ptids_[previdx];
	    dh = mDahAfterIdx( previdx, c2add );
	}
    }
    return mAddPtWithNotif( dh, c2add );
}


Well::Track::PointID Well::Track::addPoint( Coord c, ZType z, ZType dh )
{
    return addPoint( Coord3(c,z), dh );
}


Well::Track::PointID Well::Track::insertPoint( Coord c, ZType z )
{
    return insertPoint( Coord3(c,z) );
}


Well::Track::PointID Well::Track::insertPoint( Coord3 c2add )
{
    mLock4Write();
    const int cursz = pos_.size();
    if ( cursz < 1 )
	{ return mAddPtWithNotif( 0.f, c2add ); }

    if ( cursz < 2 )
    {
	Coord3 othpt( pos_[0] );
	if ( othpt.z_ < c2add.z_ )
	    { return mAddPtWithNotif( (ZType)(c2add.z_-othpt.z_), c2add ); }
	else
	{
	    doSetEmpty();
	    addPt( 0.f, c2add, 0 );
	    return mAddPtWithNotif( othpt.distTo<ZType>(c2add), c2add );
	}
    }

    // Need to find 'best' position. This is when the angle of the triangle
    // at the new point is as flat as possible (i.e. has maximum value).
    // This boils down to finding pt with minimal value of:
    // (sum of sq distances / product of distances)

    int idx2insafter = -1;
    int mindistidx = 0;
    float mindist = pos_[mindistidx].distTo<float>( c2add );
    double lowestval = 1e30;
    for ( int idx=1; idx<cursz; idx++ )
    {
	const Coord3& prevpt = pos_[idx-1];
	const Coord3& nextpt = pos_[idx];
	const float dpts = prevpt.distTo<float>( nextpt );
	const float d2prev = prevpt.distTo<float>( c2add );
	const float d2next = nextpt.distTo<float>( c2add );
	if ( mIsZero(d2prev,1e-4) || mIsZero(d2next,1e-4) )
	    return ptids_[idx]; // point already present
	const double val2minimize =
	    (( d2prev*d2prev + d2next*d2next - dpts*dpts ) / (2*d2prev*d2next));
	if ( val2minimize < lowestval )
	    { idx2insafter = idx-1; lowestval = val2minimize; }
	if ( d2next < mindist )
	    { mindist = d2next; mindistidx = idx; }
	if ( idx == cursz-1 && lowestval > 0 )
	{
	    if ( mindistidx == cursz-1 )
		return mAddPtWithNotif( dahs_[idx]+(ZType)d2next, c2add );
	    else if ( mindistidx > 0 && mindistidx < cursz-1 )
	    {
		float prevdist = pos_[mindistidx-1].distTo<float>( c2add );
		float nextdist = pos_[mindistidx+1].distTo<float>( c2add );
		idx2insafter = prevdist > nextdist ? mindistidx : mindistidx -1;
	    }
	    else
		idx2insafter = mindistidx;
	}
    }

    if ( idx2insafter == 0 )
    {
	// The point may be before the first
	const Coord3& firstpt = pos_[0];
	const Coord3& secondpt = pos_[1];
	const double d01sq = firstpt.sqDistTo( secondpt );
	const double d0nsq = firstpt.sqDistTo( c2add );
	const double d1nsq = secondpt.sqDistTo( c2add );
	if ( d01sq + d0nsq < d1nsq )
	    idx2insafter = -1;
    }
    if ( idx2insafter == cursz-2 )
    {
	// Hmmm. The point may be beyond the last
	const Coord3& beforelastpt = pos_[cursz-2];
	const Coord3& lastpt = pos_[cursz-1];
	const double d01sq = beforelastpt.sqDistTo( lastpt );
	const double d0nsq = beforelastpt.sqDistTo( c2add );
	const double d1nsq = lastpt.sqDistTo( c2add );
	if ( d01sq + d1nsq < d0nsq )
	    idx2insafter = cursz-1;
    }

    return insAfterIdx( idx2insafter, c2add, mAccessLockHandler() );
}


bool Well::Track::doSet( IdxType idx, ValueType newz )
{
    Coord3 newpos( pos_[idx] );
    if ( mIsEqual(newpos.z_,newz,dahEps()) )
	return false;

    newpos.z_ = newz;
    doSetPoint( idx, newpos );
    return true;
}


Well::Track::PointID Well::Track::doInsAtDah( ZType dh, ZType zpos )
{
    PointID id = PointID::getInvalid();
    const int sz = gtSize();
    if ( sz < 1 )
	return id;

    const float eps = dahEps();
    if ( dh < dahs_[0] - eps )
    {
	Coord3 crd( pos_[0] ); crd.z_ = zpos;
	id = insPt( 0, dh, crd, 0 );
    }
    else if ( dh > dahs_[size()-1] + eps )
    {
	Coord3 crd( pos_[size()-1] ); crd.z_ = zpos;
	id = addPt( dh, crd, 0 );
    }
    else
    {
	const int insafteridx = indexOf( dh );
	if ( insafteridx < 0 )
	    { pErrMsg("Huh"); return id; }

	if ( mIsEqual(dahs_[insafteridx],dh,eps) )
	{
	    pos_[insafteridx].z_ = zpos;
	    id = ptids_[insafteridx];
	}
	else
	{
	    Coord3 prevcrd( pos_[insafteridx] );
	    Coord3 nextcrd( pos_[insafteridx+1] );
	    Coord3 crd( ( prevcrd + nextcrd )/2 );
	    crd.z_ = zpos;
	    id = insPt( insafteridx+1, dh, crd, 0 );
	}
    }

    return id;
}


Well::Track::ValueType Well::Track::gtVal( IdxType idx ) const
{
    return pos_.validIdx(idx) ? (float)pos_[idx].z_ : mUdf(ValueType);
}


void Well::Track::setPoint( PointID id, const Coord& c, ZType z )
{
    setPoint( id, Coord3(c,z) );
}


void Well::Track::setPoint( PointID id, const Coord3& newpos )
{
    mLock4Write();
    const int idx = gtIdx( id );
    if ( pos_.validIdx(idx) )
	doSetPoint( idx, newpos );
    if ( idx == pos_.size()-1 )
	mSendChgNotif( cDahChange(), ptids_[idx].getI() );
    else
	mSendEntireObjChgNotif();
}


void Well::Track::doSetPoint( IdxType idx, const Coord3& newpos )
{
    const Coord3 oldpos( pos_[idx] );
    float olddist0 = idx > 0 ? oldpos.distTo<float>(pos_[idx-1]) : 0;
    float newdist0 = idx > 0 ? newpos.distTo<float>(pos_[idx-1]) : 0;
    dahs_[idx] += (ZType)(newdist0 - olddist0);
    pos_[idx] = newpos;
    if ( idx < pos_.size()-1 )
    {
	const float olddist1 = oldpos.distTo<float>( pos_[idx+1] );
	const float newdist1 = newpos.distTo<float>( pos_[idx+1] );
	const ZType dahshift = (ZType)(newdist0-olddist0 + newdist1-olddist1);
	shiftDahFrom( ptids_[idx+1], dahshift );
    }
}


void Well::Track::getData( ZSetType& dahs, ValueSetType& vals ) const
{
    mLock4Read();
    const int sz = gtSize();
    for ( int idx=0; idx<sz; idx++ )
    {
	dahs += dahs_[idx];
	vals += (ZType)pos_[idx].z_;
    }
}


#define mDistTol 0.5f

void Well::Track::toTime( const Data& wd )
{
    const Track& track = wd.track();
    const D2TModel& d2t = wd.d2TModel();
    MonitorLock mltrack( track );
    MonitorLock mld2t( d2t );
    if ( track.isEmpty() )
	return;

    TimeDepthModel replvelmodel;
    const double srddepth = -1. * SI().seismicReferenceDatum();
    const ZType dummythickness = 1000.f;
    ZSetType replveldepths, replveltimes;
    replveldepths += (ZType)(srddepth - dummythickness);
    replveltimes += -2.f * dummythickness / wd.info().replacementVelocity();
    replveldepths += (ZType)srddepth;
    replveltimes += 0.f;
    replvelmodel.setModel( replveldepths.arr(), replveltimes.arr(),
			   replveldepths.size() );

    TimeDepthModel dtmodel;
    if ( !d2t.getTimeDepthModel(wd,dtmodel) )
	return;

    ZSetType newdah;
    TypeSet<Coord3> newpos;
    newdah += track.firstDah();
    newpos += track.firstPos();

    ZType prevdah = newdah[0];
    for ( int trckidx=1; trckidx<track.size(); trckidx++ )
    {
	const ZType curdah = track.dahByIdx( trckidx );
	const ZType dist = curdah - prevdah;
	if ( dist > mDistTol )
	{
	    const int nrchunks = (int)( dist / mDistTol );
	    const ZType step = dist / ((ZType)nrchunks);
	    StepInterval<ZType> dahrange( prevdah, curdah, step );
	    for ( int idx=1; idx<dahrange.nrSteps(); idx++ )
	    {
		const ZType dahsegment = dahrange.atIndex( idx );
		newdah += dahsegment;
		newpos += track.getPos( dahsegment );
	    }
	}

	newdah += curdah;
	newpos += track.posByIdx( trckidx );
	prevdah = curdah;
    }

    mLock4Write();
    // Copy the extended set into the new track definition
    dahs_ = newdah;
    pos_ = newpos;
    ptids_ = track.ptids_;

    // Now, convert to time
    for ( int idx=0; idx<dahs_.size(); idx++ )
    {
	double& depth = pos_[idx].z_;
	const bool abovesrd = depth < srddepth;
	if ( !abovesrd && d2t.isEmpty() )
	    { depth = mUdf(double); continue; } //Should never happen

	depth = (double)( abovesrd ? replvelmodel.getTime( (ZType)depth )
					: dtmodel.getTime( (ZType)depth ) );
    }

    zistime_ = true;
    mSendEntireObjChgNotif();
}


Well::Track::PointID Well::Track::addPt( ZType dh, const Coord3& c,
					    AccessLockHandler* alh )
{
    return insPt( ptids_.size(), dh, c, alh );
}


Well::Track::PointID Well::Track::insPt( IdxType idx, ZType dh, const Coord3& c,
					    AccessLockHandler* alh )
{
    const PointID ptid = gtNewPointID();
    if ( idx >= gtSize() )
	{ pos_ += c; dahs_ += dh; ptids_ += ptid; }
    else
    {
	pos_.insert( idx, c );
	dahs_.insert( idx, dh );
	ptids_.insert( idx, ptid );
    }
    if ( alh )
    {
	AccessLockHandler& mAccessLockHandler() = *alh;
	mSendChgNotif( cPointAdd(), ptid.getI() );
    }
    return ptid;
}


Coord3 Well::Track::coordAfterIdx( ZType dh, int idx1 ) const
{
    const int idx2 = idx1 + 1;
    const double d1 = (double)( dh - dahs_[idx1] );
    const double d2 = (double)( dahs_[idx2] - dh );
    const Coord3& prevpt = pos_[idx1];
    const Coord3& nextpt = pos_[idx2];
    const double f =  1. / (d1 + d2);
    return Coord3( f * ( d1 * nextpt.x_ + d2 * prevpt.x_ ),
		   f * ( d1 * nextpt.y_ + d2 * prevpt.y_ ),
		   f * ( d1 * nextpt.z_ + d2 * prevpt.z_ ) );
}


Well::Track::PointID Well::Track::insAfterIdx( int idx2insafter,
		const Coord3& c2add, AccessLockHandler& mAccessLockHandler() )
{
    const int newidx = idx2insafter + 1;
    if ( newidx >= pos_.size() )
	{ return mAddPtWithNotif( mDahAfterIdx(idx2insafter,c2add), c2add ); }

    ZType extradah, newdah = 0;
    if ( idx2insafter == -1 )
	extradah = c2add.distTo<ZType>( pos_[0] );
    else
    {
	const ZType dist0 = c2add.distTo<ZType>( pos_[idx2insafter] );
	const ZType dist1 = c2add.distTo<ZType>( pos_[idx2insafter+1] );
	newdah = dahs_[idx2insafter] + dist0;
	extradah = dist0 + dist1
		 - pos_[idx2insafter].distTo<float>( pos_[idx2insafter+1] );
    }

    const PointID ptid = insPt( newidx, (ZType)newdah, c2add,
				&mAccessLockHandler() );
    shiftDahFrom( ptid, (ZType)extradah );
    return ptid;
}


Interval<double> Well::Track::gtZRangeD() const
{
    Interval<double> ret( 0., 0. );
    const int sz = gtSize();

    if ( sz > 0 )
    {
	ret.start = ret.stop = pos_[0].z_;
	for ( int idx=1; idx<sz; idx++ )
	    ret.include( pos_[idx].z_, false );
    }

    return ret;
}


Well::TrackIter::TrackIter( const Track& trck, bool atend )
    : DahObjIter(trck,atend)
{
}


Well::TrackIter::TrackIter( const TrackIter& oth )
    : DahObjIter(oth)
{
}


const Well::Track& Well::TrackIter::track() const
{
    return static_cast<const Track&>( monitored() );
}


Coord3 Well::TrackIter::pos() const
{
    return isValid() ? track().pos_[ curidx_ ] : Coord3::udf();
}
