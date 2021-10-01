/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/


#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"
#include "bufstringset.h"
#include "iopar.h"
#include "idxable.h"
#include "paralleltask.h"
#include "propertyref.h"
#include "stattype.h"
#include "unitofmeasure.h"

const char* Well::Log::sKeyMnemLbl()	{ return "Mnemonic"; }
const char* Well::Log::sKeyUnitLbl()	{ return "Unit of Measure"; }
const char* Well::Log::sKeyHdrInfo()	{ return "Header info"; }
const char* Well::Log::sKeyStorage()	{ return "Storage type"; }
const char* Well::Log::sKeyDahRange()	{ return "Dah range"; }
const char* Well::Log::sKeyLogRange()	{ return "Log range"; }

// ---- Well::LogSet


Well::LogSet::LogSet()
    : logAdded(this)
    , logRemoved(this)
{
    init();
}


Well::LogSet::~LogSet()
{
    NotifyStopper ns( logRemoved );
    setEmpty();
}


void Well::LogSet::getNames( BufferStringSet& nms, bool onlyloaded ) const
{
    nms.setEmpty();
    for ( int idx=0; idx<logs_.size(); idx++ )
    {
	if ( !onlyloaded )
	    nms.add( logs_[idx]->name() );
	else if ( logs_[idx]->isLoaded() )
	    nms.add( logs_[idx]->name() );
    }
}


void Well::LogSet::add( Well::Log* wl )
{
    if ( !wl ) return;
    Well::Log* log = getLog( wl->name() );
    if ( !log )
    {
	logs_ += wl;
	updateDahIntv( *wl );
	logAdded.trigger();
    }
    else
    {
	log->removeTopBottomUdfs();
	log->updateAfterValueChanges();
	if ( log->isEmpty() )
	    *log = *wl;
    }
}


void Well::LogSet::add( const Well::LogSet& wls )
{
    NotifyStopper ns( logAdded );
    const int prevsz = size();
    for ( int idx=0; idx<wls.size(); idx++ )
	add( new Well::Log(wls.getLog(idx)) );
    if ( prevsz < size() )
    {
	ns.enableNotification();
	logAdded.trigger();
    }
}


void Well::LogSet::updateDahIntv( const Well::Log& wl )
{
    if ( wl.isEmpty() ) return;

    if ( mIsUdf(dahintv_.start) )
	{ dahintv_.start = wl.dah(0); dahintv_.stop = wl.dah(wl.size()-1); }
    else
    {
	if ( dahintv_.start > wl.dah(0) )
	    dahintv_.start = wl.dah(0);
	if ( dahintv_.stop < wl.dah(wl.size()-1) )
	    dahintv_.stop = wl.dah(wl.size()-1);
    }
}


