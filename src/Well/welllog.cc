/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2003
-*/


#include "welllog.h"
#include "welllogset.h"
#include "iopar.h"
#include "idxable.h"
#include "keystrs.h"
#include "mnemonics.h"
#include "unitofmeasure.h"

const char* Well::Log::sKeyMnemLbl()	{ return "Mnemonic"; }
const char* Well::Log::sKeyUnitLbl()	{ return "Unit of Measure"; }
const char* Well::Log::sKeyHdrInfo()	{ return "Header info"; }
const char* Well::Log::sKeyStorage()	{ return "Storage type"; }
const char* Well::Log::sKeyDahRange()	{ return "Dah range"; }
const char* Well::Log::sKeyLogRange()	{ return "Log range"; }

#define mUnitOfMeasure() UnitOfMeasure::getGuessed(unitmeaslbl_);


// ---- Well::LogSet

mDefineInstanceCreatedNotifierAccess(Well::LogSet);


Well::LogSet::LogSet()
    : curlogidnr_(0)
{
    recalcDahIntv();
    mTriggerInstanceCreatedNotifier();
}


Well::LogSet::LogSet( const LogSet& oth )
    : NamedMonitorable(oth)
    , curlogidnr_(0)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


Well::LogSet::~LogSet()
{
    setEmpty();
    sendDelNotif();
}


mImplMonitorableAssignment( Well::LogSet, NamedMonitorable )


void Well::LogSet::copyClassData( const LogSet& oth )
{
    for ( int idx=0; idx<logs_.size(); idx++ )
	logs_[idx]->unRef();
    logs_.erase();
    for ( int idx=0; idx<oth.logs_.size(); idx++ )
    {
	Log* newlog = new Log( *oth.logs_[idx] );
	logs_ += newlog;
	newlog->ref();
    }

    logids_ = oth.logids_;
    dahintv_ = oth.dahintv_;
    curlogidnr_ = oth.curlogidnr_;
}


Monitorable::ChangeType Well::LogSet::compareClassData(
					const LogSet& oth ) const
{
    if ( logs_.size() != oth.logs_.size() )
	return cEntireObjectChange();

    for ( int idx=0; idx<logs_.size(); idx++ )
	if ( *logs_[idx] != *oth.logs_[idx] )
	    return cEntireObjectChange();

    return cNoChange();
}


Well::LogSet::idx_type Well::LogSet::gtIdx( LogID id ) const
{
    if ( id.isInvalid() )
	return -1;

    const int sz = logs_.size();
    if ( id.getI() < sz && logids_[id.getI()] == id )
	return id.getI();
    for ( idx_type idx=0; idx<logids_.size(); idx++ )
	if ( logids_[idx] == id )
	    return idx;

    return -1;
}


Well::Log* Well::LogSet::gtLog( LogID id ) const
{
    const idx_type idx = gtIdx( id );
    return idx < 0 ? 0 : const_cast<Log*>( logs_[idx] );
}


Well::LogSet::LogID Well::LogSet::gtID( const Log* log ) const
{
    for ( idx_type idx=0; idx<logs_.size(); idx++ )
    {
	if ( logs_[idx] == log )
	    return logids_[idx];
    }
    return LogID::getInvalid();
}


Well::LogSet::idx_type Well::LogSet::gtIdxByName( const char* nm ) const
{
    for ( idx_type idx=0; idx<logs_.size(); idx++ )
    {
	const Log& l = *logs_[idx];
	if ( l.hasName(nm) )
	    return idx;
    }
    return -1;
}


Well::Log* Well::LogSet::gtLogByName( const char* nm ) const
{
    const idx_type idx = gtIdxByName( nm );
    return idx < 0 ? 0 : const_cast<Log*>( logs_[idx] );
}


Well::Log* Well::LogSet::gtLogByIdx( idx_type idx ) const
{
    return logs_.validIdx(idx) ? const_cast<Log*>( logs_[idx] ) : 0;
}


