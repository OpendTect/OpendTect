/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2001
-*/


#include "seissingtrcproc.h"

#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "multiid.h"
#include "ptrman.h"
#include "scaler.h"
#include "seisioobjinfo.h"
#include "seispacketinfo.h"
#include "seispsioprov.h"
#include "seispswrite.h"
#include "seisread.h"
#include "seisresampler.h"
#include "seisselection.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "trckeyzsampling.h"

#include "uistrings.h"

#define mInitMembers \
	Executor(nm) \
	, wrr_(*new SeisTrcWriter(&out)) \
	, nrskipped_(0) \
	, intrc_(*new SeisTrc) \
	, nrwr_(0) \
	, wrrkey_(*new MultiID) \
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
	, currdridx_(-1) \

#define mInitVars() \
    wrrkey_ = out.key(); \
    worktrc_ = &intrc_; \
    curmsg_ = msg; \
    worktrc_ = &intrc_

#define mHandlePastLastReader(to_do) \
    if ( !rdrs_.validIdx(currdridx_) ) \
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
    {
	curmsg_ = tr("No input specified");
	return;
    }

    for ( int idx=0; idx<objset.size(); idx++ )
    {
	IOPar* iop = iopset && iopset->size() > idx ? (*iopset)[idx] : 0;
	addReader( *objset[idx], iop );
    }
}


SeisSingleTraceProc::~SeisSingleTraceProc()
{
    delete resampler_;
    delete &wrr_;
    deepErase( rdrs_ );
    delete &intrc_;
    delete &wrrkey_;
    delete scaler_;
}


void SeisSingleTraceProc::setInput( const IOObj& in, const IOObj& out,
			const char* nm, const IOPar* iop, const uiString& msg )
{
    wrr_.setIOObj( &out );
    deepErase( rdrs_ );
    curmsg_ = msg;
    setName( nm );
    addReader( in, iop );
}


