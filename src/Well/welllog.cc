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


// ---- Well::LogSet


void Well::LogSet::getNames( BufferStringSet& nms ) const
{
    for ( int idx=0; idx<logs_.size(); idx++ )
	nms.add( logs_[idx]->name() );
}


void Well::LogSet::add( Well::Log* wl )
{
    if ( !wl ) return;
    if ( getLog(wl->name()) ) return;

    logs_ += wl;
    updateDahIntv( *wl );
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


void Well::LogSet::updateDahIntvs()
{
    for ( int idx=0; idx<logs_.size(); idx++ )
	updateDahIntv( *logs_[idx] );
}


int Well::LogSet::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<logs_.size(); idx++ )
    {
	const Log& l = *logs_[idx];
	if ( l.name() == nm )
	    return idx;
    }
    return -1;
}


Well::Log* Well::LogSet::remove( int logidx )
{
    Log* log = logs_[logidx]; logs_ -= log;
    ObjectSet<Well::Log> tmp( logs_ );
    logs_.erase(); init();
    for ( int idx=0; idx<tmp.size(); idx++ )
	add( tmp[idx] );
    return log;
}


void Well::LogSet::setEmpty()
{
    deepErase( logs_ );
}


void Well::LogSet::removeTopBottomUdfs()
{
    for ( int idx=0; idx<logs_.size(); idx++ )
	logs_[idx]->removeTopBottomUdfs();
}


TypeSet<int> Well::LogSet::getSuitable( PropertyRef::StdType ptype,
	const PropertyRef* altpr, BoolTypeSet* arealt ) const
{
    TypeSet<int> ret;
    if ( arealt )
	arealt->setEmpty();

    for ( int idx=0; idx<logs_.size(); idx++ )
    {
	const char* loguomlbl = logs_[idx]->unitMeasLabel();
	const UnitOfMeasure* loguom = UnitOfMeasure::getGuessed( loguomlbl );
	bool isalt = false;
	bool isok = !loguom || ptype == PropertyRef::Other
	         || loguom->propType() == ptype;
	if ( !isok && altpr )
	    isok = isalt = loguom->propType() == altpr->stdType();
	if ( isok )
	{
	    ret += idx;
	    if ( arealt )
		*arealt += isalt;
	}
    }

    return ret;
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
    return UnitOfMeasure::getGuessed(unitmeaslbl_);
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

    const UnitOfMeasure* curuom = unitOfMeasure();
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