void Well::LogSet::updateDahIntv( const Well::Log& wl )
{
    if ( wl.isEmpty() )
	return;

    const Interval<Log::ZType> dahrg( wl.dahRange() );
    if ( mIsUdf(dahintv_.start) )
	dahintv_ = dahrg;
    else
	dahintv_.include( dahrg, false );
}


void Well::LogSet::recalcDahIntv()
{
    dahintv_.start = mSetUdf(dahintv_.stop);
    for ( idx_type idx=0; idx<logs_.size(); idx++ )
	updateDahIntv( *logs_[idx] );
}


Well::Log* Well::LogSet::doRemove( idx_type idx )
{
    Log* log = logs_.removeSingle( idx );
    logids_.removeSingle( idx );
    recalcDahIntv();
    return log;
}


void Well::LogSet::doSetEmpty()
{
    deepUnRef( logs_ );
    recalcDahIntv();
}


Well::LogSet::LogRefMan Well::LogSet::getLog( LogID id )
{
    mLock4Read();
    return gtLog( id );
}


Well::LogSet::CLogRefMan Well::LogSet::getLog( LogID id ) const
{
    mLock4Read();
    return gtLog( id );
}


Well::LogSet::LogRefMan Well::LogSet::getLogByName( const char* nm )
{
    mLock4Read();
    return gtLogByName( nm );
}


Well::LogSet::CLogRefMan Well::LogSet::getLogByName( const char* nm ) const
{
    mLock4Read();
    return gtLogByName( nm );
}


Well::LogSet::LogRefMan Well::LogSet::getLogByIdx( idx_type idx )
{
    mLock4Read();
    return gtLogByIdx( idx );
}


Well::LogSet::CLogRefMan Well::LogSet::getLogByIdx( idx_type idx ) const
{
    mLock4Read();
    return gtLogByIdx( idx );
}


Well::LogSet::LogRefMan Well::LogSet::firstLog()
{
    mLock4Read();
    return logs_.isEmpty() ? 0 : logs_[0];
}


Well::LogSet::CLogRefMan Well::LogSet::firstLog() const
{
    mLock4Read();
    return logs_.isEmpty() ? 0 : logs_[0];
}


Well::LogSet::size_type Well::LogSet::size() const
{
    mLock4Read();
    return logs_.size();
}


Well::LogSet::LogID Well::LogSet::getID( idx_type idx ) const
{
    mLock4Read();
    return logids_.validIdx( idx ) ? logids_[idx] : LogID::getInvalid();
}


Well::LogSet::idx_type Well::LogSet::indexOf( LogID id ) const
{
    mLock4Read();
    return gtIdx( id );
}


Well::LogSet::idx_type Well::LogSet::indexOf( const char* nm ) const
{
    mLock4Read();
    return gtIdxByName( nm );
}


bool Well::LogSet::validIdx( idx_type idx ) const
{
    mLock4Read();
    return logs_.validIdx( idx );
}


void Well::LogSet::setEmpty()
{
    if ( isEmpty() )
	return;

    mLock4Write();
    doSetEmpty();
    mSendEntireObjChgNotif();
}


Well::LogSet::LogID Well::LogSet::add( Well::Log* wl )
{
    LogID logid = LogID::getInvalid();
    if ( !wl )
	return logid;
    wl->ref();

    mLock4Write();
    const Log* existlog = gtLogByName( wl->name() );
    int existidx = existlog ? logs_.indexOf(existlog) : -1;

    logid = LogID::get( curlogidnr_++ );
    logids_ += logid;
    logs_ += wl;

    if ( existidx < 0 )
	updateDahIntv( *wl );
    else
    {
	const LogID rmid = logids_[existidx];
	Log* torem = gtLogByIdx( existidx );
	mSendChgNotif( cLogRemove(), rmid.getI() );
	mReLock();
	existidx = logs_.indexOf( existlog );
	if ( existidx >= 0 )
	{
	    doRemove( existidx );
	    torem->unRef();
	    recalcDahIntv();
	}
    }

    mSendChgNotif( cLogAdd(), logid.getI() );
    return logid;
}


