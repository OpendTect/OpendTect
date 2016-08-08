/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/


#include "welldahobj.h"
#include "bendpointfinder.h"
#include "idxable.h"


mDefineInstanceCreatedNotifierAccess(Well::DahObj);


Well::DahObj::DahObj( const char* nm )
    : SharedObject(nm)
    , curptidnr_(0)
{
    mTriggerInstanceCreatedNotifier();
}


Well::DahObj::DahObj( const DahObj& oth )
    : SharedObject(oth)
    , curptidnr_(oth.curptidnr_)
{
    copyAll( oth );
    mTriggerInstanceCreatedNotifier();
}


Well::DahObj::~DahObj()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Well::DahObj, SharedObject )


void Well::DahObj::copyClassData( const DahObj& oth )
{
    dahs_ = oth.dahs_;
    ptids_ = oth.ptids_;
    curptidnr_ = oth.curptidnr_;
}


Well::DahObj::size_type Well::DahObj::size() const
{
    mLock4Read();
    return dahs_.size();
}


Well::DahObj::ZType Well::DahObj::dah( PointID id ) const
{
    mLock4Read();
    return gtDah( gtIdx(id) );
}


Well::DahObj::ZType Well::DahObj::dahByIdx( IdxType idx ) const
{
    mLock4Read();
    return gtDah( idx );
}


Well::DahObj::ValueType Well::DahObj::value( PointID id ) const
{
    mLock4Read();
    return gtVal( gtIdx(id) );
}


Well::DahObj::ValueType Well::DahObj::valueByIdx( IdxType idx ) const
{
    mLock4Read();
    return dahs_.validIdx(idx) ? gtVal( idx ) : mUdf(ValueType);
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
    for ( IdxType idx=2; idx<sz; idx++ )
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


bool Well::DahObj::insertAtDah( ZType z, ValueType v )
{
    mLock4Write();
    if ( !doInsAtDah(z,v) )
	return false;
    mSendChgNotif( cPointAdd(), ptids_.last().getI() );
    return true;
}


bool Well::DahObj::setDah( PointID id, ZType dh )
{
    mLock4Read();
    IdxType idx = gtIdx( id );
    if ( !dahs_.validIdx(idx) )
	return false;

    if ( !mLock2Write() )
    {
	idx = gtIdx( id );
	if ( !dahs_.validIdx(idx) )
	    return false;
    }

    dahs_[idx] = dh;
    mSendChgNotif( cDahChange(), id.getI() );
    return true;
}


void Well::DahObj::remove( PointID id )
{
    mLock4Write();
    IdxType idx = gtIdx( id );
    if ( dahs_.validIdx(idx) )
    {
	mSendChgNotif( cPointRemove(), id.getI() );
	mReLock();
	idx = gtIdx( id );
	if ( dahs_.validIdx(idx) )
	    doRemove( idx );
    }
}


void Well::DahObj::removeByIdx( IdxType idx )
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


Well::DahObj::PointID Well::DahObj::pointIDFor( IdxType idx ) const
{
    mLock4Read();
    return ptids_.validIdx(idx) ? ptids_[idx] : PointID::getInvalid();
}


Well::DahObj::IdxType Well::DahObj::indexOf( ZType dh ) const
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

    for ( IdxType idx=0; idx<sz; idx++ )
	bpfinp += Coord( dahs_[idx]*0.1, gtVal( idx ) );
	// for time we want a fac of 1000, but for track 1. Compromise.

    BendPointFinder2D finder( bpfinp, 1e-5 );
    if ( !finder.execute() || finder.bendPoints().size()<1 )
	return;

    const TypeSet<IdxType>& bpidxs = finder.bendPoints();

    IdxType bpidx = 0;
    TypeSet<IdxType> torem;
    for ( IdxType idx=0; idx<bpfinp.size(); idx++ )
    {
	if ( idx != bpidxs[bpidx] )
	    torem += idx;
	else
	    bpidx++;
    }

    const size_type nrrem = torem.size();
    if ( nrrem > 0 )
    {
	for ( IdxType idx=nrrem-1; idx>-1; idx-- )
	    doRemove( idx );
	mSendEntireObjChgNotif();
    }
}


void Well::DahObj::shiftDah( PointID id, float extradah )
{
    mLock4Write();
    const IdxType fromidx = gtIdx( id );
    if ( !dahs_.validIdx(fromidx) )
	return;

    if ( fromidx > 0 && (dahs_[fromidx] + extradah < dahs_[fromidx-1]) )
	{ pErrMsg("Required precondition not met." ); return; }

    for ( IdxType idx=fromidx; idx<dahs_.size(); idx++ )
	dahs_[idx] += extradah;

    mSendEntireObjChgNotif();
}


bool Well::DahObj::doIns( ZType dh, ValueType val, ValueSetType& vals,
				bool ascendingvalonly )
{
    if ( mIsUdf(dh) || mIsUdf(val) )
	return false;

    if ( dahs_.isEmpty() || dh >= dahs_[dahs_.size()-1] )
    {
	if ( !dahs_.isEmpty() && ascendingvalonly
	    && val <= vals[dahs_.size()-1] )
	    return false;
	dahs_ += dh; vals += val;
	ptids_ += gtNewPointID();
    }
    else
    {
	IdxType insidx = -1;
	if ( dh < dahs_[0] )
	{
	    if ( ascendingvalonly && val >= vals[0] )
		return false;
	}
	else
	{
	    const IdxType insertidx = gtIndexOf( dh );
	    if ( insertidx < 0 )
		return false;
	    if ( ascendingvalonly
		&& (val <= vals[insertidx] || val >= vals[insertidx+1]) )
		return false;

	    insidx = insertidx + 1;
	}
	dahs_.insert( insidx, dh );
	vals.insert( insidx, val );
	ptids_.insert( insidx, gtNewPointID() );
    }

    return true;
}


Well::DahObj::PointID Well::DahObj::gtNewPointID() const
{
    return PointID::get( curptidnr_++ );
}


Well::DahObj::IdxType Well::DahObj::gtIndexOf( ZType dh ) const
{
    IdxType idx1 = -1;
    IdxAble::findFPPos( dahs_, dahs_.size(), dh, -1, idx1 );
    return idx1;
}


Well::DahObj::IdxType Well::DahObj::gtIdx( PointID id ) const
{
    if ( id.isValid() )
    {
	const size_type sz = dahs_.size();

	if ( id.getI() < sz && ptids_[id.getI()].getI() == id.getI() )
	    return id.getI();

	for ( IdxType idx=0; idx<sz; idx++ )
	    if ( ptids_[idx] == id )
		return idx;
    }
    return -1;
}


Well::DahObj::ZType Well::DahObj::gtDah( IdxType idx ) const
{
    return dahs_.validIdx(idx) ? dahs_[idx] : mUdf(ZType);
}


void Well::DahObj::doSetEmpty()
{
    dahs_.erase();
    ptids_.erase();
    eraseAux();
}


void Well::DahObj::doRemove( IdxType idx )
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

    IdxType idxbefore;
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
    for ( IdxType idx=idxbefore; idx>=0; idx-- )
    {
	const ValueType val = gtVal( idx );
	if ( !mIsUdf(val) )
	    { dah1 = dahs_[idx]; val1 = val; found1 = true; break; }
    }
    for ( IdxType idx=idxbefore+1; idx<sz; idx++ )
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
