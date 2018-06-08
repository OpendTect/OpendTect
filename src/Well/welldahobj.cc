/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2003
-*/


#include "welldahobj.h"
#include "bendpointfinder.h"
#include "idxable.h"


mDefineInstanceCreatedNotifierAccess(Well::DahObj);


Well::DahObj::DahObj( const char* nm )
    : NamedMonitorable(nm)
    , curptidnr_(0)
{
    mTriggerInstanceCreatedNotifier();
}


Well::DahObj::DahObj( const DahObj& oth )
    : NamedMonitorable(oth)
    , curptidnr_(oth.curptidnr_)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


Well::DahObj::~DahObj()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Well::DahObj, NamedMonitorable )


void Well::DahObj::copyClassData( const DahObj& oth )
{
    dahs_ = oth.dahs_;
    ptids_ = oth.ptids_;
    curptidnr_ = oth.curptidnr_;
}


Monitorable::ChangeType Well::DahObj::compareClassData(
					const DahObj& oth ) const
{
    mDeliverYesNoMonitorableCompare( dahs_ == oth.dahs_ );
}


Well::DahObj::size_type Well::DahObj::size() const
{
    mLock4Read();
    return dahs_.size();
}


Well::DahObj::PointID Well::DahObj::firstID() const
{
    mLock4Read();
    return ptids_.isEmpty() ? PointID::getInvalid() : ptids_.first();
}


Well::DahObj::PointID Well::DahObj::lastID() const
{
    mLock4Read();
    return ptids_.isEmpty() ? PointID::getInvalid() : ptids_.last();
}


Well::DahObj::PointID Well::DahObj::nextID( PointID id ) const
{
    mLock4Read();
    return gtNeighbourID( id, true );
}


Well::DahObj::PointID Well::DahObj::prevID( PointID id ) const
{
    mLock4Read();
    return gtNeighbourID( id, false );
}

Well::DahObj::PointID Well::DahObj::nearestID( float dh ) const
{
    mLock4Read();
    idx_type idx = gtIndexOf( dh );
    if ( idx <= 0 )
	return firstID();
    else if ( idx >= size()-1 )
	return lastID();

    const float diffprevval = dh - gtVal( idx );
    const float diffnextval = gtVal( idx+1 ) - dh;

    return diffprevval > diffnextval ? pointIDFor( idx+1 ) : pointIDFor( idx );
}


Well::DahObj::PointID Well::DahObj::gtNeighbourID( PointID id, bool next ) const
{
    if ( ptids_.size() < 2 )
	return PointID::getInvalid();
    const int idx = gtIdx( id );
    if ( idx < 0 )
	return PointID::getInvalid();
    if ( next )
	return idx < ptids_.size()-1 ? ptids_[idx+1] : PointID::getInvalid();
    else
	return idx > 0 ? ptids_[idx-1] : PointID::getInvalid();
}


Well::DahObj::ZType Well::DahObj::dah( PointID id ) const
{
    mLock4Read();
    return gtDah( gtIdx(id) );
}


Well::DahObj::ZType Well::DahObj::firstDah() const
{
    mLock4Read();
    return dahs_.isEmpty() ? mUdf(ZType) : dahs_.first();
}


Well::DahObj::ZType Well::DahObj::lastDah() const
{
    mLock4Read();
    return dahs_.isEmpty() ? mUdf(ZType) : dahs_.last();
}


Well::DahObj::ZType Well::DahObj::dahByIdx( idx_type idx ) const
{
    mLock4Read();
    return gtDah( idx );
}


Well::DahObj::ValueType Well::DahObj::value( PointID id ) const
{
    mLock4Read();
    return gtVal( gtIdx(id) );
}


Well::DahObj::ValueType Well::DahObj::valueByIdx( idx_type idx ) const
{
    mLock4Read();
    return dahs_.validIdx(idx) ? gtVal( idx ) : mUdf(ValueType);
}


Well::DahObj::ValueType Well::DahObj::firstValue( bool noudfs ) const
{
    mLock4Read();
    const size_type sz = dahs_.size();
    for ( idx_type idx=0; idx<sz; idx++ )
    {
	const float val = gtVal( idx );
	if ( !noudfs || !mIsUdf(val) )
	    return val;
    }
    return mUdf(ZType);
}


