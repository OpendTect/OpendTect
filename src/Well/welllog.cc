/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellextractdata.h"
#include "welllog.h"
#include "welllogdisp.h"
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

const char* Well::LogSet::sKeyDefMnem() { return "Default Mnemonic"; }

// ---- Well::LogSet


Well::LogSet::LogSet()
    : logAdded(this)
    , logRemoved(this)
{
    mAttachCB( MNC().customMnemonicRemoved, Well::LogSet::mnemonicRemovedCB );
    init();
}


Well::LogSet::~LogSet()
{
    detachAllNotifiers();
    setEmpty( true );
}


void Well::LogSet::getNames( BufferStringSet& nms, bool onlyloaded ) const
{
    nms.setEmpty();
    for ( const auto* log : logs_ )
    {
	if ( !onlyloaded || log->isLoaded() )
	    nms.add( log->name() );
    }
}


void Well::LogSet::add( Well::Log* wl )
{
    if ( !wl )
	return;

    Well::Log* log = getLog( wl->name().buf() );
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
    if ( wl.isEmpty() )
	return;

    if ( mIsUdf(dahintv_.start_) )
    {
        dahintv_.start_ = wl.dah(0);
        dahintv_.stop_ = wl.dah(wl.size()-1);
    }
    else
    {
        if ( dahintv_.start_ > wl.dah(0) )
            dahintv_.start_ = wl.dah(0);

        if ( dahintv_.stop_ < wl.dah(wl.size()-1) )
            dahintv_.stop_ = wl.dah(wl.size()-1);
    }
}


void Well::LogSet::updateDahIntvs()
{
    for ( auto* log : logs_ )
    {
	log->ensureAscZ();
	updateDahIntv( *log );
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
    return log && log->isLoaded();
}


bool Well::LogSet::areAllLoaded() const
{
    for ( const auto* log : logs_ )
	if ( !log->isLoaded() )
	    return false;

    return true;
}


bool Well::LogSet::isPresent( const char* nm ) const
{
    for ( const auto* log : logs_ )
    {
	if ( log->name() == nm )
	    return true;
    }

    return false;
}


Well::Log* Well::LogSet::remove( int logidx )
{
    Log* log = logs_[logidx];
    logs_ -= log;
    ObjectSet<Well::Log> tmp( logs_ );
    logs_.setEmpty();
    init();
    for ( int idx=0; idx<tmp.size(); idx++ )
	add( tmp[idx] );

    logRemoved.trigger();
    return log;
}


void Well::LogSet::setEmpty( bool withdelete )
{
    if ( withdelete )
    {
	deepErase( logs_ );
	logRemoved.trigger();
    }
    else
	logs_.erase();
}


void Well::LogSet::removeTopBottomUdfs()
{
    for ( auto* log : logs_ )
	log->removeTopBottomUdfs();
}


void Well::LogSet::mnemonicRemovedCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( const Mnemonic&, mn, cb );
    removeDefault( mn );
}


void Well::LogSet::getAllAvailMnems( MnemonicSelection& mns ) const
{
    for ( const auto* log : logs_ )
    {
	if ( log->mnemonic(false) )
	    mns.addIfNew( log->mnemonic() );
    }
}


TypeSet<int> Well::LogSet::getLogsWithNoMnemonics() const
{
    TypeSet<int> ret;
    for ( const auto* log : logs_ )
    {
	if ( !log->mnemonic(false) )
	    ret += logs_.indexOf(log);
    }

    return ret;
}


