/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jan 2007
-*/


#include "datapack.h"

#include "ascstream.h"
#include "atomic.h"
#include "debug.h"
#include "file.h"
#include "iopar.h"
#include "keystrs.h"
#include "od_ostream.h"
#include "envvars.h"
#include "msgh.h"

#include <iostream>
#include <string.h>

mUseType( DataPack,	value_type );
mUseType( DataPack,	arrnd_type );
mUseType( DataPack,	kb_size_type );
mUseType( DataPack,	total_size_type );
mUseType( DataPack,	FullID );
mUseType( FullID,	MgrID );
mUseType( FullID,	PackID );


DataPackMgr::ID DataPackMgr::BufID()		{ return ID::get(1); }
DataPackMgr::ID DataPackMgr::PointID()		{ return ID::get(2); }
DataPackMgr::ID DataPackMgr::SeisID()		{ return ID::get(3); }
DataPackMgr::ID DataPackMgr::FlatID()		{ return ID::get(4); }
DataPackMgr::ID DataPackMgr::SurfID()		{ return ID::get(5); }
const char* DataPack::sKeyCategory()		{ return "Category"; }
kb_size_type DataPack::sKb2MbFac()	{ return 0.0009765625; }

Threads::Lock DataPackMgr::mgrlistlock_;
ManagedObjectSet<DataPackMgr> DataPackMgr::mgrs_;

#ifdef __debug__
# define mTrackDPMsg(msg) \
    if ( trackDataPacks() ) \
	DBG::message( msg )
#else
# define mTrackDPMsg(msg) \
    if ( trackDataPacks() ) \
	UsrMsg( msg, MsgClass::Info )
#endif

static bool trackDataPacks()
{
    mDefineStaticLocalObject( bool, dotrack,
			      = GetEnvVarYN( "OD_TRACK_DATAPACKS" ) );
#ifdef __debug__
    if ( !DBG::isOn() )
	DBG::turnOn( DBG_DBG );
#endif
    return dotrack;
}


DataPack::FullID DataPack::FullID::getFromString( const char* str )
{
    FullID fid = getInvalid();
    if ( !isValidString(str) )
	return fid;
    fid.fromString( str );
    return fid;
}


DataPack::FullID DataPack::FullID::getInvalid()
{
    const GroupedID grid = GroupedID::getInvalid();
    return FullID( grid.groupID(), grid.objID() );
}


bool DataPack::FullID::isInDBKey( const DBKey& dbky )
{
    if ( !dbky.hasAuxKey() )
	return false;
    const BufferString auxval = dbky.auxKey();
    if ( auxval.firstChar() != '#' )
	return false;

    const char* dpstr = auxval.str() + 1;
    return isValidString( dpstr );
}


DataPack::FullID DataPack::FullID::getFromDBKey( const DBKey& dbky )
{
    if ( !dbky.hasAuxKey() )
	return getInvalid();

    const BufferString auxval = dbky.auxKey();
    if ( auxval.firstChar() != '#' )
	return getInvalid();

    const char* dpstr = auxval.str() + 1;
    return getFromString( dpstr );
}


void DataPack::FullID::putInDBKey( DBKey& dbky ) const
{
    const BufferString aux( "#", toString() );
    dbky.setAuxKey( aux );
}


mDefineInstanceCreatedNotifierAccess(DataPack)
static Threads::Atomic<PackID::IDType> curdpidnr( 0 );

DataPack::DataPack( const char* categry )
    : SharedObject("<?>")
    , category_(categry)
    , manager_( 0 )
    , id_(getNewID())
{
    mTriggerInstanceCreatedNotifier();
}


DataPack::DataPack( const DataPack& oth )
    : SharedObject(oth)
    , category_(oth.category_)
    , manager_(0)
    , id_(getNewID())
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


DataPack::~DataPack()
{
    if ( manager_ )
    {
	mTrackDPMsg( BufferString("[DP]: delete ",id_.getI(),
		     BufferString(" '",name(),"'")) );
    }

    sendDelNotif();
}


