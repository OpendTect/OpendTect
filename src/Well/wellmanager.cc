/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2001 / Mar 2016
-*/


#include "wellmanager.h"
#include "wellreader.h"
#include "wellwriter.h"
#include "wellinfo.h"
#include "welltrack.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "welldisp.h"
#include "welllogset.h"
#include "welltransl.h"
#include "dbman.h"
#include "ioobj.h"
#include "ioobjctxt.h"
#include "dbdir.h"
#include "survinfo.h"


#define mToWD(cnsttyp,reftyp,var) static_cast<cnsttyp Well::Data reftyp>(var)


mDefineSaveableManagerInstance(Well::Manager);



Well::LoadReqs::LoadReqs( bool addall )
{
    if ( addall )
	setToAll();
}


Well::LoadReqs::LoadReqs( SubObjType typ )
{
    add( typ );
}


Well::LoadReqs::LoadReqs( SubObjType typ1, SubObjType typ2 )
{
    add( typ1 ).add( typ2 );
}


Well::LoadReqs::LoadReqs( SubObjType typ1, SubObjType typ2, SubObjType typ3 )
{
    add( typ1 ).add( typ2 ).add( typ3 );
}


Well::LoadReqs& Well::LoadReqs::add( SubObjType typ )
{
    if ( typ != D2T || SI().zIsTime() )
	reqs_[typ] = 1;
    if ( typ == Trck )
	reqs_[Inf] = 1;
    return *this;
}


Well::LoadReqs Well::LoadReqs::All()
{
    LoadReqs ret( false );
    ret.reqs_.set();
    if ( !SI().zIsTime() )
	ret.reqs_[D2T] = 0;
    return ret;
}


Well::LoadReqs& Well::LoadReqs::forceAddD2T()
{
    reqs_[D2T] = 1;
    return *this;
}


void Well::LoadReqs::include( const LoadReqs& oth )
{
    for ( int idx=0; idx<mWellNrSubObjTypes; idx++ )
	if ( oth.reqs_[idx] )
	    reqs_[ idx ] = 1;
}


bool Well::LoadReqs::includes( const LoadReqs& oth ) const
{
    for ( int idx=0; idx<mWellNrSubObjTypes; idx++ )
	if ( oth.reqs_[idx] && !reqs_[idx] )
	    return false;
    return true;
}


Well::Manager::Manager()
    : SaveableManager(mIOObjContext(Well),true)
{
    mTriggerInstanceCreatedNotifier();
}


Well::Manager::~Manager()
{
    sendDelNotif();
}


template <class RefManType>
RefManType Well::Manager::doFetch( const ObjID& id, const LoadReqs& lr,
				    uiRetVal& uirv ) const
{
    mLock4Read();
    const IdxType idxof = gtIdx( id );
    Data* wd = idxof < 0 ? 0 : const_cast<Data*>( gtData(idxof) );
    if ( wd  && loadstates_[idxof].includes(lr) )
	return RefManType( wd );		// already loaded

    LoadReqs lreq( lr );
    if ( idxof >=0 )
    {
	mLock2Write();
	lreq.include( loadstates_[idxof] );
	if ( !readReqData(id,*wd,lreq,uirv) )
	    return RefManType( 0 );
    }
    else
    {
	wd = new Data;
	if ( !readReqData(id,*wd,lreq,uirv) )
	    return RefManType( 0 );

	curloadstate_.getObject() = lreq;
	add( *wd, id, mAccessLocker(), 0, true );
    }

    return RefManType( wd );
}


bool Well::Manager::readReqData( ObjID id, Data& wd, const LoadReqs& lreq,
				  uiRetVal& uirv ) const
{
    Reader rdr( id, wd );
#   define mRetIfFail(typ,oper) \
    if ( lreq.includes(typ) && !oper ) \
	{ uirv = rdr.errMsg(); return false; }
    mRetIfFail( Inf, rdr.getInfo() )
    mRetIfFail( Trck, rdr.getTrack() )
    mRetIfFail( D2T, rdr.getD2T() )

#   define mJustTry(typ,oper) \
    if ( lreq.includes(typ) ) oper;
    mJustTry( Mrkrs, rdr.getMarkers() )
    mJustTry( Logs, rdr.getLogs() )
    mJustTry( CSMdl, rdr.getCSMdl() )
    if ( lreq.includes(DispProps2D) || lreq.includes(DispProps3D) )
	rdr.getDispProps();
    return true;
}


void Well::Manager::setAuxOnAdd()
{
    loadstates_ += curloadstate_.getObject();
}


ConstRefMan<Well::Data> Well::Manager::fetch( const ObjID& id,
						LoadReqs lr ) const
{
    uiRetVal uirv;
    return doFetch< ConstRefMan<Data> >( id, lr, uirv );
}


