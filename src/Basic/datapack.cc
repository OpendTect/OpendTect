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
    , nrnull_(0)
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
	ConstRefMan<DataPack> pack = packs_[idx].get();
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
    return indexOf( dpid ) >= 0;
}


float DataPackMgr::nrKBytes() const
{
    float res = 0;
    for ( int idx=0; idx<packs_.size(); idx++ )
    {
	const DataPack* pack =(const DataPack*) refPtr(packs_[idx].get().ptr());
	if ( pack )
	{
	    res += pack->nrKBytes();
	    pack->unRefNoDelete();
	}
    }
    return res;
}


RefMan<DataPack> DataPackMgr::get( DataPack::ID dpid ) const
{
    RefMan<DataPack> res = 0;
    packslock_.readLock();
    const int idx = indexOf( dpid );
    if ( idx>=0 )
	res = packs_[idx].get();

    packslock_.readUnlock();

    return res;
}


WeakPtr<DataPack> DataPackMgr::observe( DataPack::ID dpid ) const
{
    WeakPtr<DataPack> res = 0;
    packslock_.readLock();
    const int idx = indexOf( dpid );
    if ( idx>=0 )
        res = packs_[idx];

    packslock_.readUnlock();

    return res;
}



void DataPackMgr::getPackIDs( TypeSet<DataPack::ID>& ids ) const
{
    packslock_.readLock();
    for ( int idx=0; idx<packs_.size(); idx++ )
    {
	const DataPack* pack =(const DataPack*) refPtr(packs_[idx].get().ptr());

	if ( pack )
	{
	    ids += pack->id();
	    pack->unRefNoDelete();
	}
    }

    packslock_.readUnlock();
}


void DataPackMgr::dumpInfo( od_ostream& strm ) const
{
    strm << "Manager.ID: " << id().getI() << od_newline;
    const od_int64 nrkb = mCast(od_int64,nrKBytes());
    strm << "Total memory: " << File::getFileSizeString(nrkb)
			     << od_newline;
    ascostream astrm( strm );
    astrm.newParagraph();
    for ( int idx=0; idx<packs_.size(); idx++ )
    {
	const DataPack* pack =(const DataPack*) refPtr(packs_[idx].get().ptr());
	if ( !pack )
	    continue;

	IOPar iop;
	pack->dumpInfo( iop );
	iop.putTo( astrm );
	pack->unRefNoDelete();
    }
}

#define mMaxNrNull 30


void DataPackMgr::doAdd( DataPack* dp )
{
    if ( !dp ) return;

    RefMan<DataPack> keeper = dp;
    keeper.setNoDelete( true );
    dp->setManager( this );

    packslock_.writeLock();

    //Do some cleanup while we are in writelock
    if ( nrnull_>mMaxNrNull )
    {
	for ( int idx=packs_.size()-1; idx>=0; idx-- )
	{
	    if ( !packs_[idx] )
		packs_.removeSingle( idx );
	}

	nrnull_ = 0;
    }

    packs_ += dp;

    packslock_.writeUnlock();

    mTrackDPMsg( BufferString("[DP]: add ",dp->id().getI(),
		 BufferString(" '",dp->name(),"'")) );

    newPack.trigger( dp );
}


DataPack* DataPackMgr::addAndObtain( DataPack* dp )
{
    if ( !dp ) return 0;

    dp->ref();
    dp->setManager( this );

    packslock_.writeLock();
    const int idx = packs_.indexOf( dp );
    if ( idx==-1 )
    {
	mTrackDPMsg( BufferString("[DP]: add+obtain ",dp->id().getI(),
		     BufferString(" '",dp->name(),"'")) );
	packs_ += dp;
    }
    else
    {
	mTrackDPMsg( BufferString("[DP]: add+obtain [existing!] ",
		    dp->id().getI(), BufferString(" nrusers=",dp->nrRefs())) );
    }
    packslock_.writeUnlock();

    if ( idx==-1 )
	newPack.trigger( dp );

    return dp;
}