mImplMonitorableAssignment( DataPack, SharedObject )


void DataPack::copyClassData( const DataPack& oth )
{
    dbkey_ = oth.dbkey_;
}


Monitorable::ChangeType DataPack::compareClassData( const DataPack& oth ) const
{
    mDeliverYesNoMonitorableCompare( id_ == oth.id_ );
}


DataPack::ID DataPack::getNewID()
{
    return ID::get( ++curdpidnr );
}


void DataPack::setManager( const DataPackMgr* mgr )
{
    if ( manager_ && mgr )
    {
	if ( manager_ != mgr )
	    DBG::forceCrash( false );

	return;
    }

    manager_ = mgr;
}


DataPackMgr* DataPackMgr::gtDPM( DataPackMgr::ID dpid, bool crnew )
{
    Threads::Locker lock( mgrlistlock_ );

    for ( auto mgr : mgrs_ )
    {
	if ( mgr->id() == dpid )
	    return mgr;
    }
    if ( !crnew )
	return 0;

    DataPackMgr* newmgr = new DataPackMgr( dpid );
    mgrs_ += newmgr;
    return newmgr;
}


DataPackMgr& DataPackMgr::DPM( DataPackMgr::ID dpid )
{
    return *gtDPM( dpid, true );
}


DataPackMgr& DPM( DataPackMgr::ID dpid )
{
    return DataPackMgr::DPM( dpid );
}


DataPackMgr& DPM( const DataPack::FullID& fid )
{
    const DataPackMgr::ID manid = fid.mgrID();
    DataPackMgr* dpm = DataPackMgr::gtDPM( manid, false );
    if ( dpm ) return *dpm;

    mDefineStaticLocalObject( PtrMan<DataPackMgr>, emptymgr, = 0 );
    emptymgr = new DataPackMgr( manid );
    return *emptymgr;
}



const char* DataPackMgr::nameOf( const DataPack::FullID& fid )
{
    return ::DPM(fid).nameOf( fid.packID() );
}


const char* DataPackMgr::categoryOf( const DataPack::FullID& fid )
{
    return ::DPM(fid).categoryOf( fid.packID() );
}


void DataPackMgr::dumpDPMs( od_ostream& strm )
{
    Threads::Locker lock( mgrlistlock_ );
    strm << "** Data Pack Manager dump **\n";
    if ( mgrs_.isEmpty() )
	{ strm << "No Data pack managers (yet)" << od_newline; return; }

    for ( auto mgr : mgrs_ )
    {
	strm << "\n\n";
	mgr->dumpInfo( strm );
    }
}


DataPackMgr::DataPackMgr( DataPackMgr::ID dpid )
    : id_(dpid)
    , newPack(this)
    , packToBeRemoved(this)
{}


DataPackMgr::~DataPackMgr()
{
#ifdef __debug__
    //Don't do in release mode, as we may have race conditions of sta-tic
    //variables deleting at different times
    for ( idx_type idx=0; idx<packs_.size(); idx++ )
    {
	ConstRefMan<DataPack> pack = packs_[idx];
	if ( !pack )
	    continue;

	//Using std C++ function because we cannot use pErrMsg or BufferString
	std::cerr << "(PE) DataPackMgr | Datapack " << pack->id().getI();
	if ( pack->category() )
	    std::cerr << " with category " << pack->category();
	std::cerr << " is still referenced.\n";
    }

#endif
}


kb_size_type DataPackMgr::nrKBytes() const
{
    kb_size_type res = 0;
    for ( idx_type idx=0; idx<packs_.size(); idx++ )
    {
        ConstRefMan<DataPack> pack = packs_[idx];
        pack.setNoDelete(true);
	if ( pack )
	    res += pack->nrKBytes();
    }
    return res;
}


RefMan<DataPack> DataPackMgr::getDP( PackID dpid )
{
    const RefCount::WeakPtrSetBase::CleanupBlocker cleanupblock( packs_ );

    for ( idx_type idx=0; idx<packs_.size(); idx++ )
    {
        RefMan<DataPack> pack = packs_[idx];
        if ( pack && pack->id() == dpid )
            return pack;

        pack.setNoDelete( true );
    }

    return 0;
}