bool SeisSingleTraceProc::addReader( const IOObj& ioobj, const IOPar* iop )
{
    SeisTrcReader* rdr = new SeisTrcReader( &ioobj );
    if ( !rdr->ioObj() )
    {
	curmsg_ = tr("Cannot find read object");
	delete rdr;
	return false;
    }

    const bool is3d = !rdr->is2D() && !wrr_.is2D();
    if ( is3d && wrr_.ioObj()->key() == rdr->ioObj()->key() )
    {
	curmsg_ = tr("Input equals output");
	delete rdr;
	return false;
    }

    bool szdone = false;
    if ( totnr_ < 0 )
	totnr_ = 0;

    if ( iop )
    {
	rdr->usePar( *iop );
	if ( rdr->selData() )
	{
	    totnr_ += rdr->selData()->expectedNrTraces( !is3d );
	    if ( rdr->isPS() )
	    {
		const int nroffsets = rdr->getNrOffsets();
		if ( !mIsUdf(nroffsets) )
		    totnr_ *= nroffsets;
	    }

	    szdone = true;
	    if ( is3d && rdrs_.isEmpty() )
		wrr_.setSelData( rdr->selData()->clone() );
	}
	else if ( !is3d )
	{
	    // 2D, so let's make sure the line key is set
	    Seis::SelData* sd = Seis::SelData::get(Seis::Range);
	    sd->usePar( *iop );
	    rdr->setSelData( sd );
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
	SeisTrcTranslator* str = rdr->seisTranslator();
	if ( str )
	{
	    int estnr = str->estimatedNrTraces();
	    if ( estnr > 0 )
		{ szdone = true; totnr_ += estnr; }
	}

	if ( !szdone )
	    allszsfound_ = false;
    }

    rdr->setComponent( compnr_ );

    rdrs_ += rdr;

    if ( rdrs_.size() == 1 )
    {
	is3d_ = !rdr->is2D() && !wrr_.is2D();
	wrr_.setCrFrom( rdr->ioObj()->fullUserExpr() );
	if ( iop )
	    wrr_.auxPars() = *iop;

	nextReader();
    }

    return true;
}


bool SeisSingleTraceProc::nextReader()
{
    currdridx_++;
    mHandlePastLastReader( return false );

    SeisTrcReader* currdr = rdrs_[currdridx_];
    if ( !currdr->prepareWork() )
    {
	curmsg_ = currdr->errMsg();
	return false;
    }

    wrr_.setGeomIDProvider( currdr->geomIDProvider() );

    return true;
}


void SeisSingleTraceProc::setScaler( Scaler* newsclr )
{
    if ( newsclr == scaler_ )
	return;

    delete scaler_; scaler_ = newsclr;
    for ( int idx=0; idx<rdrs_.size(); idx++ )
	rdrs_[idx]->forceFloatData( scaler_ ? true : false );
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
    TrcKeyZSampling cs;
    if ( cs.usePar( iop ) )
    {
	SeisResampler* resmplr = new SeisResampler( cs, is2d );
	setResampler( resmplr );
    }

    setScaler( sclr );
    skipNullTraces( nulltrcpol < 1 );
    fillNullTraces( nulltrcpol == 2 );
    setExtTrcToSI( exttrcs );
}


uiString SeisSingleTraceProc::uiMessage() const
{
    if ( !curmsg_.isEmpty() )
	return curmsg_;

    if ( currdridx_ < rdrs_.size() )
    {
	const SeisTrcReader* currdr = rdrs_[currdridx_];

	uiString ret;
	if ( !currdr->is2D() )
	    ret  = uiStrings::sData();
	else
	{
	    Pos::GeomID geomid = currdr->geomID();
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


uiString SeisSingleTraceProc::uiNrDoneText() const
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
    SeisTrcReader& currdr = *rdrs_[currdridx_];

    int rv = currdr.get( intrc_.info() );
    if ( rv == 0 )
    {
	if ( !nextReader() )
	{
	    wrapUp();
	    return Finished();
	}

	return MoreToDo();
    }
    else if ( rv < 0 )
    {
	curmsg_ = currdr.errMsg();
	if ( currdridx_ == rdrs_.size()-1 )
	    return ErrorOccurred();

	 currdridx_++; return MoreToDo();
    }
    else if ( rv == 2 )
	return 2;

    worktrc_ = &intrc_;
    skipcurtrc_ = false;
    traceselected_.trigger();
    if ( skipcurtrc_ )
    {
	nrskipped_++;
	return 2;
    }

    if ( !currdr.get(intrc_) )
    {
	curmsg_ = currdr.errMsg();
	if ( currdridx_ == rdrs_.size()-1 )
	    return ErrorOccurred();
    }

    return MoreToDo();
}


void SeisSingleTraceProc::prepareNullFilling()
{
    if ( rdrs_.isEmpty() || !is3d_ || wrr_.isPS() )
    {
	fillnull_ = false;
	return;
    }

    const SeisTrcReader& rdr = *rdrs_[0];
    const Seis::SelData* sd = rdr.selData();
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
	filltrc_->setNrComponents( intrc_.nrComponents() );
	filltrc_->zero();
	filltrc_->info().nr = 0;
	filltrc_->info().offset = 0;
	filltrc_->info().azimuth = 0;
	filltrc_->info().refnr = filltrc_->info().pick = mUdf(float);
    }

    if ( fillbid_.inl() > fillhs_.stop_.inl() )
    {
	wrapUp();
	return Finished();
    }

    SeisTrcReader& currdr = *rdrs_[currdridx_];
    const bool wantbid = !currdr.selData() || currdr.selData()->isOK(fillbid_);
    bool neednulltrc = !wantbid || !currdr.seisTranslator()->goTo(fillbid_);
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
	worktrc_->info().binid = fillbid_;
	worktrc_->info().coord = SI().transform( fillbid_ );
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
	if ( !worktrc_ )
	{
	    nrskipped_++;
	    return false;
	}
    }

    if ( skipnull_ && worktrc_->isNull() )
	skipcurtrc_ = true;

    if ( !skipcurtrc_ )
	proctobedone_.trigger();

    if ( skipcurtrc_ )
    {
	nrskipped_++;
	return false;
    }

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
	mHandlePastLastReader( currdridx_-- );
	const SeisTrcReader& currdr = *rdrs_[currdridx_];
	SeisTrcTranslator& wrtr = *wrr_.seisTranslator();
	wrtr.setCurGeomID( currdr.geomID() );
	if ( currdr.ioObj() )
	{
	    StringPair datanm( currdr.ioObj()->name() );
	    const SeisTrcTranslator* trl = currdr.seisTranslator();
	    if ( trl && trl->componentInfo().size()>1 && trl->nrSelComps()==1 )
	    {
		const int selcomp = trl->selComp( 0 );
		if ( trl->componentInfo().validIdx(selcomp) )
		    datanm.second() = trl->componentInfo()[selcomp]->name();
	    }

	    wrtr.setCurLineKey( LineKey(datanm.getCompString()) );
	}

	if ( currdr.is2D() )
	{
	    wrtr.packetInfo().inlrg.set( currdr.geomID(), currdr.geomID(), 1 );
	    if ( currdr.selData() )
		wrtr.packetInfo().crlrg = currdr.selData()->crlRange();
	    else
		wrtr.packetInfo().crlrg = currdr.curTrcNrRange();
	}
	else
	{
	    if ( !wrr_.prepareWork(*worktrc_) )
	    {
		curmsg_ = wrr_.errMsg();
		return false;
	    }

	    if ( currdr.seisTranslator() && worktrc_->nrComponents() > 1 )
	    {
		SeisTrcTranslator& rdtr = const_cast<SeisTrcTranslator&>(
						*currdr.seisTranslator() );
		for ( int icomp=0; icomp<wrtr.componentInfo().size(); icomp++ )
		    wrtr.componentInfo()[icomp]->setName(
				rdtr.componentInfo()[icomp]->name() );
	    }
	}
    }

    if ( !wrr_.put(*worktrc_) )
    {
	curmsg_ = wrr_.errMsg();
	return false;
    }

    nrwr_++;
    return true;
}


int SeisSingleTraceProc::nextStep()
{
    mHandlePastLastReader( return Finished() );

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


void SeisSingleTraceProc::wrapUp()
{
    wrr_.close();
}