Well::DahObj::ValueType Well::DahObj::lastValue( bool noudfs ) const
{
    mLock4Read();
    const size_type sz = dahs_.size();
    for ( idx_type idx=sz-1; idx>=0; idx-- )
    {
	const float val = gtVal( idx );
	if ( !noudfs || !mIsUdf(val) )
	    return val;
    }
    return mUdf(ZType);
}


Well::DahObj::ValueType Well::DahObj::valueAt( ZType dh, bool noudfs ) const
{
    mLock4Read();
    return gtValueAt( dh, noudfs );
}


Interval<float>	Well::DahObj::dahRange() const
{
    mLock4Read();
    Interval<float> ret( 0.f, 0.f );
    if ( !gtIsEmpty() )
	ret = Interval<float>( dahs_.first(), dahs_.last() );
    return ret;
}


float Well::DahObj::dahStep( bool retmin ) const
{
    mLock4Read();
    const size_type sz = dahs_.size();
    if ( sz < 2 )
	return mUdf(float);

    float res = dahs_[1] - dahs_[0];
    if ( res <0 ) res = 0;
    size_type nrvals = 1;
    for ( idx_type idx=2; idx<sz; idx++ )
    {
	float val = dahs_[idx] - dahs_[idx-1];
	if ( mIsZero(val,mDefEps) )
	    continue;

	if ( !retmin )
	    res += val;
	else
	{
	    if ( val < res && val >= 0 )
		res = val;
	}
	nrvals++;
    }
    mUnlockAllAccess();

    if ( !retmin )
	res /= nrvals;	// not min? then average
    return mIsZero(res,mDefEps) ? mUdf(float) : res;
}


void Well::DahObj::getDahValues( ZSetType& dahs ) const
{
    mLock4Read();
    dahs = dahs_;
}


void Well::DahObj::setEmpty()
{
    mLock4Read();
    if ( gtIsEmpty() )
	return;
    if ( !mLock2Write() && gtIsEmpty() )
	return;

    doSetEmpty();
    mSendEntireObjChgNotif();
}


bool Well::DahObj::areEqualDahs( ZType dh1, ZType dh2 )
{
    return isFPEqual( dh1, dh2, dahEps() );
}


void Well::DahObj::setDah( PointID id, ZType dh )
{
    mLock4Read();
    idx_type idx = gtIdx( id );
    if ( !dahs_.validIdx(idx) || areEqualDahs(dh,dahs_[idx]) )
	return;

    if ( !mLock2Write() )
    {
	idx = gtIdx( id );
	if ( !dahs_.validIdx(idx) )
	    return;
    }

    dahs_[idx] = dh;
    mSendChgNotif( cDahChange(), id.getI() );
}


bool Well::DahObj::setValue( PointID id, ValueType v )
{
    mLock4Write();
    idx_type idx = gtIdx( id );
    if ( idx < 0 )
	return false;

    if ( !mLock2Write() )
    {
	idx = gtIdx( id );
	if ( idx < 0 )
	    return false;
    }

    const bool rv = doSet( idx, v );
    if ( rv )
	mSendChgNotif( cValueChange(), id.getI() );
    return rv;
}


Well::DahObj::PointID Well::DahObj::setValueAt( ZType dh, ValueType v )
{
    mLock4Write();
    const size_type sz = dahs_.size();
    if ( sz > 0 && dh < dahs_.last() + dahEps() )
    {
	idx_type idx = gtIndexOf( dh );
	if ( idx >= 0 )
	{
	    const ZType founddh = dahs_[idx];
	    if ( areEqualDahs( founddh, dh ) )
	    {
		const PointID id = ptids_[idx];
		if ( doSet(idx,v) )
		    mSendChgNotif( cValueChange(), id.getI() );
		return id;
	    }
	}
    }

    const PointID id = doInsAtDah( dh, v );
    if ( id.isValid() )
	mSendChgNotif( cPointAdd(), id.getI() );
    return id;
}


void Well::DahObj::remove( PointID id )
{
    mLock4Write();
    idx_type idx = gtIdx( id );
    if ( dahs_.validIdx(idx) )
    {
	mSendChgNotif( cPointRemove(), id.getI() );
	mReLock();
	idx = gtIdx( id );
	if ( dahs_.validIdx(idx) )
	    doRemove( idx );
    }
}


