/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2001
-*/


#include "seissingtrcproc.h"
#include "seisinfo.h"
#include "seisstorer.h"
#include "seispsioprov.h"
#include "seispswrite.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "seisprovider.h"
#include "seisrangeseldata.h"
#include "seisseldata.h"
#include "seisresampler.h"
#include "cubedata.h"
#include "cubesubsel.h"
#include "fullsubsel.h"
#include "dbkey.h"
#include "survinfo.h"
#include "ioobj.h"
#include "iopar.h"
#include "iostrm.h"
#include "scaler.h"
#include "ptrman.h"
#include "uistrings.h"
#include "keystrs.h"

#define mInitMembers(nm) \
	Executor(nm) \
	, nrskipped_(0) \
	, intrc_(*new SeisTrc) \
	, nrwr_(0) \
	, wrrkey_(*new DBKey) \
	, trcsperstep_(10) \
	, scaler_(0) \
	, skipnull_(false) \
	, fillnull_(false) \
	, filltrc_(0) \
	, fillchss_(0) \
	, filliter_(0) \
	, resampler_(0) \
	, extendtrctosi_( false ) \
	, is3d_(true) \
	, traceselected_(this) \
	, proctobedone_(this) \
	, allszsfound_(true) \
	, totnr_(-1) \
	, curprovidx_(-1) \

#define mPastLastReader() (!provs_.validIdx(curprovidx_))
#define mWrapUpIfPastLastReader() \
    if ( mPastLastReader() ) \
	return wrapUp();


SeisSingleTraceProc::SeisSingleTraceProc( const IOObj& out, const char* nm,
					  const uiString& msg )
    : mInitMembers(nm)
    , storer_(*new Seis::Storer(out))
{
    worktrc_ = &intrc_;
    curmsg_ = msg;
}


SeisSingleTraceProc::SeisSingleTraceProc( const IOObj& in, const IOObj& out,
					  const char* nm, const IOPar* iop,
					  const uiString& msg, int compnr,
					  bool forceFPdata )
    : mInitMembers(nm)
    , storer_(*new Seis::Storer(out))
{
    worktrc_ = &intrc_;
    curmsg_ = msg;
    compnr_ = compnr;
    forcefpdata_ = forceFPdata;

    setInput( in, out, nm, iop, msg );
}


SeisSingleTraceProc::SeisSingleTraceProc( ObjectSet<IOObj> objset,
					  const IOObj& out, const char* nm,
					  ObjectSet<IOPar>* iopset,
					  const uiString& msg, int compnr,
					  bool forceFPdata )
    : mInitMembers(nm)
    , storer_(*new Seis::Storer(out))
{
    worktrc_ = &intrc_;
    curmsg_ = msg;
    compnr_ = compnr;
    forcefpdata_ = forceFPdata;

    if ( objset.isEmpty() )
	{ curmsg_ = tr("No input specified"); return; }

    for ( int idx=0; idx<objset.size(); idx++ )
    {
	IOPar* iop = iopset && iopset->size() > idx ? (*iopset)[idx] : 0;
	addReader( *objset[idx], iop );
    }
}


SeisSingleTraceProc::SeisSingleTraceProc( Provider* prov, Storer* strr )
    : mInitMembers("Seismic data copier")
    , storer_(*strr)
{
    worktrc_ = &intrc_;
    addProv( prov, false );
    curmsg_ = tr("Copying");
}


SeisSingleTraceProc::SeisSingleTraceProc( const ObjectSet<Provider>& provs,
					  Storer* strr )
    : mInitMembers("Seismic data concatenator")
    , storer_(*strr)
{
    worktrc_ = &intrc_;
    for ( auto prov : provs )
	addProv( prov, false );

    curmsg_ = provs_.size() > 1 ? tr("Concatenating") : tr("Copying");
}


SeisSingleTraceProc::~SeisSingleTraceProc()
{
    deepErase( provs_ );
    delete resampler_;
    delete scaler_;
    delete filliter_;
    delete fillchss_;
    delete filltrc_;
    delete &storer_;
    delete &intrc_;
}


void SeisSingleTraceProc::setInput( const IOObj& in, const IOObj& out,
			const char* nm, const IOPar* iop, const uiString& msg )
{
    storer_.setOutput( out );
    deepErase( provs_ );
    curmsg_ = msg;
    setName( nm );
    addReader( in, iop );
}


void SeisSingleTraceProc::addProv( Provider* prov, bool szdone,
				    const IOPar* storauxiop )
{
    if ( totnr_ < 0 )
	totnr_ = 0;

    if ( !szdone )
    {
	const od_int64 totnr = prov->totalNr();
	if ( totnr > 0 )
	    { szdone = true; totnr_ += totnr; }
    }
    if ( !szdone )
	allszsfound_ = false;

    provs_ += prov;

    if ( provs_.size() == 1 )
    {
	is3d_ = !prov->is2D();
	storer_.setCrFrom( mainFileOf(prov->dbKey()) );
	if ( storauxiop )
	    storer_.auxPars() = *storauxiop;
	nextReader();
    }
}