void Well::LogSet::updateDahIntvs()
{
    for ( int idx=0; idx<logs_.size(); idx++ )
    {
	logs_[idx]->ensureAscZ();
	updateDahIntv( *logs_[idx] );
    }
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


bool Well::LogSet::isLoaded( const char* nm ) const
{
    const Well::Log* log = getLog( nm );
    return log ? log->isLoaded() : false;
}


bool Well::LogSet::isPresent( const char* nm ) const
{
    for ( int idx=0; idx<logs_.size(); idx++ )
	if ( logs_[idx]->name() == nm )
	    return true;
    return false;
}


Well::Log* Well::LogSet::remove( int logidx )
{
    Log* log = logs_[logidx]; logs_ -= log;
    ObjectSet<Well::Log> tmp( logs_ );
    logs_.setEmpty();
    init();
    for ( int idx=0; idx<tmp.size(); idx++ )
	add( tmp[idx] );
    logRemoved.trigger();
    return log;
}


void Well::LogSet::setEmpty()
{
    deepErase( logs_ );
    logRemoved.trigger();
}


void Well::LogSet::removeTopBottomUdfs()
{
    for ( int idx=0; idx<logs_.size(); idx++ )
	logs_[idx]->removeTopBottomUdfs();
}


TypeSet<int> Well::LogSet::getSuitable( const Mnemonic& mn ) const
{
    TypeSet<int> ret;
    for ( const auto* log : logs_ )
	if ( mn.isCompatibleWith(log->mnemonic()) )
	    ret += logs_.indexOf(log);

    return ret;
}


TypeSet<int> Well::LogSet::getSuitable( Mnemonic::StdType ptype,
	const PropertyRef* altpr, BoolTypeSet* arealt ) const
{
    TypeSet<int> ret;
    if ( arealt )
	arealt->setEmpty();

    for ( int idx=0; idx<logs_.size(); idx++ )
    {
	const UnitOfMeasure* loguom = logs_[idx]->unitOfMeasure();
	bool isalt = false;
	bool isok = !loguom || ptype == Mnemonic::Other
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




Well::Log& Well::Log::operator =( const Well::Log& oth )
{
    if ( &oth != this )
    {
	DahObj::operator=( oth );
	vals_ = oth.vals_;
	range_ = oth.range_;
	mn_ = oth.mn_;
	uom_ = oth.uom_;
	mnemlbl_ = oth.mnemlbl_;
	unitmeaslbl_ = oth.unitmeaslbl_;
	iscode_ = oth.iscode_;
	pars_ = oth.pars_;
    }

    return *this;
}


bool Well::Log::isLoaded() const
{
// no values
    if ( isEmpty() )
	return false;

// only contains dah range
    if ( size()==2 && mIsUdf(value(0)) && mIsUdf(value(1)) )
	return false;

    return true;
}


static bool valIsCode( float val, float eps )
{
    if ( mIsUdf(val) )
	return true; //No reason for failure

    return mIsEqual(val,mCast(float,mNINT32(val)),eps);
}


void Well::Log::setValue( int idx, float val )
{
    if ( !vals_.validIdx(idx) )
	return;

    vals_[idx] = val;
    range_.include( val );

    if ( iscode_ && !valIsCode(val,1e-3f) )
	iscode_ = false;
}


float Well::Log::getValue( float dh, bool noudfs ) const
{
    if ( isEmpty() )
	return noudfs ? 0 : mUdf(float);

    int idx1;
    const float ret = gtVal( dh, idx1 );
    if ( !noudfs || !mIsUdf(ret) )
	return ret;

    float dah1=mUdf(float),val1=mUdf(float),dah2=mUdf(float),val2=mUdf(float);
    bool found1 = false, found2 = false;
    if ( idx1 > 0 )
    {
	for ( int idx=idx1; idx>=0; idx-- )
	{
	    const float val = value( idx );
	    if ( !mIsUdf(val) )
		{ dah1 = dah( idx ); val1 = val; found1 = true; break; }
	}
    }
    if ( idx1 < size()-1 )
    {
	for ( int idx=idx1+1; idx<size(); idx++ )
	{
	    const float val = value( idx );
	    if ( !mIsUdf(val) )
		{ dah2 = dah( idx ); val2 = val; found2 = true; break; }
	}
    }

    if ( !found1 && !found2 )
	return 0;
    else if ( !found1 )
	return val2;
    else if ( !found2 )
	return val1;

    if ( iscode_ )
	return val2;

    return ((dh-dah1) * val2 + (dah2-dh) * val1) / (dah2 - dah1);
}


float Well::Log::gtVal( float dh, int& idx1 ) const
{
    if ( IdxAble::findFPPos(dah_,dah_.size(),dh,-1,idx1) )
	return vals_[idx1];
    else if ( idx1 < 0 || idx1 == dah_.size()-1 )
	return mUdf(float);

    const int idx2 = idx1 + 1;
    const float v1 = vals_[idx1];
    const float v2 = vals_[idx2];

    const float d1 = dh - dah_[idx1];
    const float d2 = dah_[idx2] - dh;
    if ( iscode_ || mIsUdf(v1) || mIsUdf(v2) )
	return d1 > d2 ? v2 : v1;

    return ( d1*vals_[idx2] + d2*vals_[idx1] ) / (d1 + d2);
}


void Well::Log::addValue( float dh, float val )
{
    dah_ += dh;
    dahrange_.include( dh );
    vals_ += val;
    range_.include( val );

    if ( isEmpty() )
	iscode_ = valIsCode( val, 1e-3f );
    else if ( iscode_ && !valIsCode(val,1e-3f) )
	 iscode_ = false;
}


void Well::Log::updateAfterValueChanges()
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( !valIsCode(vals_[idx],1e-3f) )
	{
	    iscode_ = false;;
	    return;
	}
    }

    iscode_ = true;
}

#define mDefZStep 0.1524f
#define mMaxGap 1.f
void Well::Log::prepareForDisplay()
{
    const float zfacmtofeet = SI().zIsTime() && SI().depthsInFeet()
							? mToFeetFactorF
							: 1.f;
    const float startdah = dahRange().start;
    const int startidx = indexOf( startdah );
    for ( int idx=startidx+1; idx<size(); idx++ )
    {
	const float dah0 = dah_[idx-1];
	const float val0 = vals_[idx-1];
	if ( mIsUdf(val0) )
	    continue;

	const float dah1 = dah_[idx];
	const float gap = dah1-dah0;
	const float maxgap = mMaxGap*zfacmtofeet;
	if ( gap >= maxgap )
	{
	    dah_.insert( idx, dah0-(mDefZStep*zfacmtofeet) );
	    vals_.insert( idx, mUdf(float) );
	}
    }
}


void Well::Log::ensureAscZ()
{
    if ( dah_.size() < 2 ) return;
    const int sz = dah_.size();
    if ( dah_[0] < dah_[sz-1] ) return;
    const int hsz = sz / 2;
    for ( int idx=0; idx<hsz; idx++ )
    {
	Swap( dah_[idx], dah_[sz-idx-1] );
	Swap( vals_[idx], vals_[sz-idx-1] );
    }
}


void Well::Log::removeTopBottomUdfs()
{
    const int sz = size();
    Interval<int> defrg( 0, sz-1 );
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( !mIsUdf(vals_[idx]) )
	    break;
	defrg.start++;
    }
    for ( int idx=sz-1; idx>=defrg.start; idx-- )
    {
	if ( !mIsUdf(vals_[idx]) )
	    break;
	dah_.removeSingle( idx ); vals_.removeSingle( idx );
    }

    if ( defrg.start == 0 )
    {
	updateDahRange();
	return;
    }

    TypeSet<float> newval, newdah;
    for ( int idx=defrg.start; idx<size(); idx++ )
	{ newdah += dah_[idx]; newval += vals_[idx]; }
    dah_ = newdah; vals_ = newval;
    updateDahRange();
}


