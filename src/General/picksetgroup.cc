/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : June 2017
-*/

#include "picksetgroup.h"

mDefineInstanceCreatedNotifierAccess(Pick::SetGroup)


Pick::SetGroup::SetGroup( const char* nm )
    : SharedObject(nm)
    , cursetidnr_(0)
{
    mTriggerInstanceCreatedNotifier();
}


Pick::SetGroup::SetGroup( const SetGroup& oth )
    : SharedObject(oth)
    , cursetidnr_(oth.cursetidnr_)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


Pick::SetGroup::~SetGroup()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Pick::SetGroup, SharedObject )

void Pick::SetGroup::copyClassData( const SetGroup& oth )
{
    for ( int idx=0; idx<sets_.size(); idx++ )
	sets_[idx]->unRef();
    sets_.erase();
    for ( int idx=0; idx<oth.sets_.size(); idx++ )
    {
	Set* newset = new Set( *oth.sets_[idx] );
	sets_ += newset;
	newset->ref();
    }

    setids_ = oth.setids_;
    cursetidnr_ = oth.cursetidnr_;
}


Monitorable::ChangeType Pick::SetGroup::compareClassData(
						const SetGroup& oth ) const
{
    if ( sets_.size() != oth.sets_.size() )
	return cEntireObjectChange();

    for ( int idx=0; idx<sets_.size(); idx++ )
	if ( *sets_[idx] != *oth.sets_[idx] )
	    return cEntireObjectChange();

    return cNoChange();
}


Pick::SetGroup::idx_type Pick::SetGroup::gtIdx( SetID id ) const
{
    if ( id.isInvalid() )
	return -1;

    const int sz = sets_.size();
    if ( id.getI() < sz && setids_[id.getI()] == id )
	return id.getI();
    for ( idx_type idx=0; idx<setids_.size(); idx++ )
	if ( setids_[idx] == id )
	    return idx;

    return -1;
}


Pick::Set* Pick::SetGroup::gtSet( SetID id ) const
{
    const idx_type idx = gtIdx( id );
    return idx < 0 ? 0 : const_cast<Set*>( sets_[idx] );
}


Pick::SetGroup::SetID Pick::SetGroup::gtID( const Set* st ) const
{
    for ( idx_type idx=0; idx<sets_.size(); idx++ )
    {
	if ( sets_[idx] == st )
	    return setids_[idx];
    }
    return SetID::getInvalid();
}


Pick::SetGroup::idx_type Pick::SetGroup::gtIdxByName( const char* nm ) const
{
    for ( idx_type idx=0; idx<sets_.size(); idx++ )
    {
	const Set& l = *sets_[idx];
	if ( l.hasName(nm) )
	    return idx;
    }
    return -1;
}


Pick::Set* Pick::SetGroup::gtSetByName( const char* nm ) const
{
    const idx_type idx = gtIdxByName( nm );
    return idx < 0 ? 0 : const_cast<Set*>( sets_[idx] );
}


Pick::Set* Pick::SetGroup::gtSetByIdx( idx_type idx ) const
{
    return sets_.validIdx(idx) ? const_cast<Set*>( sets_[idx] ) : 0;
}


Pick::Set* Pick::SetGroup::doRemove( idx_type idx )
{
    Set* st = sets_.removeSingle( idx );
    setids_.removeSingle( idx );
    return st;
}


void Pick::SetGroup::doSetEmpty()
{
    deepUnRef( sets_ );
}


Pick::SetGroup::SetRefMan Pick::SetGroup::getSet( SetID id )
{
    mLock4Read();
    return gtSet( id );
}


Pick::SetGroup::CSetRefMan Pick::SetGroup::getSet( SetID id ) const
{
    mLock4Read();
    return gtSet( id );
}


Pick::SetGroup::SetRefMan Pick::SetGroup::getSetByName( const char* nm )
{
    mLock4Read();
    return gtSetByName( nm );
}


Pick::SetGroup::CSetRefMan Pick::SetGroup::getSetByName( const char* nm ) const
{
    mLock4Read();
    return gtSetByName( nm );
}


Pick::SetGroup::SetRefMan Pick::SetGroup::getSetByIdx( idx_type idx )
{
    mLock4Read();
    return gtSetByIdx( idx );
}


Pick::SetGroup::CSetRefMan Pick::SetGroup::getSetByIdx( idx_type idx ) const
{
    mLock4Read();
    return gtSetByIdx( idx );
}


Pick::SetGroup::SetRefMan Pick::SetGroup::firstSet()
{
    mLock4Read();
    return sets_.isEmpty() ? 0 : sets_[0];
}


Pick::SetGroup::CSetRefMan Pick::SetGroup::firstSet() const
{
    mLock4Read();
    return sets_.isEmpty() ? 0 : sets_[0];
}


