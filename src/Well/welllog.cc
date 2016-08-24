/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/


#include "welllog.h"
#include "welllogset.h"
#include "iopar.h"
#include "idxable.h"
#include "unitofmeasure.h"

const char* Well::Log::sKeyUnitLbl()	{ return "Unit of Measure"; }
const char* Well::Log::sKeyHdrInfo()	{ return "Header info"; }
const char* Well::Log::sKeyStorage()	{ return "Storage type"; }

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
    copyAll( oth );
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
    deepCopy( logs_, oth.logs_ );
    logids_ = oth.logids_;
    dahintv_ = oth.dahintv_;
    curlogidnr_ = oth.curlogidnr_;
}


Well::LogSet::IdxType Well::LogSet::gtIdx( LogID id ) const
{
    if ( id.isInvalid() )
	return -1;

    const int sz = logs_.size();
    if ( id.getI() < sz && logids_[id.getI()] == id )
	return id.getI();
    for ( IdxType idx=0; idx<logids_.size(); idx++ )
	if ( logids_[idx] == id )
	    return idx;

    return -1;
}


Well::Log* Well::LogSet::gtLog( LogID id ) const
{
    const IdxType idx = gtIdx( id );
    return idx < 0 ? 0 : const_cast<Log*>( logs_[idx] );
}


Well::LogSet::LogID Well::LogSet::gtID( const Log* log ) const
{
    for ( IdxType idx=0; idx<logs_.size(); idx++ )
    {
	if ( logs_[idx] == log )
	    return logids_[idx];
    }
    return LogID::getInvalid();
}


Well::LogSet::IdxType Well::LogSet::gtIdxByName( const char* nm ) const
{
    for ( IdxType idx=0; idx<logs_.size(); idx++ )
    {
	const Log& l = *logs_[idx];
	if ( l.name() == nm )
	    return idx;
    }
    return -1;
}


Well::Log* Well::LogSet::gtLogByName( const char* nm ) const
{
    const IdxType idx = gtIdxByName( nm );
    return idx < 0 ? 0 : const_cast<Log*>( logs_[idx] );
}


Well::Log* Well::LogSet::gtLogByIdx( IdxType idx ) const
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
    for ( IdxType idx=0; idx<logs_.size(); idx++ )
	updateDahIntv( *logs_[idx] );
}


Well::Log* Well::LogSet::doRemove( IdxType idx )
{
    Log* log = logs_.removeSingle( idx );
    logids_.removeSingle( idx );
    recalcDahIntv();
    return log;
}


void Well::LogSet::doSetEmpty()
{
    deepErase( logs_ );
    recalcDahIntv();
}


Well::Log* Well::LogSet::getLog( LogID id )
{
    mLock4Read();
    return gtLog( id );
}


const Well::Log* Well::LogSet::getLog( LogID id ) const
{
    mLock4Read();
    return gtLog( id );
}


Well::Log* Well::LogSet::getLogByName( const char* nm )
{
    mLock4Read();
    return gtLogByName( nm );
}


const Well::Log* Well::LogSet::getLogByName( const char* nm ) const
{
    mLock4Read();
    return gtLogByName( nm );
}


Well::Log* Well::LogSet::getLogByIdx( IdxType idx )
{
    mLock4Read();
    return gtLogByIdx( idx );
}


const Well::Log* Well::LogSet::getLogByIdx( IdxType idx ) const
{
    mLock4Read();
    return gtLogByIdx( idx );
}


const Well::Log* Well::LogSet::firstLog() const
{
    mLock4Read();
    return logs_.isEmpty() ? 0 : logs_[0];
}


Well::LogSet::size_type Well::LogSet::size() const
{
    mLock4Read();
    return logs_.size();
}


Well::LogSet::IdxType Well::LogSet::indexOf( LogID id ) const
{
    mLock4Read();
    return gtIdx( id );
}


Well::LogSet::IdxType Well::LogSet::indexOf( const char* nm ) const
{
    mLock4Read();
    return gtIdxByName( nm );
}


bool Well::LogSet::validIdx( IdxType idx ) const
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

    mLock4Write();
    const Log* existlog = gtLogByName( wl->name() );
    const int existidx = existlog ? logs_.indexOf(existlog) : -1;

    logid = LogID::get( curlogidnr_++ );
    logids_ += logid;
    logs_ += wl;

    if ( existidx < 0 )
	updateDahIntv( *wl );
    else
    {
	const LogID rmid = logids_[existidx];
	delete doRemove( existidx );
	recalcDahIntv();
	mSendChgNotif( cLogRemove(), rmid.getI() );
	mReLock();
    }

    mSendChgNotif( cLogAdd(), logid.getI() );
    return logid;
}