ConstRefMan<DataPack> DataPackMgr::getDP( PackID dpid ) const
{
    const RefCount::WeakPtrSetBase::CleanupBlocker cleanupblock( packs_ );

    for ( idx_type idx=0; idx<packs_.size(); idx++ )
    {
        ConstRefMan<DataPack> pack = packs_[idx];
        if ( pack && pack->id() == dpid )
            return pack;

        pack.setNoDelete( true );
    }

    return 0;
}


WeakPtr<DataPack> DataPackMgr::observeDP( PackID dpid ) const
{
    const RefCount::WeakPtrSetBase::CleanupBlocker cleanupblock( packs_ );
    for ( idx_type idx=0; idx<packs_.size(); idx++ )
    {
        RefMan<DataPack> pack = packs_[idx];
        pack.setNoDelete(true);
        if ( pack && pack->id() == dpid )
            return WeakPtr<DataPack>( pack );
    }

    return 0;
}


bool DataPackMgr::isPresent( PackID packid ) const
{
    const RefCount::WeakPtrSetBase::CleanupBlocker cleanupblock( packs_ );

    for ( idx_type idx=0; idx<packs_.size(); idx++ )
    {
        ConstRefMan<DataPack> pack = packs_[idx];
        pack.setNoDelete(true);
        if ( pack && pack->id() == packid )
	    return true;
    }
    return false;
}


void DataPackMgr::getPackIDs( TypeSet<PackID>& ids ) const
{
    const RefCount::WeakPtrSetBase::CleanupBlocker cleanupblock( packs_ );

    for ( idx_type idx=0; idx<packs_.size(); idx++ )
    {
        ConstRefMan<DataPack> pack = packs_[idx];
        pack.setNoDelete(true);
        if ( pack )
	    ids += pack->id();
    }
}


void DataPackMgr::dumpInfo( od_ostream& strm ) const
{
    strm << "Manager.ID: " << id().getI() << od_newline;
    const auto nrkb = (od_int64)( nrKBytes() + 0.5f );
    strm << "Total memory: " << File::getFileSizeString(nrkb)
			     << od_newline;
    ascostream astrm( strm );
    astrm.newParagraph();

    const RefCount::WeakPtrSetBase::CleanupBlocker cleanupblock( packs_ );
    for ( idx_type idx=0; idx<packs_.size(); idx++ )
    {
        ConstRefMan<DataPack> pack = packs_[idx];
        pack.setNoDelete(true);
	if ( !pack )
	    continue;

	IOPar iop;
	pack->dumpInfo( iop );
	iop.putTo( astrm, true );
    }
}


bool DataPackMgr::doAdd( const DataPack* constdp )
{
    DataPack* dp = const_cast<DataPack*>( constdp );
    if ( !dp )
	{ pErrMsg("null datapack to add"); return false; }
    else if ( isPresent(dp->id()) )
	return false;

    RefMan<DataPack> keeper = dp;
    keeper.setNoDelete( true );
    dp->setManager( this );

    packs_ += dp;

    mTrackDPMsg( BufferString("[DP]: add ",dp->id().getI(),
		 BufferString(" '",dp->name(),"'")) );

    newPack.trigger( dp );
    return true;
}


DataPack* DataPackMgr::addAndObtain( DataPack* dp )
{
    if ( !dp )
	return 0;

    dp->ref();
    dp->setManager( this );

    const bool added = packs_ += dp;
    if ( added )
    {
	mTrackDPMsg( BufferString("[DP]: add+obtain ",dp->id().getI(),
		     BufferString(" '",dp->name(),"'")) );
    }
    else
    {
	mTrackDPMsg( BufferString("[DP]: add+obtain [existing!] ",
		    dp->id().getI(), BufferString(" nrusers=",dp->nrRefs())) );
    }

    if ( added )
	newPack.trigger( dp );

    return dp;
}


