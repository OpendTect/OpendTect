/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2001
-*/


#include "seissingtrcproc.h"
#include "seiswrite.h"
#include "seispsioprov.h"
#include "seispswrite.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "seisprovider.h"
#include "seisselection.h"
#include "seisresampler.h"
#include "trckeyzsampling.h"
#include "dbkey.h"
#include "survinfo.h"
#include "ioobj.h"
#include "iopar.h"
#include "dbman.h"
#include "iostrm.h"
#include "scaler.h"
#include "ptrman.h"
#include "uistrings.h"
#include "keystrs.h"

#define mInitMembers \
	Executor(nm) \
	, wrr_(*new SeisTrcWriter(&out)) \
	, nrskipped_(0) \
	, intrc_(*new SeisTrc) \
	, nrwr_(0) \
	, wrrkey_(*new DBKey) \
	, trcsperstep_(10) \
	, scaler_(0) \
	, skipnull_(false) \
	, fillnull_(false) \
	, fillhs_(true) \
	, filltrc_(0) \
	, resampler_(0) \
	, extendtrctosi_( false ) \
	, is3d_(true) \
	, traceselected_(this) \
	, proctobedone_(this) \
	, allszsfound_(true) \
	, totnr_(-1) \
	, curprovidx_(-1) \

#define mInitVars() \
    wrrkey_ = out.key(); \
    worktrc_ = &intrc_; \
    curmsg_ = msg; \
    worktrc_ = &intrc_

#define mHandlePastLastReader(to_do) \
    if ( !provs_.validIdx(curprovidx_) ) \
	{ wrapUp(); to_do; }


SeisSingleTraceProc::SeisSingleTraceProc( const IOObj& out, const char* nm,
					  const uiString& msg )
    : mInitMembers
{
    mInitVars();
}


SeisSingleTraceProc::SeisSingleTraceProc( const IOObj& in, const IOObj& out,
					  const char* nm, const IOPar* iop,
					  const uiString& msg, int compnr )
    : mInitMembers
{
    mInitVars();
    compnr_ = compnr;

    setInput( in, out, nm, iop, msg );
}


SeisSingleTraceProc::SeisSingleTraceProc( ObjectSet<IOObj> objset,
					  const IOObj& out, const char* nm,
					  ObjectSet<IOPar>* iopset,
					  const uiString& msg, int compnr )
    : mInitMembers
{
    mInitVars();
    compnr_ = compnr;

    if ( objset.isEmpty() )
	{ curmsg_ = tr("No input specified"); return; }

    for ( int idx=0; idx<objset.size(); idx++ )
    {
	IOPar* iop = iopset && iopset->size() > idx ? (*iopset)[idx] : 0;
	addReader( *objset[idx], iop );
    }
}


SeisSingleTraceProc::~SeisSingleTraceProc()
{
    delete resampler_;
    deepErase( provs_ );
    delete &wrr_;
    delete &intrc_;
    delete &wrrkey_;
    delete scaler_;
}


void SeisSingleTraceProc::setInput( const IOObj& in, const IOObj& out,
			const char* nm, const IOPar* iop, const uiString& msg )
{
    wrr_.setIOObj( &out );
    deepErase( provs_ );
    curmsg_ = msg;
    setName( nm );
    addReader( in, iop );
}