Well::Log* Well::LogSet::remove( LogID id )
{
    mLock4Write();
    IdxType idx = gtIdx( id );
    if ( idx < 0 )
	return 0;

    mSendChgNotif( cLogRemove(), id.getI() );
    mReLock();
    idx = gtIdx( id );
    if ( idx < 0 )
	return 0;

    return doRemove( idx );
}


Well::Log* Well::LogSet::removeByName( const char* nm )
{
    mLock4Write();
    IdxType idx = gtIdxByName( nm );
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
    for ( IdxType idx=0; idx<logs_.size(); idx++ )
	if ( logs_[idx]->name() == nm )
	    return true;
    return false;
}


void Well::LogSet::getNames( BufferStringSet& nms ) const
{
    mLock4Read();
    for ( IdxType idx=0; idx<logs_.size(); idx++ )
	nms.add( logs_[idx]->name() );
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


bool Well::LogSet::swap( IdxType idx1, IdxType idx2 )
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
    : MonitorableIter<LogSet::IdxType>(ls,atend?ls.size():-1)
{
}


Well::LogSetIter::LogSetIter( const LogSetIter& oth )
    : MonitorableIter<LogSet::IdxType>(oth)
{
}


const Well::LogSet& Well::LogSetIter::logSet() const
{
    return static_cast<const LogSet&>( monitored() );
}


Well::LogSetIter::size_type Well::LogSetIter::size() const
{
    return logSet().logs_.size();
}


Well::LogSet::LogID Well::LogSetIter::ID() const
{
    return logSet().logs_.validIdx(curidx_) ? logSet().logids_[ curidx_ ]
					    : LogID::getInvalid();
}


static const Well::Log emptylog;

const Well::Log& Well::LogSetIter::log() const
{
    return logSet().logs_.validIdx(curidx_) ? *logSet().logs_[curidx_] :emptylog;
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
    copyAll( oth );
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
    pars_ = oth.pars_;
    valrg_ = oth.valrg_;
    valsarecodes_ = oth.valsarecodes_;
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
	for ( IdxType idx=0; idx<sz; idx++ )
	    updValStats( vals_[idx] );
    }

}


void Well::Log::setValue( PointID id, ValueType val )
{
    mLock4Write();
    const IdxType idx = gtIdx( id );
    if ( idx < 0 )
	return;

    stVal( idx, val );
    mSendChgNotif( cValueChange(), id.getI() );
}



void Well::Log::setValue( IdxType idx, ValueType val )
{
    mLock4Read();
    if ( !vals_.validIdx(idx) )
	return;

    if ( !mLock2Write() && !vals_.validIdx(idx) )
	return;

    stVal( idx, val );
    mSendChgNotif( cValueChange(), ptids_[idx].getI() );
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
    for ( IdxType idx=0; idx<sz; idx++ )
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
    for ( IdxType idx=0; idx<sz; idx++ )
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
    return uom ? uom->propType() : PropertyRef::Other;
}


void Well::Log::updValStats( ValueType val )
{
    if ( mIsUdf(val) )
	return;

    if ( val < valrg_.start )
	valrg_.start = val;
    if ( val > valrg_.stop )
	valrg_.stop = val;

    const ValueType intval = mRounded( ValueType, val );
    const bool iscode = mIsEqual(val,intval,valsarecodeeps);
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
    for ( IdxType idx=0; idx<hsz; idx++ )
    {
	Swap( dahs_[idx], dahs_[sz-idx-1] );
	Swap( vals_[idx], vals_[sz-idx-1] );
    }

}


void Well::Log::removeTopBottomUdfs()
{
    mLock4Read();

    const size_type sz = size();
    if ( sz < 1 )
	return;

    Interval<IdxType> defrg( 0, sz-1 );
    for ( IdxType idx=0; idx<sz; idx++ )
    {
	if ( !mIsUdf(vals_[idx]) )
	    break;
	defrg.start++;
    }

    for ( IdxType idx=sz-1; idx>=defrg.start; idx-- )
    {
	if ( !mIsUdf(vals_[idx]) )
	    break;
	defrg.stop--;
    }

    if ( defrg.start == 0 && defrg.stop == sz-1 )
	return;

    ZSetType newdahs; ValueSetType newvals;
    for ( IdxType idx=defrg.start; idx<=defrg.stop; idx++ )
	{ newdahs += dahs_[idx]; newvals += vals_[idx]; }

    mUnlockAllAccess();
    setData( newdahs, newvals );
}


bool Well::Log::doSet( IdxType idx, ValueType val )
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


void Well::Log::stVal( IdxType idx, ValueType val )
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