bool DataPackMgr::ref( DataPack::ID dpid )
{
    bool res;
    packslock_.readLock();
    const int idx = indexOf( dpid );
    if ( packs_.validIdx(idx) )
    {
	RefMan<DataPack> pack = packs_[idx].get();
	packslock_.readUnlock();
	if ( pack )
	{
	    pack->ref();
	    mTrackDPMsg( BufferString("[DP]: ref ",pack->id().getI(),
			 BufferString(" nrusers=",pack->nrRefs())) );

	}
	res = true;
    }
    else
    {
	packslock_.readUnlock();
	res = false;
    }

    return res;
}


bool DataPackMgr::unRef( DataPack::ID dpid )
{
    bool res;
    packslock_.readLock();
    const int idx = indexOf( dpid );
    if ( packs_.validIdx(idx) )
    {
	RefMan<DataPack> pack = packs_[idx].get();
	packslock_.readUnlock();
	if ( pack )
	{
            pack->unRef();
            //We have reffed in the refman above
            //Hence the 'real' number is actual refs -1

	    mTrackDPMsg( BufferString("[DP]: unRef ",pack->id().getI(),
			 BufferString(" nrusers=",pack->nrRefs()-1)) );

	}

	res = true;
    }
    else
    {
	packslock_.readUnlock();
	res = false;
    }

    return res;
}


DataPack* DataPackMgr::doObtain( DataPack::ID dpid, bool obs ) const
{
    packslock_.readLock();
    const int idx = indexOf( dpid );

    DataPack* res = 0;
    if ( packs_.validIdx(idx) )
    {
	RefMan<DataPack> pack = packs_[idx].get();
	packslock_.readUnlock();
	res = pack.ptr();
	if ( !obs )
	{
	    res->ref();
            //Real number of refs is one higher, as we have a refman in the
            //function
	    mTrackDPMsg( BufferString("[DP]: obtain ",res->id().getI(),
			 BufferString(" nrusers=",res->nrRefs()-1)) );
	}
        else
        {
            pack.setNoDelete( true );
        }
    }
    else
    {
	packslock_.readUnlock();
    }

    return res;
}


int DataPackMgr::indexOf( DataPack::ID dpid ) const
{
    //Count how many null pointers we have and update if need be.
    int nrnullptr = 0;
    int res = -1;

    for ( int idx=0; idx<packs_.size(); idx++ )
    {
	const DataPack* pack =(const DataPack*) refPtr(packs_[idx].get().ptr());
	if ( pack )
	{
	    if ( pack->id() == dpid )
	    {
		res = idx;
		pack->unRefNoDelete();
		break;
	    }
	}
	else
	{
	    nrnullptr++;
	}

	unRefNoDeletePtr( pack );
    }

    //As we are in a locked section, we are sure no-one has set it to zero
    //in the meanwhile.

    nrnull_.setIfLarger( nrnullptr );

    return res;
}


void DataPackMgr::release( DataPack::ID dpid )
{
    packslock_.readLock();
    int idx = indexOf( dpid );
    if ( packs_.validIdx(idx) )
    {
	RefMan<DataPack> pack = packs_[idx].get();
	packslock_.readUnlock();
	pack.ptr()->unRef();

	if ( pack->nrRefs()>1 ) //1 is our own ref
	{
	    mTrackDPMsg( BufferString("[DP]: release ",pack->id().getI(),
                         BufferString(" nrusers=",pack->nrRefs()-1)) );
	    return;
	}
    }
    else
    {
	packslock_.readUnlock();
    }
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

mDefDPMDataPackFn(const char*,name)
mDefDPMDataPackFn(const char*,category)
mDefDPMDataPackFn(float,nrKBytes)

void DataPackMgr::dumpInfoFor( DataPack::ID dpid, IOPar& iop ) const
{
    const int idx = indexOf( dpid ); if ( idx < 0 ) return;
    const DataPack* pack = (const DataPack*) refPtr( packs_[idx].get().ptr() );
    if ( pack ) pack->dumpInfo( iop );
    unRefNoDeletePtr( pack );
}


void DataPack::dumpInfo( IOPar& iop ) const
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