Well::LogSet::LogRefMan Well::LogSet::remove( LogID id )
{
    mLock4Write();
    idx_type idx = gtIdx( id );
    if ( idx < 0 )
	return 0;

    mSendChgNotif( cLogRemove(), id.getI() );
    mReLock();
    idx = gtIdx( id );
    if ( idx < 0 )
	return 0;

    LogRefMan refman = doRemove( idx );
    refman.setNoDelete( true );
    return refman;
}


Well::LogSet::LogRefMan Well::LogSet::removeByName( const char* nm )
{
    mLock4Write();
    idx_type idx = gtIdxByName( nm );
    if ( idx < 0 )
	return 0;

    LogID logid = logids_[idx];
    mSendChgNotif( cLogRemove(), logid.getI() );
    mReLock();
    idx = gtIdx( logid );
    if ( idx < 0 )
	return 0;

    return doRemove( idx );
}


bool Well::LogSet::isPresent( const char* nm ) const
{
    mLock4Read();
    for ( idx_type idx=0; idx<logs_.size(); idx++ )
	if ( logs_[idx]->hasName(nm) )
	    return true;
    return false;
}


void Well::LogSet::getNames( BufferStringSet& nms ) const
{
    mLock4Read();
    for ( idx_type idx=0; idx<logs_.size(); idx++ )
	nms.add( logs_[idx]->name() );
}


void Well::LogSet::getInfo( ObjectSet<IOPar>& iops ) const
{
    mLock4Read();
    for ( idx_type idx=0; idx<logs_.size(); idx++ )
    {
	IOPar* iop = new IOPar( logs_[idx]->pars() );
	iop->set( sKey::Unit(), logs_[idx]->unitMeasLabel() );
	iops += iop;
    }
}


void Well::LogSet::removeTopBottomUdfs()
{
    mLock4Read();
    for ( int idx=0; idx<logs_.size(); idx++ )
	logs_[idx]->removeTopBottomUdfs();
}


TypeSet<Well::LogSet::LogID> Well::LogSet::getSuitable(
	PropertyRef::StdType ptype,
	const PropertyRef* altpr, BoolTypeSet* arealt ) const
{
    TypeSet<LogID> ret;
    if ( arealt )
	arealt->setEmpty();

    mLock4Read();
    for ( int idx=0; idx<logs_.size(); idx++ )
    {
	const Well::Log& wl = *logs_[idx];
	const UnitOfMeasure* loguom = wl.unitOfMeasure();
	bool isalt = false;
	bool isok = !loguom || ptype == PropertyRef::Other
	         || loguom->propType() == ptype;
	if ( !isok && altpr )
	    isok = isalt = loguom->propType() == altpr->stdType();
	if ( isok )
	{
	    ret += logids_[idx];
	    if ( arealt )
		*arealt += isalt;
	}
    }

    return ret;
}


bool Well::LogSet::swap( idx_type idx1, idx_type idx2 )
{
    mLock4Write();
    if ( !logs_.validIdx(idx1) || !logs_.validIdx(idx2) )
	return false;

    logs_.swap( idx1, idx2 );
    logids_.swap( idx1, idx2 );
    mSendChgNotif( cOrderChange(), logids_[idx1].getI() );
    return true;
}


// Well::LogSetIter

Well::LogSetIter::LogSetIter( const LogSet& ls, bool atend )
    : MonitorableIter4Read<LogSet::idx_type>(ls,
	    atend?ls.size()-1:0,atend?0:ls.size()-1)
{
}


Well::LogSetIter::LogSetIter( const LogSetIter& oth )
    : MonitorableIter4Read<LogSet::idx_type>(oth)
{
}


const Well::LogSet& Well::LogSetIter::logSet() const
{
    return static_cast<const LogSet&>( monitored() );
}


Well::LogSet::LogID Well::LogSetIter::ID() const
{
    return isValid() ? logSet().logids_[ curidx_ ] : LogID::getInvalid();
}


static ConstRefMan<Well::Log> emptylog = new Well::Log;