bool SeisSingleTraceProc::addReader( const IOObj& ioobj, const IOPar* iop )
{
    uiRetVal uirv;
    if ( ioobj.pars().isEmpty() )
	ioobj.pars() = *iop;

    Seis::Provider* prov = Seis::Provider::create( ioobj, &uirv );
    if ( !prov )
	{ curmsg_ = uirv; delete prov; return false; }
    prov->selectComponent( compnr_ );

    const bool is3d = !prov->is2D();
    if ( is3d && !storer_.isPS() )
    {
	mDynamicCastGet(const IOStream*,iostrm,&ioobj)
	if ( iostrm )
	    iostrm->resetConnIdx();
    }

    if ( is3d && storer_.ioObj()->key()==prov->dbKey() )
	{ curmsg_ = tr("Input equals output"); delete prov; return false; }

    bool szdone = false;
    if ( iop )
    {
	prov->usePar( *iop );
	if ( prov->selData() )
	{
	    totnr_ += prov->selData()->expectedNrTraces();
	    if ( prov->isPS() )
	    {
		const int nroffsets = prov->nrOffsets();
		if ( !mIsUdf(nroffsets) )
		    totnr_ *= nroffsets;
	    }
	    szdone = true;
	}
    }

    if ( forcefpdata_ )
	prov->forceFPData();

    addProv( prov, szdone, iop );

    return true;
}


bool SeisSingleTraceProc::nextReader()
{
    curprovidx_++;
    if ( mPastLastReader() )
	{ wrapUp(); return false; }

    return true;
}


void SeisSingleTraceProc::setScaler( Scaler* newsclr )
{
    if ( newsclr == scaler_ ) return;
    delete scaler_; scaler_ = newsclr;
    for ( int idx=0; idx<provs_.size(); idx++ )
	provs_[idx]->forceFPData( scaler_ );
}


void SeisSingleTraceProc::setResampler( SeisResampler* r )
{
    delete resampler_; resampler_ = r;
}


void SeisSingleTraceProc::setProcPars( const IOPar& iop )
{
    Scaler* sclr = Scaler::get( iop.find(sKey::Scale()) );
    const int nulltrcpol = toInt( iop.find("Null trace policy") );
    const bool exttrcs = iop.isTrue( "Extend Traces To Survey Z Range" );
    if ( is3d_ )
    {
	CubeSubSel css; css.usePar( iop );
	setResampler( new SeisResampler( css ) );
    }
    else
    {
	auto* sd = new Seis::RangeSelData();
	sd->setForceIsAll( true );
	setResampler( new SeisResampler( *sd ) );
	delete sd;
    }

    setScaler( sclr );
    skipNullTraces( nulltrcpol < 1 );
    fillNullTraces( nulltrcpol == 2 );
    setExtTrcToSI( exttrcs );
}


uiString SeisSingleTraceProc::message() const
{
    if ( !curmsg_.isEmpty() )
	return curmsg_;

    if ( curprovidx_ < provs_.size() )
    {
	const Seis::Provider* prov = provs_[curprovidx_];
	return tr("Handling %1").arg( prov->is2D()
		? toUiString(prov->as2D()->curGeomID().name())
		: uiStrings::sData() );
    }

    return uiStrings::sFinished();
}


od_int64 SeisSingleTraceProc::nrDone() const
{
    return nrwr_;
}


uiString SeisSingleTraceProc::nrDoneText() const
{
    return tr("Traces handled");
}


od_int64 SeisSingleTraceProc::totalNr() const
{
    if ( !allszsfound_ || totnr_ < 3 || totnr_-nrskipped_ < 0 )
	return -1;

    return totnr_ - nrskipped_;
}


int SeisSingleTraceProc::getNextTrc()
{
    mWrapUpIfPastLastReader();
    Seis::Provider& curprov = *provs_[curprovidx_];

    const uiRetVal uirv = curprov.getNext( intrc_ );
    if ( !uirv.isOK() )
    {
	if ( isFinished(uirv) )
	{
	    if ( !nextReader() )
		return wrapUp();

	    return MoreToDo();
	}

	curmsg_ = uirv;
	if ( curprovidx_ == provs_.size()-1 )
	    return ErrorOccurred();

	curprovidx_++; return MoreToDo();
    }

    worktrc_ = &intrc_;
    skipcurtrc_ = false;
    traceselected_.trigger();
    if ( skipcurtrc_ )
	{ nrskipped_++; return 2; }

    return MoreToDo();
}