void Well::DahObj::removeByIdx( idx_type idx )
{
    mLock4Write();
    if ( dahs_.validIdx(idx) )
    {
	const PointID ptid( ptids_[idx] );
	mSendChgNotif( cPointRemove(), ptid.getI() );
	mReLock();
	if ( dahs_.validIdx(idx) && ptids_[idx] == ptid )
	    doRemove( idx );
    }
}


Well::DahObj::PointID Well::DahObj::pointIDFor( idx_type idx ) const
{
    mLock4Read();
    return ptids_.validIdx(idx) ? ptids_[idx] : PointID::getInvalid();
}


Well::DahObj::idx_type Well::DahObj::indexOf( PointID id ) const
{
    mLock4Read();
    return gtIdx( id );
}


Well::DahObj::idx_type Well::DahObj::indexOf( ZType dh ) const
{
    mLock4Read();
    return gtIndexOf( dh );
}


void Well::DahObj::deInterpolate()
{
    mLock4Write();
    const size_type sz = dahs_.size();
    if ( sz < 3 )
	return;

    TypeSet<Coord> bpfinp;
    bpfinp.setCapacity( sz, false );

    for ( idx_type idx=0; idx<sz; idx++ )
	bpfinp += Coord( dahs_[idx]*0.1, gtVal( idx ) );
	// for time we want a fac of 1000, but for track 1. Compromise.

    BendPointFinder2D finder( bpfinp, 1e-5 );
    if ( !finder.execute() || finder.bendPoints().size()<1 )
	return;

    const TypeSet<idx_type>& bpidxs = finder.bendPoints();

    idx_type bpidx = 0;
    TypeSet<idx_type> torem;
    for ( idx_type idx=0; idx<bpfinp.size(); idx++ )
    {
	if ( idx != bpidxs[bpidx] )
	    torem += idx;
	else
	    bpidx++;
    }

    const size_type nrrem = torem.size();
    if ( nrrem > 0 )
    {
	for ( idx_type idx=nrrem-1; idx>-1; idx-- )
	    doRemove( idx );
	mSendEntireObjChgNotif();
    }
}


void Well::DahObj::convertZ( bool tofeet )
{
    mLock4Write();

    const float zfac = tofeet ? mToFeetFactorF : mFromFeetFactorF;
    for ( idx_type idx=0; idx<dahs_.size(); idx++ )
	dahs_[idx] *= zfac;

    mSendEntireObjChgNotif();
}


void Well::DahObj::shiftDahFrom( PointID id, float dahshift )
{
    mLock4Write();
    const idx_type fromidx = gtIdx( id );
    if ( !dahs_.validIdx(fromidx) )
	return;

    if ( fromidx > 0 && (dahs_[fromidx] + dahshift < dahs_[fromidx-1]) )
	{ pErrMsg("Required precondition not met." ); return; }

    for ( idx_type idx=fromidx; idx<dahs_.size(); idx++ )
	dahs_[idx] += dahshift;

    mSendEntireObjChgNotif();
}


Well::DahObj::PointID Well::DahObj::doIns( ZType dh, ValueType val,
			    ValueSetType& vals, bool ascendingvalonly )
{
    PointID id = PointID::getInvalid();
    if ( mIsUdf(dh) || mIsUdf(val) )
	return id;

    if ( dahs_.isEmpty() || dh >= dahs_[dahs_.size()-1] )
    {
	if ( !dahs_.isEmpty() && ascendingvalonly
	    && val <= vals[dahs_.size()-1] )
	    return id;
	id = gtNewPointID();
	dahs_ += dh; vals += val;
	ptids_ += id;
    }
    else
    {
	idx_type insidx = -1;
	if ( dh < dahs_[0] )
	{
	    if ( ascendingvalonly && val >= vals[0] )
		return id;

	    insidx = 0;
	}
	else
	{
	    const idx_type insertidx = gtIndexOf( dh );
	    if ( insertidx < 0 )
		return id;
	    else if ( ascendingvalonly
		&& (val <= vals[insertidx] || val >= vals[insertidx+1]) )
		return id;

	    insidx = insertidx + 1;
	}

	id = gtNewPointID();
	dahs_.insert( insidx, dh );
	vals.insert( insidx, val );
	ptids_.insert( insidx, id );
    }

    return id;
}