Pick::SetGroup::size_type Pick::SetGroup::size() const
{
    mLock4Read();
    return sets_.size();
}


od_int64 Pick::SetGroup::nrLocations() const
{
    mLock4Read();
    od_int64 ret = 0;
    for ( idx_type idx=0; idx<sets_.size(); idx++ )
	ret += sets_[idx]->size();
    return ret;
}


Pick::SetGroup::SetID Pick::SetGroup::getID( idx_type idx ) const
{
    mLock4Read();
    return setids_.validIdx( idx ) ? setids_[idx] : SetID::getInvalid();
}


Pick::SetGroup::idx_type Pick::SetGroup::indexOf( SetID id ) const
{
    mLock4Read();
    return gtIdx( id );
}


Pick::SetGroup::idx_type Pick::SetGroup::indexOf( const char* nm ) const
{
    mLock4Read();
    return gtIdxByName( nm );
}


bool Pick::SetGroup::validIdx( idx_type idx ) const
{
    mLock4Read();
    return sets_.validIdx( idx );
}


void Pick::SetGroup::setEmpty()
{
    if ( isEmpty() )
	return;

    mLock4Write();
    doSetEmpty();
    mSendEntireObjChgNotif();
}


Pick::SetGroup::SetID Pick::SetGroup::add( Pick::Set* st )
{
    SetID setid = SetID::getInvalid();
    if ( !st )
	return setid;
    st->ref();

    mLock4Write();
    const Set* existset = gtSetByName( st->name() );
    int existidx = existset ? sets_.indexOf(existset) : -1;

    setid = SetID::get( cursetidnr_++ );
    setids_ += setid;
    sets_ += st;

    if ( existidx >= 0 )
    {
	const SetID rmid = setids_[existidx];
	Set* torem = gtSetByIdx( existidx );
	mSendChgNotif( cSetRemove(), rmid.getI() );
	mReLock();
	existidx = sets_.indexOf( existset );
	if ( existidx >= 0 )
	{
	    doRemove( existidx );
	    torem->unRef();
	}
    }

    mSendChgNotif( cSetAdd(), setid.getI() );
    return setid;
}


Pick::SetGroup::SetRefMan Pick::SetGroup::remove( SetID id )
{
    mLock4Write();
    idx_type idx = gtIdx( id );
    if ( idx < 0 )
	return 0;

    mSendChgNotif( cSetRemove(), id.getI() );
    mReLock();
    idx = gtIdx( id );
    if ( idx < 0 )
	return 0;

    SetRefMan refman = doRemove( idx );
    refman.setNoDelete( true );
    return refman;
}


Pick::SetGroup::SetRefMan Pick::SetGroup::removeByName( const char* nm )
{
    mLock4Write();
    idx_type idx = gtIdxByName( nm );
    if ( idx < 0 )
	return 0;

    SetID setid = setids_[idx];
    mSendChgNotif( cSetRemove(), setid.getI() );
    mReLock();
    idx = gtIdx( setid );
    if ( idx < 0 )
	return 0;

    return doRemove( idx );
}


bool Pick::SetGroup::isPresent( const char* nm ) const
{
    mLock4Read();
    for ( idx_type idx=0; idx<sets_.size(); idx++ )
	if ( sets_[idx]->hasName(nm) )
	    return true;
    return false;
}


void Pick::SetGroup::getNames( BufferStringSet& nms ) const
{
    mLock4Read();
    for ( idx_type idx=0; idx<sets_.size(); idx++ )
	nms.add( sets_[idx]->name() );
}


bool Pick::SetGroup::swap( idx_type idx1, idx_type idx2 )
{
    mLock4Write();
    if ( !sets_.validIdx(idx1) || !sets_.validIdx(idx2) )
	return false;

    sets_.swap( idx1, idx2 );
    setids_.swap( idx1, idx2 );
    mSendChgNotif( cOrderChange(), setids_[idx1].getI() );
    return true;
}


// Pick::SetGroupIter

Pick::SetGroupIter::SetGroupIter( const SetGroup& ls, bool atend )
    : MonitorableIter4Read<SetGroup::idx_type>(ls,
	    atend?ls.size()-1:0,atend?0:ls.size()-1)
{
}


Pick::SetGroupIter::SetGroupIter( const SetGroupIter& oth )
    : MonitorableIter4Read<SetGroup::idx_type>(oth)
{
}


const Pick::SetGroup& Pick::SetGroupIter::setGroup() const
{
    return static_cast<const SetGroup&>( monitored() );
}


Pick::SetGroup::SetID Pick::SetGroupIter::ID() const
{
    return isValid() ? setGroup().setids_[ curidx_ ]
		     : Pick::SetGroup::SetID::getInvalid();
}


const Pick::Set& Pick::SetGroupIter::set() const
{
    return isValid() ? *setGroup().sets_[curidx_] : Set::emptySet();
}