const char* Well::Log::mnemLabel() const
{
    if ( mnemlbl_.isEmpty() && !mn_ )
	mnemonic();

    return mnemlbl_.buf();
}


const Mnemonic* Well::Log::mnemonic() const
{
    if ( !mn_ )
    {
	const Mnemonic* ret = MNC().getByName( mnemlbl_, false );
	if ( ret )
	{
	    const_cast<Log&>( *this ).setMnemonic( *ret );
	    return ret;
	}

	ret = MNC().getByName( name(), true );
	const UnitOfMeasure* uom = unitOfMeasure();
	if ( ret && ((uom && ret->stdType() == uom->propType()) || !uom) )
	{
	    const_cast<Log&>( *this ).setMnemonic( *ret );
	    return ret;
	}

	const_cast<Log&>( *this ).setMnemonic(
				isCode() ? MNC().getGuessed( propType() )
					 : MNC().getGuessed( uom ) );
    }

    return mn_;
}


void Well::Log::setMnemonic( const Mnemonic& mn )
{
    mn_ = &mn;
    mnemlbl_ = mn_->name();
}


void Well::Log::setMnemLabel( const char* mnem )
{
    const Mnemonic* mn = MNC().getByName( mnem, false );
    if ( mn )
    {
	setMnemonic( *mn );
	return;
    }

    mn_ = nullptr;
    mnemlbl_.set( mnem );
}


void Well::Log::setUnitOfMeasure( const UnitOfMeasure* newuom )
{
    uom_ = newuom;
    BufferString unitlbl( uom_ ? uom_->symbol() : "" );
    if ( unitlbl.isEmpty() && uom_ )
	unitlbl.set( uom_->name().str() );

    if ( !unitlbl.isEmpty() )
	unitmeaslbl_.set( unitlbl );
}


void Well::Log::setUnitMeasLabel( const char* newunitlbl, bool tryconvert )
{
    if ( !tryconvert )
    {
	uom_ = UoMR().get( newunitlbl );
	unitmeaslbl_.set( newunitlbl );
	return;
    }

    setUnitOfMeasure( UnitOfMeasure::getGuessed(newunitlbl) );
    if ( unitmeaslbl_.isEmpty() )
	unitmeaslbl_.set( newunitlbl );
}


void Well::Log::convertTo( const UnitOfMeasure* touom )
{
    const UnitOfMeasure* curuom = unitOfMeasure();
    if ( !curuom || vals_.isEmpty() )
	return;

    for ( auto& val : vals_ )
	convValue( val, curuom, touom );

    setUnitOfMeasure( touom ? touom : UoMR().getInternalFor(propType()) );
}


