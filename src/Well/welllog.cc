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
#include "mnemonics.h"
#include "paralleltask.h"
#include "stattype.h"
#include "unitofmeasure.h"

const char* Well::Log::sKeyMnemLbl()	{ return "Mnemonic"; }
const char* Well::Log::sKeyUnitLbl()	{ return "Unit of Measure"; }
const char* Well::Log::sKeyHdrInfo()	{ return "Header info"; }
const char* Well::Log::sKeyStorage()	{ return "Storage type"; }
const char* Well::Log::sKeyDahRange()	{ return "Dah range"; }
const char* Well::Log::sKeyLogRange()	{ return "Log range"; }

// ---- Well::LogSet

void Well::LogSet::getNames( BufferStringSet& nms ) const
{
    for ( int idx=0; idx<logs_.size(); idx++ )
	nms.add( logs_[idx]->name() );
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
    }
    else
    {
	log->removeTopBottomUdfs();
	log->updateAfterValueChanges();
	if ( log->isEmpty() )
	    *log = *wl;
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

const char* Well::Log::mnemLabel() const
{
    if ( mnemlbl_.isEmpty() )
	return mnemonic()->name();

    return mnemlbl_;
}


const Mnemonic* Well::Log::mnemonic() const
{
    if (!mnemlbl_.isEmpty())
	return MNC().find( mnemlbl_ );
    else
	return isCode() ? MNC().getGuessed( propType() )
			: MNC().getGuessed( unitOfMeasure() );
}


void Well::Log::setMnemLabel( const char* mnem )
{
    mnemlbl_ = mnem;
}


Well::Log& Well::Log::operator =( const Well::Log& wl )
{
    if ( &wl != this )
    {
	DahObj::operator=( wl );
	vals_ = wl.vals_;
	range_ = wl.range_;
	iscode_ = wl.iscode_;
	setUnitMeasLabel( wl.unitMeasLabel() );
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


const UnitOfMeasure* Well::Log::unitOfMeasure() const
{
    return UnitOfMeasure::getGuessed(unitmeaslbl_);
}


void Well::Log::convertTo( const UnitOfMeasure* touom )
{
    const UnitOfMeasure* curuom = unitOfMeasure();
    if ( !curuom || !vals_.size() )
	return;

    for ( int idx=0; idx<vals_.size(); idx++ )
	convValue( vals_[idx], curuom, touom );

    if ( touom )
	unitmeaslbl_ = touom->symbol();
    else
    {
	PropertyRef::StdType tp = curuom->propType();
	const UnitOfMeasure* siuom = UoMR().getInternalFor( tp );
	unitmeaslbl_ = siuom ? siuom->symbol() : "";
    }
}


PropertyRef::StdType Well::Log::propType() const
{
    const UnitOfMeasure* uom = unitOfMeasure();
    return uom ? uom->propType() : ( isCode() ?
	    PropertyRef::Class : PropertyRef::Other );
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


bool Well::Log::insertAtDah( float dh, float val )
{
    mWellDahObjInsertAtDah( dh, val, vals_, false );
    range_.include( val );
    dahrange_.include( dh );
    return true;
}


Well::Log* Well::Log::cleanUdfs() const
{
    Well::Log* outlog = new Well::Log;
    outlog->setName( getName() );
    outlog->setUnitMeasLabel( unitMeasLabel() );
    outlog->setMnemLabel( mnemLabel() );

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
const float dah = logout_.dah(idx);
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
    const bool logisvel = propType() == PropertyRef::Vel;
    LogUpScaler upscaler( outlog->size(), *this, *outlog, dahrg, uptype,
			    logisvel );
    upscaler.execute();
    outlog->setName( getName() );
    outlog->setUnitMeasLabel( unitMeasLabel() );
    outlog->setMnemLabel( mnemLabel() );
    return outlog;
}


mDefParallelCalc3Pars(LogRegularSampler,
		  od_static_tr("LogRegularSampler", "Regularly sample a log"),
		      const Well::Log&, login, Well::Log&, logout,
		      const StepInterval<float>&, dahrg)
mDefParallelCalcBody( ,
const float dah = logout_.dah(idx);
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
    outlog->setUnitMeasLabel( unitMeasLabel() );
    outlog->setMnemLabel( mnemLabel() );
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