TypeSet<int> Well::LogSet::getSuitable( const Mnemonic& mn ) const
{
    TypeSet<int> ret;
    for ( const auto* log : logs_ )
    {
	if ( mn.isCompatibleWith(log->mnemonic(true)) )
	    ret += logs_.indexOf(log);
    }

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


bool Well::LogSet::hasDefaultFor( const Mnemonic& mnem ) const
{
    for ( const auto* deflog : defaultlogs_ )
    {
	if ( &deflog->first == &mnem )
	    return true;
    }

    return false;
}


bool Well::LogSet::setDefaultMnemLog( const Mnemonic& mnem,
				      const char* lognm )
{
    for ( auto* deflog : defaultlogs_ )
    {
	if ( &deflog->first == &mnem )
	{
	    deflog->second = lognm;
	    return true;
	}
    }

    auto* defaultlog =
		new std::pair<const Mnemonic&,BufferString>( mnem,lognm );
    if ( !defaultlog )
	return false;

    defaultlogs_.add( defaultlog );
    return true;
}


bool Well::LogSet::removeDefault( const Mnemonic& mnem )
{
    for ( int idx=defaultlogs_.size()-1; idx>=0; idx-- )
    {
	if ( &defaultlogs_.get(idx)->first == &mnem )
	{
	    delete defaultlogs_.removeSingle( idx );
	    return true;
	}
    }

    return false;
}


bool Well::LogSet::isDefaultLog( const char* lognm ) const
{
    BufferStringSet alldeflognms;
    getDefaultLogs( alldeflognms );
    if ( !alldeflognms.isPresent(lognm) )
	return false;

    return true;
}


void Well::LogSet::renameDefaultLog( const char* oldnm,
				     const char* newnm )
{
    if ( !isDefaultLog(oldnm) )
	return;

    for ( auto* defaultlog : defaultlogs_ )
    {
	if ( !defaultlog->second.isEqual(oldnm) )
	    continue;

	defaultlog->second = newnm;
    }
}


const Well::Log* Well::LogSet::getLog( const Mnemonic& mnem ) const
{
    if ( hasDefaultFor(mnem) )
	return getDefaultLog( mnem );

    const TypeSet<int> logidxs = getSuitable( mnem );
    if ( logidxs.isEmpty() )
	return nullptr;

    return &getLog( logidxs.first() );
}


const Well::Log* Well::LogSet::getDefaultLog( const Mnemonic& mnem ) const
{
    if ( !hasDefaultFor(mnem) )
	return nullptr;

    for ( const auto* deflog : defaultlogs_ )
    {
	if ( &deflog->first == &mnem )
	    return getLog( deflog->second.buf() );
    }

    return nullptr;
}


void Well::LogSet::getDefaultLogs( BufferStringSet& deflognms,
				   bool onlyloaded ) const
{
    for ( auto* deflog : defaultlogs_ )
    {
	const BufferString& deflognm = deflog->second;
	if ( !onlyloaded )
	{
	    deflognms.addIfNew( deflognm );
	    continue;
	}

	if ( isLoaded(deflognm) )
	    deflognms.addIfNew( deflognm );
    }
}


void Well::LogSet::defaultLogUsePar( const IOPar& iop )
{
    PtrMan<IOPar> defpar = iop.subselect( sKeyDefMnem() );
    if ( !defpar )
	return;

    IOParIterator iter( *defpar );
    BufferString mnem, deflognm;
    while ( iter.next(mnem,deflognm) )
    {
	const Mnemonic* currmnem = MNC().getByName( mnem, false );
	if ( currmnem )
	    setDefaultMnemLog( *currmnem, deflognm );
    }
}


void Well::LogSet::defaultLogFillPar( IOPar& iop ) const
{
    for ( const auto* deflog : defaultlogs_ )
    {
	iop.set( IOPar::compKey(sKeyDefMnem(),deflog->first.name()),
		 deflog->second );
	// To-Do: need to be made more robust
    }
}


const Mnemonic* Well::LogSet::getMnemonicOfLog( const char* nm ) const
{
    const Well::Log* log = getLog( nm );
    return log ? log->mnemonic( true ) : nullptr;
}


// ---- Well::Log

Well::Log::Log( const char* nm )
    : DahObj(nm)
    , range_(mUdf(float),-mUdf(float))
{
    mAttachCB( MNC().customMnemonicRemoved, Log::setMnemonicNullCB );
}


Well::Log::Log( const Log& oth )
    : DahObj("")
{
    *this = oth;
    mAttachCB( MNC().customMnemonicRemoved, Log::setMnemonicNullCB );
}


Well::Log::~Log()
{
    detachAllNotifiers();
}


Well::Log& Well::Log::operator =( const Well::Log& oth )
{
    if ( &oth == this )
	return *this;

    DahObj::operator=( oth );
    vals_ = oth.vals_;
    range_ = oth.range_;
    mn_ = oth.mn_;
    uom_ = oth.uom_;
    mnemlbl_ = oth.mnemlbl_;
    unitmeaslbl_ = oth.unitmeaslbl_;
    iscode_ = oth.iscode_;
    pars_ = oth.pars_;

    return *this;
}


Well::Log* Well::Log::clone() const
{
    return new Log( *this );
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

    return val < 10000.f && mIsEqual(val,mCast(float,mNINT32(val)),eps);
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
	    {
		dah1 = dah( idx );
		val1 = val;
		found1 = true;
		break;
	    }
	}
    }
    if ( idx1 < size()-1 )
    {
	for ( int idx=idx1+1; idx<size(); idx++ )
	{
	    const float val = value( idx );
	    if ( !mIsUdf(val) )
	    {
		dah2 = dah( idx );
		val2 = val;
		found2 = true;
		break;
	    }
	}
    }

    if ( !found1 && !found2 )
	return 0;
    if ( !found1 )
	return val2;
    if ( !found2 )
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
    iscode_ = true;
    range_.setUdf();
    for ( int idx=0; idx<size(); idx++ )
    {
	const float val = vals_[idx];
	range_.include( val );
	if ( !valIsCode(val,1e-3f) )
	    iscode_ = false;;
    }
}