const Well::Log& Well::LogSetIter::log() const
{
    return isValid() ? *logSet().logs_[curidx_] : *emptylog;
}


// ---- Well::Log

static const float valsarecodeeps = 1e-4f;
mDefineInstanceCreatedNotifierAccess(Well::Log);


Well::Log::Log( const char* nm )
    : DahObj(nm)
{
    redoValStats();
    mTriggerInstanceCreatedNotifier();
}


Well::Log::Log( const Log& oth )
    : DahObj(oth)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


Well::Log::~Log()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Well::Log, Well::DahObj )


void Well::Log::copyClassData( const Log& oth )
{
    vals_ = oth.vals_;
    unitmeaslbl_ = oth.unitmeaslbl_;
    mnemlbl_ = oth.mnemlbl_;
    pars_ = oth.pars_;
    valrg_ = oth.valrg_;
    valsarecodes_ = oth.valsarecodes_;
}


Monitorable::ChangeType Well::Log::compareClassData( const Log& oth ) const
{
    if ( vals_ != oth.vals_ )
	return cEntireObjectChange();

    mDeliverSingCondMonitorableCompare(
	unitmeaslbl_ == oth.unitmeaslbl_ && pars_ == oth.pars_,
	cParsChange() );
}


void Well::Log::getValues( ValueSetType& vals ) const
{
    mLock4Read();
    vals = vals_;
}


void Well::Log::getData( ZSetType& dahs, ValueSetType& vals ) const
{
    mLock4Read();
    dahs = dahs_;
    vals = vals_;
}


void Well::Log::setValues( const ValueSetType& vals )
{
    mLock4Write();
    if ( vals.size() != dahs_.size() )
	{ pErrMsg("size not OK"); return; }
    vals_ = vals;
    redoValStats();
    mSendEntireObjChgNotif();
}


void Well::Log::setData( const ZSetType& zs, const ValueSetType& vals )
{
    mLock4Write();
    if ( doSetData( zs, vals, vals_ ) )
    {
	ensureAscZ();
	redoValStats();
	mSendEntireObjChgNotif();
    }
}


void Well::Log::redoValStats()
{
    valsarecodes_ = false;
    valrg_.start = valrg_.stop = mUdf(ValueType);

    const int sz = vals_.size();
    if ( sz > 0 )
    {
	valsarecodes_ = true;
	valrg_.start = valrg_.stop = vals_[0];
	for ( idx_type idx=0; idx<sz; idx++ )
	    updValStats( vals_[idx] );
    }
}


void Well::Log::setValue( PointID id, ValueType val )
{
    mLock4Write();
    const idx_type idx = gtIdx( id );
    if ( idx < 0 )
	return;

    stVal( idx, val );
    mSendChgNotif( cValueChange(), id.getI() );
}



void Well::Log::setValue( idx_type idx, ValueType val )
{
    mLock4Read();
    if ( !vals_.validIdx(idx) )
	return;

    if ( !mLock2Write() && !vals_.validIdx(idx) )
	return;

    stVal( idx, val );
    mSendChgNotif( cValueChange(), ptids_[idx].getI() );
}


void Well::Log::addValue( ZType dh, ValueType val )
{
    setValueAt( dh, val );
    updValStats( val );
}


const char* Well::Log::mnemLabel() const
{
    if ( mnemonic() )
	return mnemonic()->name();

    return mnemlbl_;
}


const Mnemonic* Well::Log::mnemonic() const
{
    if (!mnemlbl_.isEmpty())
	return eMNC().find( mnemlbl_ );
    else
    {
	if ( unitOfMeasure()  )
	    return eMNC().getGuessed( unitOfMeasure() );
    }

    return nullptr;
}


void Well::Log::setMnemLabel( const char* mnem )
{
    mnemlbl_ = mnem;
}


const UnitOfMeasure* Well::Log::unitOfMeasure() const
{
    mLock4Read();
    return mUnitOfMeasure();
}