bool SeisSingleTraceProc::prepareNullFilling()
{
    if ( provs_.isEmpty() || !is3d_ || storer_.isPS() )
	{ fillnull_ = false; return false; }

    CubeHorSubSel chss;
    Survey::FullHorSubSel fhss;
    provs_[0]->getFullHorSubSel( fhss );
    for ( auto idx=1; idx<provs_.size(); idx++ )
    {
	Survey::FullHorSubSel curfhss;
	provs_[idx]->getFullHorSubSel( curfhss );
	fhss.merge( curfhss );
    }
    fillchss_ = new CubeHorSubSel( fhss.subSel3D() );
    filliter_ = new Survey::HorSubSelIterator( *fillchss_ );

    // Find the first actual trace; it will provide data type, size, etc.
    while ( true )
    {
	int rv = getNextTrc();
	if ( rv < 1 )
	    return false;
	if ( rv == 1 && prepareTrc() )
	    break;
    }
    filltrc_ = new SeisTrc( *worktrc_ );
    filltrc_->zero();
    filltrc_->info().offset_ = 0;
    filltrc_->info().azimuth_ = 0;
    filltrc_->info().refnr_ = filltrc_->info().pick_ = mUdf(float);

    return true;
}


int SeisSingleTraceProc::getFillTrc()
{
    mWrapUpIfPastLastReader();
    if ( !filltrc_ && !prepareNullFilling() )
	return wrapUp();
    if ( !filliter_->next() )
	return wrapUp();

    const auto& curprov = *provs_[curprovidx_]->as3D();
    const auto curbid( filliter_->binID() );
    uiRetVal uirv = curprov.getAt( curbid, intrc_ );
    if ( !uirv.isOK() )
    {
	intrc_ = *filltrc_;
	intrc_.info().setPos( curbid );
	intrc_.info().coord_ = SI().transform( curbid );
	worktrc_ = &intrc_;
    }

    return Executor::MoreToDo();
}


bool SeisSingleTraceProc::prepareTrc()
{
    if ( resampler_ )
    {
	worktrc_ = resampler_->get(intrc_);
	if ( !worktrc_ ) { nrskipped_++; return false; }
    }

    if ( skipnull_ && worktrc_->isNull() )
	skipcurtrc_ = true;

    if ( !skipcurtrc_ )
	proctobedone_.trigger();
    if ( skipcurtrc_ ) { nrskipped_++; return false; }

    if ( scaler_ )
	const_cast<SeisTrc*>(worktrc_)->data().scale( *scaler_ );

    if ( extendtrctosi_ )
    {
	SeisTrc* newtrc = worktrc_->getExtendedTo( SI().zRange() );
	*const_cast<SeisTrc*>(worktrc_) = *newtrc;
	delete newtrc;
    }

    return true;
}


bool SeisSingleTraceProc::writeTrc()
{
    if ( nrwr_ < 1 && storer_.translator() )
    {
	if ( mPastLastReader() )
	    curprovidx_--;
	const Seis::Provider& curprov = *provs_[curprovidx_];
	SeisTrcTranslator& wrtr = *storer_.translator();
	if ( !curprov.is2D() )
	{
	    uiRetVal uirv = storer_.prepareWork( *worktrc_ );
	    if ( !uirv.isOK() )
		{ curmsg_ = uirv; return false; }

	    BufferStringSet compnms;
	    curprov.getComponentInfo( compnms );
	    if ( compnms.size() != wrtr.componentInfo().size() )
		pErrMsg("Invalid component info");
	    else if ( worktrc_->nrComponents() > 1 )
		for ( int icomp=0; icomp<wrtr.componentInfo().size();icomp++)
		    wrtr.componentInfo()[icomp]->setName(compnms.get(icomp));
	}
    }

    uiRetVal uirv = storer_.put( *worktrc_ );
    if ( !uirv.isOK() )
	{ curmsg_ = uirv; return false; }

    nrwr_++;
    return true;
}


int SeisSingleTraceProc::nextStep()
{
    mWrapUpIfPastLastReader();

    for ( int idx=0; idx<trcsperstep_; idx++ )
    {
	int rv = fillnull_ ? getFillTrc() : getNextTrc();
	if ( rv < 1 )
	    return rv;
	else if ( rv > 1 )
	    continue;

	if ( !prepareTrc() )
	    continue;

	if ( !writeTrc() )
	    return Executor::ErrorOccurred();
    }
    return Executor::MoreToDo();
}


int SeisSingleTraceProc::wrapUp()
{
    const auto uirv = storer_.close();
    if ( uirv.isOK() )
	return Finished();

    curmsg_ = uirv;
    return ErrorOccurred();
}