void Well::Log::setValueRange( const Interval<float>& valrg )
{
    range_ = valrg;
    if ( range_.isUdf() || isLoaded() )
	return;

    iscode_ = valIsCode(range_.start_,1e-3f) && valIsCode(range_.stop_,1e-3f);
}

#define mDefZStep 0.1524f
#define mMaxGap 1.f
void Well::Log::prepareForDisplay()
{
    const float zfacmtofeet = SI().zIsTime() && SI().depthsInFeet()
							? mToFeetFactorF
							: 1.f;
    const float startdah = dahRange().start_;
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
    if ( dah_.size() < 2 )
	return;

    const int sz = dah_.size();
    if ( dah_[0] < dah_[sz-1] )
	return;

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
        defrg.start_++;
    }

    for ( int idx=sz-1; idx>=defrg.start_; idx-- )
    {
	if ( !mIsUdf(vals_[idx]) )
	    break;

	dah_.removeSingle( idx );
	vals_.removeSingle( idx );
    }

    if ( defrg.start_ == 0 )
    {
	updateDahRange();
	return;
    }

    TypeSet<float> newval, newdah;
    for ( int idx=defrg.start_; idx<size(); idx++ )
    {
	newdah += dah_[idx];
	newval += vals_[idx];
    }

    dah_ = newdah;
    vals_ = newval;
    updateDahRange();
}


const char* Well::Log::mnemonicLabel() const
{
    if ( mnemonic() )
	return mnemonic()->name();

    return mnemlbl_.buf();
}


const Mnemonic* Well::Log::mnemonic( bool setifnull ) const
{
    if ( !mn_ && setifnull )
	return getNonConst(*this).guessMnemonic();

    return mn_;
}


bool Well::Log::haveMnemonic() const
{
    return !mnemlbl_.isEmpty() && mnemlbl_ != Mnemonic::undef().name();
}