ConstRefMan<Well::Data> Well::Manager::fetch( const ObjID& id,
				    LoadReqs lr, uiRetVal& uirv ) const
{
    return doFetch< ConstRefMan<Data> >( id, lr, uirv );
}


RefMan<Well::Data> Well::Manager::fetchForEdit( const ObjID& id,
						LoadReqs lr ) const
{
    uiRetVal uirv;
    return doFetch< RefMan<Data> >( id, lr, uirv );
}


RefMan<Well::Data> Well::Manager::fetchForEdit( const ObjID& id,
				    LoadReqs lr, uiRetVal& uirv ) const
{
    return doFetch< RefMan<Data> >( id, lr, uirv );
}


Well::Manager::ObjID Well::Manager::getID( const Data& wd ) const
{
    return SaveableManager::getID( wd );
}


Well::Manager::ObjID Well::Manager::getIDByUWI( const char* uwi ) const
{
    if ( !uwi || !*uwi )
	return ObjID::getInvalid();

    mLock4Read();

    for ( IdxType idx=0; idx<savers_.size(); idx++ )
    {
	const Saveable& saver = *savers_[idx];
	const SharedObject* obj = saver.object();
	mDynamicCastGet( const Data*, wd, obj );
	if ( wd && wd->info().UWI() == uwi )
	    return saver.key();
    }

    mUnlockAllAccess();

    PtrMan<IOObj> ioobj = getIOObjByUWI( uwi );
    if ( ioobj )
	return ioobj->key();

    return ObjID::getInvalid();
}


IOObj* Well::Manager::getIOObjByUWI( const char* uwi ) const
{
    if ( !uwi || !*uwi )
	return 0;

    const DBDirEntryList del( mIOObjContext(Well) );
    RefMan<Well::Data> data = new Well::Data;
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj& ioobj = del.ioobj( idx );
	Reader wr( ioobj, *data );
	if ( wr.getInfo() && data->info().UWI() == uwi )
	    return ioobj.clone();
    }

    return 0;
}


Coord Well::Manager::getMapLocation( const ObjID& id ) const
{
    mLock4Read();
    const IdxType idxof = gtIdx( id );
    if ( idxof >= 0 )
	return gtData(idxof)->info().surfaceCoord();

    mUnlockAllAccess();

    PtrMan<IOObj> ioobj = getIOObj( id );
    if ( !ioobj )
	return Coord::udf();

    RefMan<Well::Data> data = new Well::Data; Coord maploc;
    Well::Reader rdr( *ioobj, *data );
    return rdr.getMapLocation(maploc) ? maploc : Coord::udf();
}


uiRetVal Well::Manager::store( const Data& wd,
				  const IOPar* ioobjpars ) const
{
    return SaveableManager::store( wd, ioobjpars );
}


uiRetVal Well::Manager::store( const Data& wd, const ObjID& id,
			      const IOPar* ioobjpars ) const
{
    return SaveableManager::store( wd, id, ioobjpars );
}


uiRetVal Well::Manager::save( const ObjID& id ) const
{
    return SaveableManager::save( id );
}


uiRetVal Well::Manager::save( const Data& wd ) const
{
    return SaveableManager::save( wd );
}


bool Well::Manager::needsSave( const ObjID& id ) const
{
    return SaveableManager::needsSave( id );
}


bool Well::Manager::needsSave( const Data& wd ) const
{
    return SaveableManager::needsSave( wd );
}


void Well::Manager::getLogNames( const ObjID& id, BufferStringSet& nms ) const
{
    mLock4Read();
    const int idx = gtIdx( id );
    if ( idx >= 0 && loadstates_[idx].includes( Logs ) )
	gtData(idx)->logs().getNames( nms );
    else
    {
	mUnlockAllAccess();
	RefMan<Data> wd = new Well::Data;
	Reader rdr( id, *wd );
	rdr.getLogInfo( nms );
    }
}


void Well::Manager::getAllMarkerNames( BufferStringSet& nms ) const
{
    const DBDirEntryList del( mIOObjContext(Well) );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	RefMan<Data> wd = new Well::Data;
	Reader rdr( del.ioobj(idx), *wd );
	BufferStringSet newnms;
	if ( rdr.getMarkers() )
	    wd->markers().getNames( newnms );
	nms.add( newnms, false );
    }
}


ConstRefMan<Well::Log> Well::Manager::getLog( const ObjID& id,
						const char* lognm ) const
{
    mLock4Read();
    const IdxType idx = gtIdx( id );
    if ( idx >=0 && loadstates_[idx].includes(Logs) )
	return gtData(idx)->logs().getLogByName( lognm );
    else
    {
	RefMan<Data> wd = new Well::Data;
	Reader rdr( id, *wd );
	if ( !rdr.getLog(lognm) )
	    return 0;

	return wd->logs().getLogByName( lognm );
    }
}