bool SeisSingleTraceProc::addReader( const IOObj& ioobj, const IOPar* iop )
{
    uiRetVal uirv;
    Seis::Provider* prov = Seis::Provider::create( ioobj.key(), &uirv );
    if ( !prov )
	{ curmsg_ = uirv; delete prov; return false; }

    const bool is3d = !prov->is2D() && !wrr_.is2D();
    if ( is3d && wrr_.ioObj()->key()==prov->dbKey() )
	{ curmsg_ = tr("Input equals output"); delete prov; return false; }

    bool szdone = false;
    if ( totnr_ < 0 )
	totnr_ = 0;

    if ( iop )
    {
	prov->usePar( *iop );
	if ( prov->selData() )
	{
	    totnr_ += prov->selData()->expectedNrTraces( !is3d );
	    if ( prov->isPS() )
	    {
		const int nroffsets = prov->nrOffsets();
		if ( !mIsUdf(nroffsets) )
		    totnr_ *= nroffsets;
	    }
	    szdone = true;
	    if ( provs_.isEmpty() )
		wrr_.setSelData( prov->selData()->clone() );
	}
	else if ( !is3d )
	{
	    // 2D, so let's make sure the line key is set
	    Seis::SelData* sd = Seis::SelData::get(Seis::Range);
	    sd->usePar( *iop );
	    prov->setSelData( sd );
	}
    }

    if ( is3d && !wrr_.isPS() && !szdone )
    {
	TrcKeyZSampling cs( false );
	mDynamicCastGet(const IOStream*,iostrm,&ioobj)
	if ( iostrm )
	    iostrm->resetConnIdx();
	if ( SeisTrcTranslator::getRanges(ioobj,cs) )
	{
	    totnr_ += cs.nrInl() * cs.nrCrl();
	    szdone = true;
	}
    }

    if ( !szdone )
    {
	const int totnr = prov->totalNr();
	if ( totnr > 0 )
	    { szdone = true; totnr_ += totnr; }
	if ( !szdone )
	    allszsfound_ = false;
    }

    prov->selectComponent( compnr_ );

    provs_ += prov;

    if ( provs_.size() == 1 )
    {
	is3d_ = !prov->is2D() && !wrr_.is2D();
	wrr_.setCrFrom( ioobj.fullUserExpr() );
	if ( iop )
	    wrr_.auxPars() = *iop;
	nextReader();
    }

    return true;
}


bool SeisSingleTraceProc::nextReader()
{
    curprovidx_++;
    mHandlePastLastReader( return false );
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
    if ( resampler_ )
	resampler_->set2D( !is3d_ );
}


void SeisSingleTraceProc::setProcPars( const IOPar& iop, bool is2d )
{
    Scaler* sclr = Scaler::get( iop.find(sKey::Scale()) );
    const int nulltrcpol = toInt( iop.find("Null trace policy") );
    const bool exttrcs = iop.isTrue( "Extend Traces To Survey Z Range" );
    TrcKeyZSampling cs; cs.usePar( iop );
    SeisResampler* resmplr = new SeisResampler( cs, is2d );

    setScaler( sclr );
    skipNullTraces( nulltrcpol < 1 );
    fillNullTraces( nulltrcpol == 2 );
    setExtTrcToSI( exttrcs );
    setResampler( resmplr );
}


uiString SeisSingleTraceProc::message() const
{
    if ( !curmsg_.isEmpty() )
	return curmsg_;

    if ( curprovidx_ < provs_.size() )
    {
	const Seis::Provider* prov = provs_[curprovidx_];

	uiString ret;
	if ( !prov->is2D() )
	    ret  = uiStrings::sData();
	else
	{
	    const Pos::GeomID geomid = prov->curGeomID();
	    if ( geomid < 0 )
		ret = uiStrings::sData();
	    else
		{ ret = tr("'%1'").arg( toString(geomid) ); }
	}

	return tr("Handling %1").arg(ret);
    }

    return uiString::emptyString();
}


od_int64 SeisSingleTraceProc::nrDone() const
{
    return nrwr_;
}


uiString SeisSingleTraceProc::nrDoneText() const
{
    return tr( "Traces handled" );
}


od_int64 SeisSingleTraceProc::totalNr() const
{
    if ( !allszsfound_ || totnr_ < 3 || totnr_-nrskipped_ < 0 )
	return -1;

    return totnr_ - nrskipped_;
}