const Mnemonic* Well::Log::guessMnemonic()
{
    if ( mn_ )
	return mn_;

    const UnitOfMeasure* uom = unitOfMeasure();
    if ( !uom && haveUnit() )
	uom = UoMR().get( unitMeasLabel() );

    const BufferStringSet hintnms( name().str() );
    const Mnemonic* mn = nullptr;
    if ( mnemlbl_ == Mnemonic::undef().name() )
	mn = &Mnemonic::undef();
    else
    {
	const char* nm = mnemlbl_.buf();
	if ( isCode() )
	    mn = MnemonicSelection::getGuessed( nm, Mnemonic::Class, &hintnms );
	if ( !mn )
	    mn = MnemonicSelection::getGuessed( nm, uom, &hintnms );
	if ( !mn )
	    mn = &Mnemonic::undef();
    }

    setMnemonic( *mn );
    return mn;
}


void Well::Log::setMnemonic( const Mnemonic& mn )
{
    mn_ = &mn;
    mnemlbl_ = mn_->name();
}


void Well::Log::setMnemonicNullCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( const Mnemonic&, mn, cb );
    if ( mn_ == &mn )
	mn_ = nullptr;
}


void Well::Log::setMnemonicLabel( const char* mnem, bool setifnull )
{
    const StringView mnnm( mnem );
    if ( mnnm.isEmpty() )
    {
	mn_ = nullptr;
	mnemlbl_.setEmpty();
	return;
    }

    if ( mnnm == Mnemonic::undef().name() )
    {
	mn_ = &Mnemonic::undef();
	mnemlbl_.set( mnem );
	return;
    }

    const Mnemonic* mn = MNC().getByName( mnem, false );
    if ( mn )
    {
	setMnemonic( *mn );
	return;
    }

    if ( setifnull || !mn_ )
    {
	guessMnemonic();
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

    outlog->setName( getName() );
    outlog->setUnitMeasLabel( unitMeasLabel() );
    outlog->setMnemonicLabel( mnemonicLabel() );
    return outlog;
}


mDefParallelCalc5Pars(LogUpScaler, od_static_tr("LogUpScaler", "Upscale a log"),
    const Well::Log&, login, Well::Log&, logout,
    const StepInterval<float>&, dahrg, const Stats::UpscaleType, uptype,
    const bool, logisvel)
mDefParallelCalcBody( ,
const float dah =  logout_.dah(idx);
const float val = Well::LogDataExtracter::calcVal(login_, dah, dahrg_.step_,
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
    outlog->setUnitMeasLabel( unitMeasLabel() );
    outlog->setMnemonicLabel( mnemonicLabel() );
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
    outlog->setUnitMeasLabel( unitMeasLabel() );
    outlog->setMnemonicLabel( mnemonicLabel() );
    return outlog;
}


Well::Log* Well::Log::createSampledLog( const StepInterval<float>& dahrg,
					float val)
{
    Well::Log* wl = new Well::Log;
    StepInterval<float> outdahrg( dahrg );
    outdahrg.sort();
    outdahrg.stop_ =
		Math::Floor( outdahrg.stop_/outdahrg.step_ ) * outdahrg.step_;
    outdahrg.start_ =
		Math::Floor( outdahrg.start_/outdahrg.step_ ) * outdahrg.step_;
    const int nr = outdahrg.nrSteps() + 1;
    for (int idx=0; idx<nr; idx++ )
    {
	const float dah = outdahrg.atIndex( idx );
	wl->addValue( dah, val );
    }

    return wl;
}


// LogDisplayPars
Well::LogDisplayPars::LogDisplayPars( const char* nm )
    : name_(nm)
{
    range_.setUdf();
}


Well::LogDisplayPars::~LogDisplayPars()
{}


// LogDisplayParSet
Well::LogDisplayParSet::LogDisplayParSet()
{
    leftlogpar_ = new LogDisplayPars( "None" );
    rightlogpar_ = new LogDisplayPars( "None" );
}


Well::LogDisplayParSet::~LogDisplayParSet()
{
    delete leftlogpar_;
    delete rightlogpar_;
}



// Well::LogIter
Well::LogIter::LogIter( const Log& trck, bool atend )
    : DahObjIter(trck,atend)
{
}


Well::LogIter::~LogIter()
{
}


const Well::Log& Well::LogIter::log() const
{
    return static_cast<const Log&>( dahObj() );
}