void Well::Log::applyUnit( const UnitOfMeasure* touom )
{
    if ( !touom || isEmpty() )
	return;

    mLock4Write();
    const size_type sz = vals_.size();
    for ( idx_type idx=0; idx<sz; idx++ )
	convValue( vals_[idx], 0, touom );
    unitmeaslbl_ = touom->symbol();
    mSendEntireObjChgNotif();
}


void Well::Log::convertTo( const UnitOfMeasure* touom )
{
    mLock4Write();
    const size_type sz = vals_.size();
    if ( sz < 1 )
	return;

    const UnitOfMeasure* curuom = mUnitOfMeasure();
    for ( idx_type idx=0; idx<sz; idx++ )
	convValue( vals_[idx], curuom, touom );

    if ( touom )
	unitmeaslbl_ = touom->symbol();
    else
    {
	PropertyRef::StdType tp = curuom->propType();
	const UnitOfMeasure* siuom = UoMR().getInternalFor( tp );
	unitmeaslbl_ = siuom ? siuom->symbol() : "";
    }
    mSendEntireObjChgNotif();
}


PropertyRef::StdType Well::Log::propType() const
{
    const UnitOfMeasure* uom = unitOfMeasure();
    return uom ? uom->propType() : (  valsarecodes_ ?
	    PropertyRef::Class : PropertyRef::Other );
}


void Well::Log::updValStats( ValueType val )
{
    if ( mIsUdf(val) )
	return;

    if ( val < valrg_.start )
	valrg_.start = val;
    if ( val > valrg_.stop )
	valrg_.stop = val;

    const int intval = mRounded( int, val );
    const bool iscode = intval > -99 && intval < 100000
		     && mIsEqual(val,intval,valsarecodeeps);
    if ( valsarecodes_ && !iscode )
	valsarecodes_ = false;
}


void Well::Log::ensureAscZ()
{
    size_type sz = dahs_.size();
    if ( sz < 2 )
	return;
    if ( dahs_[0] < dahs_[sz-1] )
	return;

    const size_type hsz = sz / 2;
    for ( idx_type idx=0; idx<hsz; idx++ )
    {
	std::swap( dahs_[idx], dahs_[sz-idx-1] );
	std::swap( vals_[idx], vals_[sz-idx-1] );
    }

}


void Well::Log::removeTopBottomUdfs()
{
    mLock4Read();

    const size_type sz = size();
    if ( sz < 1 )
	return;

    Interval<idx_type> defrg( 0, sz-1 );
    for ( idx_type idx=0; idx<sz; idx++ )
    {
	if ( !mIsUdf(vals_[idx]) )
	    break;
	defrg.start++;
    }

    for ( idx_type idx=sz-1; idx>=defrg.start; idx-- )
    {
	if ( !mIsUdf(vals_[idx]) )
	    break;
	defrg.stop--;
    }

    if ( defrg.start == 0 && defrg.stop == sz-1 )
	return;

    ZSetType newdahs; ValueSetType newvals;
    for ( idx_type idx=defrg.start; idx<=defrg.stop; idx++ )
	{ newdahs += dahs_[idx]; newvals += vals_[idx]; }

    mUnlockAllAccess();
    setData( newdahs, newvals );
}


bool Well::Log::doSet( idx_type idx, ValueType val )
{
    if ( val == vals_[idx] ) // cannot assume any epsilon
	return false;

    vals_[idx] = val;
    updValStats( val );
    return true;
}


Well::Log::PointID Well::Log::doInsAtDah( ZType dh, ValueType val )
{
    PointID id = doIns(dh,val,vals_,false);
    if ( id.isValid() )
	updValStats( val );
    return id;
}


void Well::Log::stVal( idx_type idx, ValueType val )
{
    vals_[idx] = val;
    updValStats( val );
}


Well::LogIter::LogIter( const Log& trck, bool atend )
    : DahObjIter(trck,atend)
{
}


Well::LogIter::LogIter( const LogIter& oth )
    : DahObjIter(oth)
{
}


const Well::Log& Well::LogIter::log() const
{
    return static_cast<const Log&>( monitored() );
}


