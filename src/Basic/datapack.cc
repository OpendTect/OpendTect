/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
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

DataPackMgr::ID DataPackMgr::BufID()	{ return ID::get(1); }
DataPackMgr::ID DataPackMgr::PointID()	{ return ID::get(2); }
DataPackMgr::ID DataPackMgr::SeisID()	{ return ID::get(3); }
DataPackMgr::ID DataPackMgr::FlatID()	{ return ID::get(4); }
DataPackMgr::ID DataPackMgr::SurfID()	{ return ID::get(5); }
const char* DataPack::sKeyCategory()	{ return "Category"; }
float DataPack::sKb2MbFac()		{ return 0.0009765625; }

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
static Threads::Atomic<int> curdpidnr( 0 );

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

    for ( int idx=0; idx<mgrs_.size(); idx++ )
    {
	DataPackMgr* mgr = mgrs_[idx];
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
    if ( !mgrs_.size() )
	{ strm << "No Data pack managers (yet)" << od_newline; return; }

    for ( int imgr=0; imgr<mgrs_.size(); imgr++ )
    {
	strm << "\n\n";
	mgrs_[imgr]->dumpInfo(strm);
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
    for ( int idx=0; idx<packs_.size(); idx++ )
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


bool DataPackMgr::haveID( DataPack::ID dpid ) const
{
    RefMan<DataPack> pack = get( dpid );
    pack.setNoDelete( true );
    return pack;
}


float DataPackMgr::nrKBytes() const
{
    float res = 0;
    for ( int idx=0; idx<packs_.size(); idx++ )
    {
        ConstRefMan<DataPack> pack = packs_[idx];
        pack.setNoDelete(true);
	if ( pack )
	{
	    res += pack->nrKBytes();
	}
    }
    return res;
}


RefMan<DataPack> DataPackMgr::get( DataPack::ID dpid ) const
{
    const RefCount::WeakPtrSetBase::CleanupBlocker cleanupblock( packs_ );
    
    for ( int idx=0; idx<packs_.size(); idx++ )
    {
        RefMan<DataPack> pack = packs_[idx];
        if ( pack && pack->id() == dpid )
            return pack;
            
    
        pack.setNoDelete( true );
    }
    
    return 0;
}


WeakPtr<DataPack> DataPackMgr::observe( DataPack::ID dpid ) const
{
    const RefCount::WeakPtrSetBase::CleanupBlocker cleanupblock( packs_ );
    for ( int idx=0; idx<packs_.size(); idx++ )
    {
        RefMan<DataPack> pack = packs_[idx];
        pack.setNoDelete(true);
        if ( pack && pack->id() == dpid )
        {
            return pack;
        }
    }
    
    return 0;
}



void DataPackMgr::getPackIDs( TypeSet<DataPack::ID>& ids ) const
{
    const RefCount::WeakPtrSetBase::CleanupBlocker cleanupblock( packs_ );
    
    for ( int idx=0; idx<packs_.size(); idx++ )
    {
        ConstRefMan<DataPack> pack = packs_[idx];
        pack.setNoDelete(true);
        if ( pack )
	{
	    ids += pack->id();
	}
    }
}


void DataPackMgr::dumpInfo( od_ostream& strm ) const
{
    strm << "Manager.ID: " << id().getI() << od_newline;
    const od_int64 nrkb = mCast(od_int64,nrKBytes());
    strm << "Total memory: " << File::getFileSizeString(nrkb)
			     << od_newline;
    ascostream astrm( strm );
    astrm.newParagraph();
    
    const RefCount::WeakPtrSetBase::CleanupBlocker cleanupblock( packs_ );
    for ( int idx=0; idx<packs_.size(); idx++ )
    {
        ConstRefMan<DataPack> pack = packs_[idx];
        pack.setNoDelete(true);
	if ( !pack )
	    continue;

	IOPar iop;
	pack->dumpInfo( iop );
	iop.putTo( astrm );
    }
}


void DataPackMgr::doAdd( DataPack* dp )
{
    if ( !dp ) return;

    RefMan<DataPack> keeper = dp;
    keeper.setNoDelete( true );
    dp->setManager( this );
    
    packs_ += dp;

    mTrackDPMsg( BufferString("[DP]: add ",dp->id().getI(),
		 BufferString(" '",dp->name(),"'")) );

    newPack.trigger( dp );
}


DataPack* DataPackMgr::addAndObtain( DataPack* dp )
{
    if ( !dp ) return 0;

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

DataPack* DataPackMgr::obtain( DataPack::ID dpid )
{
    RefMan<DataPack> pack = get( dpid );
    if ( pack ) pack->ref();
    return pack.ptr();
}


const DataPack* DataPackMgr::obtain( DataPack::ID dpid ) const
{
    ConstRefMan<DataPack> pack = get( dpid );
    if ( pack ) pack->ref();
    return pack.ptr();
}


bool DataPackMgr::ref( DataPack::ID dpid )
{
    RefMan<DataPack> pack = get( dpid );
    if ( pack )
    {
        pack->ref();
        return true;
    }
    return false;
}


bool DataPackMgr::unRef( DataPack::ID dpid )
{
    RefMan<DataPack> pack = get( dpid );
    if ( pack )
    {
        pack->unRef();
        return true;
    }
    
    return false;
}


void DataPackMgr::release( DataPack::ID dpid )
{
    unRef( dpid );
}


void DataPackMgr::releaseAll( bool notif )
{
}


#define mDefDPMDataPackFn(ret,fn) \
ret DataPackMgr::fn##Of( DataPack::ID dpid ) const \
{ \
    const int idx = indexOf( dpid ); if ( idx < 0 ) return 0; \
    const DataPack* pack = (const DataPack*) refPtr( packs_[idx].get().ptr() );\
    ret res = pack ? (ret) pack->fn() : (ret) 0; \
    unRefNoDeletePtr( pack ); \
    return res; \
}

const char* DataPackMgr::nameOf( const DataPack::ID dpid ) const
{
    RefMan<DataPack> pack = get( dpid );
    if ( !pack )
        return 0;
    
    pack.setNoDelete( true );
    return pack->name();
}


const char* DataPackMgr::categoryOf( const DataPack::ID dpid ) const
{
    RefMan<DataPack> pack = get( dpid );
    if ( !pack )
        return 0;
    
    pack.setNoDelete( true );
    return pack->category();
}


float DataPackMgr::nrKBytesOf( const DataPack::ID dpid ) const
{
    RefMan<DataPack> pack = get( dpid );
    if ( !pack )
        return 0;
    
    pack.setNoDelete( true );
    return pack->nrKBytes();
}


void DataPackMgr::dumpInfoFor( DataPack::ID dpid, IOPar& iop ) const
{
    RefMan<DataPack> pack = get( dpid );
    pack.setNoDelete( true );
    if ( pack ) pack->dumpInfo( iop );
}


void DataPack::doDumpInfo( IOPar& iop ) const
{
    iop.set( sKeyCategory(), category() );
    iop.set( sKey::Name(), name() );
    iop.set( "Pack.ID", id_ );
    iop.set( "Nr users", nrRefs() );
    const od_int64 nrkb = mCast(od_int64,nrKBytes());
    iop.set( "Memory consumption", File::getFileSizeString(nrkb) );
    const DBKey dbky( dbKey() );
    BufferString dbkystr( "-" );
    if ( dbky.isValid() )
	dbkystr.set( dbKey().toString() );
    iop.set( "DB Key", dbkystr );
}


BufferDataPack::BufferDataPack( char* b, od_int64 sz, const char* catgry )
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


void BufferDataPack::setBuf( char* b, od_int64 sz )
{
    delete [] buf_;
    buf_ = b;
    sz_ = buf_ ? sz : 0;
}


bool BufferDataPack::mkNewBuf( od_int64 sz )
{
    delete [] buf_;
    setBuf( createBuf(sz), sz );
    return sz_ > 0;
}


char* BufferDataPack::createBuf( od_int64 sz )
{
    char* ret = 0;
    if ( sz > 0 )
	{ mTryAlloc( ret, char [sz] ); }
    return ret;
}