ConstRefMan<Well::Data> Well::Manager::get( IdxType idx ) const
{
    const SharedObject* shobj = gtObj( idx );
    return ConstRefMan<Data>( mToWD(const,*,shobj) );
}


RefMan<Well::Data> Well::Manager::getForEdit( IdxType idx )
{
    SharedObject* shobj = gtObj( idx );
    return RefMan<Data>( mToWD(,*,shobj) );
}


Well::Data* Well::Manager::gtData( IdxType idx ) const
{
    SharedObject* obj = const_cast<SharedObject*>( savers_[idx]->object() );
    return mToWD(,*,obj);
}


Saveable* Well::Manager::getSaver( const SharedObject& obj ) const
{
    return new Saver( mToWD(const,&,obj) );
}


mDefineInstanceCreatedNotifierAccess(Well::Saver)


Well::Saver::Saver( const Data& wd )
    : Saveable(wd)
{
    for ( int idx=0; idx<nrSubObjTypes(); idx++ )
	lastsavedsubobjdirtycounts_ += DirtyCounter( 0 );
    Saver::setJustSaved();
    mTriggerInstanceCreatedNotifier();
}


Well::Saver::Saver( const Well::Saver& oth )
    : Saveable(oth)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


Well::Saver::~Saver()
{
    sendDelNotif();
}


mImplMonitorableAssignment(Well::Saver,Saveable)

void Well::Saver::copyClassData( const Well::Saver& oth )
{
    lastsavedsubobjdirtycounts_ = oth.lastsavedsubobjdirtycounts_;
}


ConstRefMan<Well::Data> Well::Saver::wellData() const
{
    return ConstRefMan<Data>( static_cast<const Data*>( object() ) );
}


void Well::Saver::setWellData( const Data& wd )
{
    setObject( wd );
}



uiRetVal Well::Saver::doStore( const IOObj& ioobj ) const
{
    uiRetVal uirv;
    ConstRefMan<Data> wd = wellData();
    if ( !wd )
	return uirv;

    Writer wrr( ioobj, *wd );
    const StoreReqs sreqs = getStoreReqs( *wd );

#   define mReqsIncl( typ ) sreqs.includes( typ )
#   define mCondWrite( cond, oper ) \
    if ( cond ) \
    { \
	if ( !wrr.oper ) \
	    uirv.add( wrr.errMsg() ); \
    }

    mCondWrite( mReqsIncl(Inf) || mReqsIncl(Trck), putInfoAndTrack() )
    mCondWrite( mReqsIncl(D2T), putD2T() )
    mCondWrite( mReqsIncl(CSMdl), putCSMdl() )
    mCondWrite( mReqsIncl(Mrkrs), putMarkers() )
    mCondWrite( mReqsIncl(Logs), putLogs() )
    mCondWrite( mReqsIncl(DispProps2D) || mReqsIncl(DispProps3D),
		putDispProps() )

    if ( isSave(ioobj) )
	updateLastSavedSubObjDirtyCounts( *wd );
    return uirv;
}


void Well::Saver::updateLastSavedSubObjDirtyCounts( const Well::Data& wd ) const
{
#   define mSetFor( typ, subobj ) \
    lastsavedsubobjdirtycounts_[typ] = wd.subobj.dirtyCount();
    mSetFor( Inf, info() );
    mSetFor( Trck, track() );
    mSetFor( D2T, d2TModel() );
    mSetFor( CSMdl, checkShotModel() );
    mSetFor( Mrkrs, markers() );
    mSetFor( Logs, logs() );
    mSetFor( DispProps2D, displayProperties(true) );
    mSetFor( DispProps3D, displayProperties(false) );
}


Well::Saver::StoreReqs Well::Saver::getStoreReqs( const Well::Data& wd ) const
{
    StoreReqs reqs;
#   define mUnsetIfUnchanged( typ, subobj ) \
    if ( lastsavedsubobjdirtycounts_[typ] == wd.subobj.dirtyCount() ) \
	reqs.remove( typ )
    mUnsetIfUnchanged( Inf, info() );
    mUnsetIfUnchanged( Trck, track() );
    mUnsetIfUnchanged( D2T, d2TModel() );
    mUnsetIfUnchanged( CSMdl, checkShotModel() );
    mUnsetIfUnchanged( Mrkrs, markers() );
    mUnsetIfUnchanged( Logs, logs() );
    mUnsetIfUnchanged( DispProps2D, displayProperties(true) );
    mUnsetIfUnchanged( DispProps3D, displayProperties(false) );

    return reqs;
}