Mnemonic::StdType Well::Log::propType() const
{
    const Mnemonic* mn = mnemonic();
    if ( mn )
	return mn->stdType();

    const UnitOfMeasure* uom = unitOfMeasure();
    return uom ? uom->propType()
	       : ( isCode() ? Mnemonic::Class : Mnemonic::Other );
}


bool Well::Log::insertAtDah( float dh, float val )
{
    mWellDahObjInsertAtDah( dh, val, vals_, false );
    range_.include( val );
    dahrange_.include( dh );
    return true;
}


Well::Log* Well::Log::cleanUdfs() const
{
    auto* outlog = new Well::Log;
    outlog->setName( getName() );
    outlog->setMnemLabel( mnemLabel() );
    outlog->setUnitMeasLabel( unitMeasLabel() );

    bool first = true;
    bool newz = true;
    int numconsecudf = 0;
    int lastidx = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( mIsUdf(vals_[idx]) )
	{
	    if ( numconsecudf==0 )
		lastidx = idx;
	    newz = true;
	    numconsecudf++;
	    continue;
	}

	if ( first )
	{
	    outlog->addValue( dah_[idx], vals_[idx] );
	    first = false;
	    newz = false;

	}
	else if ( newz && numconsecudf>=2 )
	{
	    outlog->addValue( dah_[lastidx], vals_[lastidx] );
	    outlog->addValue( dah_[idx-1], vals_[idx-1] );
	    outlog->addValue( dah_[idx], vals_[idx] );
	    newz = false;
	}
	else
	    outlog->addValue( dah_[idx], vals_[idx] );
	numconsecudf = 0;
    }
    return outlog;
}


mDefParallelCalc5Pars(LogUpScaler, od_static_tr("LogUpScaler", "Upscale a log"),
    const Well::Log&, login, Well::Log&, logout,
    const StepInterval<float>&, dahrg, const Stats::UpscaleType, uptype,
    const bool, logisvel)
mDefParallelCalcBody( ,
const float dah =  logout_.dah(idx);
const float val = Well::LogDataExtracter::calcVal(login_, dah, dahrg_.step,
						    uptype_, logisvel_);
logout_.setValue(idx, val);
,
)


Well::Log* Well::Log::upScaleLog( const StepInterval<float>& dahrg ) const
{
    Well::Log* outlog = createSampledLog( dahrg, 1.0 );
    const Stats::UpscaleType uptype = isCode() ? Stats::UseMostFreq :
						    Stats::UseAvg;
    const bool logisvel = propType() == Mnemonic::Vel;
    LogUpScaler upscaler( outlog->size(), *this, *outlog, dahrg, uptype,
			    logisvel );
    upscaler.execute();
    outlog->setName( getName() );
    outlog->setMnemLabel( mnemLabel() );
    outlog->setUnitMeasLabel( unitMeasLabel() );
    return outlog;
}


mDefParallelCalc3Pars(LogRegularSampler,
		  od_static_tr("LogRegularSampler", "Regularly sample a log"),
		      const Well::Log&, login, Well::Log&, logout,
		      const StepInterval<float>&, dahrg)
mDefParallelCalcBody( ,
const float dah =  logout_.dah(idx);
const float val = login_.getValue( dah );
logout_.setValue(idx, val);
,
)


Well::Log* Well::Log::sampleLog( const StepInterval<float>& dahrg ) const
{
    Well::Log* outlog = createSampledLog( dahrg );
    LogRegularSampler sampler( outlog->size(), *this, *outlog, dahrg );
    sampler.execute();
    outlog->setName( getName() );
    outlog->setMnemLabel( mnemLabel() );
    outlog->setUnitMeasLabel( unitMeasLabel() );
    return outlog;
}


Well::Log* Well::Log::createSampledLog(const StepInterval<float>& dahrg,
				 const float val)
{
    Well::Log* wl = new Well::Log;
    StepInterval<float> outdahrg( dahrg );
    outdahrg.sort();
    outdahrg.stop = Math::Floor( outdahrg.stop/outdahrg.step ) * outdahrg.step;
    outdahrg.start = Math::Floor( outdahrg.start/outdahrg.step )
								* outdahrg.step;
    const int nr = outdahrg.nrSteps() + 1;
    for (int idx = 0; idx < nr; idx++)
    {
	const float dah = outdahrg.atIndex(idx);
	wl->addValue(dah, val);
    }
    return wl;
}