bool Well::DahObj::doSetData( const ZSetType& zs, const ValueSetType& newvals,
				ValueSetType& vals )
{
    if ( zs.isEmpty() )
    {
        if ( dahs_.isEmpty() )
	    return false;
	doSetEmpty();
	return true;
    }

    const size_type sz = zs.size();
    if ( sz != dahs_.size() )
    {
	ptids_.setEmpty();
	for ( int idx=0; idx<sz; idx++ )
	    ptids_ += gtNewPointID();
    }
    dahs_ = zs;
    vals = newvals;
    return true;
}


Well::DahObj::PointID Well::DahObj::gtNewPointID() const
{
    return PointID::get( curptidnr_++ );
}


Well::DahObj::idx_type Well::DahObj::gtIndexOf( ZType dh ) const
{
    idx_type idx1 = -1;
    IdxAble::findFPPos( dahs_, dahs_.size(), dh, -1, idx1 );
    return idx1;
}


Well::DahObj::idx_type Well::DahObj::gtIdx( PointID id ) const
{
    if ( id.isValid() )
    {
	const size_type sz = dahs_.size();

	if ( id.getI() < sz && ptids_[id.getI()].getI() == id.getI() )
	    return id.getI();

	for ( idx_type idx=0; idx<sz; idx++ )
	    if ( ptids_[idx] == id )
		return idx;
    }
    return -1;
}


Well::DahObj::ZType Well::DahObj::gtDah( idx_type idx ) const
{
    return dahs_.validIdx(idx) ? dahs_[idx] : mUdf(ZType);
}


void Well::DahObj::doSetEmpty()
{
    dahs_.erase();
    ptids_.erase();
    eraseAux();
}


void Well::DahObj::doRemove( idx_type idx )
{
    dahs_.removeSingle( idx );
    ptids_.removeSingle( idx );
    removeAux( idx );
}


Well::DahObj::ValueType Well::DahObj::gtValueAt( ZType dh, bool noudfs ) const
{
    const size_type sz = dahs_.size();
    if ( sz < 1 )
	return noudfs ? 0 : mUdf(ValueType);

    idx_type idxbefore;
    if ( IdxAble::findFPPos(dahs_,sz,dh,-1,idxbefore) )
    {
	const ValueType valatidx = gtVal( idxbefore );
	if ( !noudfs || !mIsUdf(valatidx) )
	    return valatidx;
	idxbefore--;
    }

    ZType dah1 = mUdf(ZType), dah2 = mUdf(ZType);
    ValueType val1 = mUdf(ValueType), val2 = mUdf(ValueType);
    bool found1 = false, found2 = false;
    for ( idx_type idx=idxbefore; idx>=0; idx-- )
    {
	const ValueType val = gtVal( idx );
	if ( !mIsUdf(val) )
	    { dah1 = dahs_[idx]; val1 = val; found1 = true; break; }
    }
    for ( idx_type idx=idxbefore+1; idx<sz; idx++ )
    {
	const ValueType val = gtVal( idx );
	if ( !mIsUdf(val) )
	    { dah2 = dahs_[idx]; val2 = val; found2 = true; break; }
    }

    if ( !found1 && !found2 )
	return 0;
    else if ( !found1 )
	return val2;
    else if ( !found2 )
	return val1;

    const ZType distfrom1 = dh - dah1; const ZType distfrom2 = dah2 - dh;
    if ( valsAreCodes() )
	return distfrom1 < distfrom2 ? val1 : val2;

    const ZType zdist = dah2 - dah1;
    return zdist ? (distfrom1*val2 + distfrom2*val1) / zdist : val1;
}


Well::DahObjIter::DahObjIter( const DahObj& dahobj, bool atend )
    : MonitorableIter4Read<DahObj::idx_type>(dahobj,
	    atend?dahobj.size()-1:0, atend?0:dahobj.size()-1)
{
}


Well::DahObjIter::DahObjIter( const DahObjIter& oth )
    : MonitorableIter4Read<DahObj::idx_type>(oth)
{
}


const Well::DahObj& Well::DahObjIter::dahObj() const
{
    return static_cast<const Well::DahObj&>( monitored() );
}


Well::DahObjIter::PointID Well::DahObjIter::ID() const
{
    return isValid() ? dahObj().ptids_[curidx_] : PointID::getInvalid();
}


Well::DahObjIter::ZType Well::DahObjIter::dah() const
{
    return isValid() ? dahObj().gtDah( curidx_ ) : mUdf(ZType);
}


Well::DahObjIter::ValueType Well::DahObjIter::value() const
{
    return isValid() ? dahObj().gtVal( curidx_ ) : mUdf(ValueType);
}