int SeisSingleTraceProc::getNextTrc()
{
    mHandlePastLastReader( return Finished() );
    Seis::Provider& curprov = *provs_[curprovidx_];

    const uiRetVal uirv = curprov.getNext( intrc_ );
    if ( !uirv.isOK() )
    {
	if ( isFinished(uirv) )
	{
	    if ( !nextReader() )
		{ wrapUp(); return Finished(); }

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


void SeisSingleTraceProc::prepareNullFilling()
{
    if ( provs_.isEmpty() || !is3d_ || wrr_.isPS() )
	{ fillnull_ = false; return; }

    const Seis::SelData* sd = provs_[0]->selData();
    if ( sd && !sd->isAll() )
    {
	Interval<int> rg( sd->inlRange() );
	fillhs_.start_.inl() = rg.start;
	fillhs_.stop_.inl() = rg.stop;
	rg = sd->crlRange();
	fillhs_.start_.crl() = rg.start;
	fillhs_.stop_.crl() = rg.stop;
    }
    fillbid_ = BinID( fillhs_.start_.inl(), fillhs_.start_.crl() );
}


int SeisSingleTraceProc::getFillTrc()
{
    mHandlePastLastReader( return Finished() );

    if ( !filltrc_ )
    {
	prepareNullFilling();
	if ( !fillnull_ )
	    return getNextTrc();

	while ( true )
	{
	    int rv = getNextTrc();
	    if ( rv < 1 )
		return rv;
	    if ( rv == 1 )
	    {
		if ( prepareTrc() )
		{
		    break;
		}
	    }
	}
	filltrc_ = new SeisTrc( *worktrc_ );
	filltrc_->zero();
	filltrc_->info().offset_ = 0;
	filltrc_->info().azimuth_ = 0;
	filltrc_->info().refnr_ = filltrc_->info().pick_ = mUdf(float);
    }

    if ( fillbid_.inl() > fillhs_.stop_.inl() )
	{ wrapUp(); return Finished(); }

    const Seis::Provider& curprov = *provs_[curprovidx_];
    const Seis::SelData* seldata = curprov.selData();
    bool neednulltrc = seldata && !seldata->isOK(fillbid_);
    if ( !neednulltrc )
    {
	int rv = getNextTrc();
	if ( rv != 1 )
	    neednulltrc = true;
    }
    if ( neednulltrc )
    {
	worktrc_ = &intrc_;
	*worktrc_ = *filltrc_;
	worktrc_->info().setBinID( fillbid_ );
	worktrc_->info().coord_ = SI().transform( fillbid_ );
    }

    fillbid_.crl() += fillhs_.step_.crl();
    if ( fillbid_.crl() > fillhs_.stop_.crl() )
    {
	fillbid_.inl() += fillhs_.step_.inl();
	fillbid_.crl() = fillhs_.start_.crl();
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
	SeisTrc* newtrc = worktrc_->getExtendedTo( SI().zRange(true) );
	*const_cast<SeisTrc*>(worktrc_) = *newtrc;
	delete newtrc;
    }

    return true;
}


bool SeisSingleTraceProc::writeTrc()
{
    if ( nrwr_ < 1 && wrr_.seisTranslator() )
    {
	mHandlePastLastReader( curprovidx_-- );
	const Seis::Provider& curprov = *provs_[curprovidx_];
	SeisTrcTranslator& wrtr = *wrr_.seisTranslator();
	wrtr.setCurGeomID( curprov.curGeomID() );
	if ( curprov.is2D() )
	{
	    mDynamicCastGet(const Seis::Provider2D&,prov2d,curprov);
	    StepInterval<int> trcnrrg; ZSampling zsamp;
	    prov2d.getRanges( prov2d.curLineIdx(), trcnrrg, zsamp );
	    wrtr.packetInfo().crlrg = trcnrrg;
	}
	else
	{
	    if ( !wrr_.prepareWork(*worktrc_) )
		{ curmsg_ = wrr_.errMsg(); return false; }

	    BufferStringSet compnms;
	    const uiRetVal retval = curprov.getComponentInfo( compnms );
	    if ( !retval.isOK() || compnms.size()!=wrtr.componentInfo().size() )
		pErrMsg("Invalid component info.");
	    else if ( worktrc_->nrComponents() > 1 )
		for ( int icomp=0; icomp<wrtr.componentInfo().size();icomp++)
		    wrtr.componentInfo()[icomp]->setName(compnms.get(icomp));
	}
    }

    if ( !wrr_.put(*worktrc_) )
	{ curmsg_ = wrr_.errMsg(); return false; }

    nrwr_++;
    return true;
}


int SeisSingleTraceProc::nextStep()
{
    mHandlePastLastReader( return Finished() );

    for ( int idx=0; idx<trcsperstep_; idx++ )
    {
	int rv = fillnull_ ? getFillTrc() : getNextTrc();
	if ( rv < 1 )		return rv;
	else if ( rv > 1 )	continue;

	if ( !prepareTrc() )
	    continue;

	if ( !writeTrc() )
	    return Executor::ErrorOccurred();
    }
    return Executor::MoreToDo();
}


void SeisSingleTraceProc::wrapUp()
{
    wrr_.close();
}