DataPack* DataPackMgr::obtain( PackID dpid )
{
    auto pack = getDP( dpid );
    if ( pack )
	pack->ref();
    return pack.ptr();
}


const DataPack* DataPackMgr::obtain( PackID dpid ) const
{
    auto pack = getDP( dpid );
    if ( pack )
	pack->ref();
    return pack.ptr();
}


bool DataPackMgr::ref( PackID dpid )
{
    auto pack = getDP( dpid );
    if ( pack )
	{ pack->ref(); return true; }
    return false;
}


bool DataPackMgr::unRef( PackID dpid )
{
    auto pack = getDP( dpid );
    if ( pack )
	{ pack->unRef(); return true; }

    return false;
}


void DataPackMgr::release( PackID dpid )
{
    unRef( dpid );
}


void DataPackMgr::releaseAll( bool notif )
{
}


const char* DataPackMgr::nameOf( PackID dpid ) const
{
    auto pack = getDP( dpid );
    if ( !pack )
        return 0;

    pack.setNoDelete( true );
    return pack->name();
}


const char* DataPackMgr::categoryOf( PackID dpid ) const
{
    auto pack = getDP( dpid );
    if ( !pack )
        return 0;

    pack.setNoDelete( true );
    return pack->category();
}


kb_size_type DataPackMgr::nrKBytesOf( PackID dpid ) const
{
    auto pack = getDP( dpid );
    if ( !pack )
        return 0;

    pack.setNoDelete( true );
    return pack->nrKBytes();
}


void DataPackMgr::dumpInfoFor( PackID dpid, IOPar& iop ) const
{
    ConstRefMan<DataPack> pack = getDP( dpid );
    pack.setNoDelete( true );
    if ( pack )
	pack->dumpInfo( iop );
}


void DataPack::doDumpInfo( IOPar& iop ) const
{
    iop.set( sKeyCategory(), category() );
    iop.set( sKey::Name(), name() );
    iop.set( "Pack.ID", id_ );
    iop.set( "Nr users", nrRefs() );
    const od_int64 nrkb = (od_int64)(nrKBytes() + 0.5f);
    iop.set( "Memory consumption", File::getFileSizeString(nrkb) );
    const DBKey dbky( dbKey() );
    BufferString dbkystr( "-" );
    if ( dbky.isValid() )
	dbkystr.set( dbKey().toString() );
    iop.set( "DB Key", dbkystr );
}


BufferDataPack::BufferDataPack( char* b, total_size_type sz,
				const char* catgry )
    : DataPack(catgry)
    , buf_(0)
{
    setBuf( b, sz );
}


BufferDataPack::BufferDataPack( const BufferDataPack& oth )
    : DataPack(oth)
    , buf_(0)
    , sz_(0)
{
    copyClassData( oth );
}


BufferDataPack::~BufferDataPack()
{
    sendDelNotif();
    delete [] buf_;
}


mImplMonitorableAssignment( BufferDataPack, DataPack )


void BufferDataPack::copyClassData( const BufferDataPack& oth )
{
    mkNewBuf( oth.sz_ );
    if ( buf_ )
	OD::memCopy( buf_, oth.buf_, sz_ );
}


Monitorable::ChangeType BufferDataPack::compareClassData(
					const BufferDataPack& oth ) const
{
    if ( sz_ != oth.sz_ )
	return cEntireObjectChange();
    else if ( !buf_ )
	return cNoChange();

    mDeliverYesNoMonitorableCompare( !memcmp(buf_,oth.buf_,sz_) );
}


void BufferDataPack::setBuf( char* b, total_size_type sz )
{
    delete [] buf_;
    buf_ = b;
    sz_ = buf_ ? sz : 0;
}


bool BufferDataPack::mkNewBuf( total_size_type sz )
{
    delete [] buf_;
    setBuf( createBuf(sz), sz );
    return sz_ > 0;
}


char* BufferDataPack::createBuf( total_size_type sz )
{
    char* ret = 0;
    if ( sz > 0 )
	{ mTryAlloc( ret, char [sz] ); }
    return ret;
}
