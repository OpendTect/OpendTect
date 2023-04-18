/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seissingtrcproc.h"

#include "coordsystem.h"
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


SeisSingleTraceProc::SeisSingleTraceProc( const SeisStoreAccess::Setup& inpsu,
					  const SeisStoreAccess::Setup& outsu,
					  const char* nm, const uiString& msg )
    : Executor(nm)
    , execnm_(nm)
    , initmsg_(msg)
    , inpsetup_(inpsu)
    , outsetup_(outsu)
    , intrc_(*new SeisTrc)
    , wrrkey_(*new MultiID)
    , fillhs_(true)
    , traceselected_(this)
    , proctobedone_(this)
{
    if ( outsu.ioobj_ )
	wrrkey_ = outsu.ioobj_->key();

    worktrc_ = &intrc_;
}


SeisSingleTraceProc::SeisSingleTraceProc( const IOObj& in, const IOObj& out,
					  const char* nm, const IOPar* iop,
					  const uiString& msg, int compnr )
    : SeisSingleTraceProc(SeisStoreAccess::Setup(in,nullptr),
			  SeisStoreAccess::Setup(out,nullptr), nm, msg)
{
    if ( iop )
    {
	inpsetup_.usePar( *iop );
	outsetup_.usePar( *iop );
    }

    inpsetup_.compnr( compnr );
    outsetup_.compnr( compnr );
}


SeisSingleTraceProc::~SeisSingleTraceProc()
{
    wrapUp();
    delete resampler_;
    delete &intrc_;
    delete &wrrkey_;
    delete scaler_;
}


bool SeisSingleTraceProc::is3D() const
{
    return Seis::is3D( inpsetup_.geomtype_ );
}


bool SeisSingleTraceProc::is2D() const
{
    return Seis::is2D( inpsetup_.geomtype_ );
}


bool SeisSingleTraceProc::isPS() const
{
    return Seis::isPS( inpsetup_.geomtype_ );
}


bool SeisSingleTraceProc::goImpl( od_ostream* strm, bool first, bool last,
				  int delay )
{
    wrapUp();
    const bool res = setInput() &&
		     Executor::goImpl( strm, first, last, delay );
    wrapUp();
    return res;
}


bool SeisSingleTraceProc::setInput()
{
    if ( is3D() && areEqual(inpsetup_.ioobj_,outsetup_.ioobj_) )
    {
	errmsg_ = tr("Input equals output");
	return false;
    }

    if ( !addReader() )
	return false;

    delete wrr_;
    wrr_ = new SeisTrcWriter( outsetup_ );
    wrr_->auxPars() = outsetup_.ioobj_->pars();
    wrr_->setCrFrom( reader(currdridx_)->ioObj()->fullUserExpr() );
    wrr_->setGeomIDProvider( reader(currdridx_)->geomIDProvider() );

    curmsg_ = initmsg_;
    setName( execnm_ );
    return true;
}


bool SeisSingleTraceProc::addReader()
{
    PtrMan<SeisTrcReader> rdr = new SeisTrcReader( inpsetup_ );
    if ( !rdr->ioObj() )
    {
	errmsg_ = tr("Cannot find read object");
	return false;
    }

    if ( totnr_ < 0 )
	totnr_ = 0;

    if ( !rdr->prepareWork() )
    {
	errmsg_ = rdr->errMsg();
	return false;
    }

    totnr_ = rdr->expectedNrTraces();
    if ( totnr_ <= 0 )
	allszsfound_ = false;

    rdr->setComponent( inpsetup_.compnr_ );
    rdrs_.add( rdr.release() );
    currdridx_++;

    return true;
}


#define mHandlePastLastReader(to_do) \
    if ( !rdrs_.validIdx(currdridx_) ) \
	{ to_do; }

bool SeisSingleTraceProc::nextReader()
{
    currdridx_++;
    mHandlePastLastReader( return false );

    SeisTrcReader* currdr = rdrs_.get( currdridx_ );
    if ( !currdr->prepareWork() )
    {
	errmsg_ = currdr->errMsg();
	return false;
    }

    return true;
}


void SeisSingleTraceProc::setScaler( Scaler* newsclr )
{
    if ( newsclr == scaler_ )
	return;

    delete scaler_; scaler_ = newsclr;
    for ( auto* rdr : rdrs_ )
	rdr->forceFloatData( scaler_ ? true : false );
}


void SeisSingleTraceProc::setResampler( SeisResampler* r )
{
    delete resampler_; resampler_ = r;
    if ( resampler_ )
	resampler_->set2D( is2D() );
}


void SeisSingleTraceProc::setProcPars( const IOPar& iop, bool is2d )
{
    Scaler* sclr = Scaler::get( iop.find(sKey::Scale()) );
    const int nulltrcpol = iop.find("Null trace policy").toInt();
    const bool exttrcs = iop.isTrue( "Extend Traces To Survey Z Range" );
    TrcKeyZSampling tks;
    if ( tks.usePar(iop) )
    {
	auto* resmplr = new SeisResampler( tks, is2d );
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
	uiString ret;
	const Pos::GeomID geomid = inpsetup_.geomid_;
	if ( Survey::is2DGeom(geomid) )
	    { ret = tr("'%1'").arg( geomid.toString() ); }
	else
	    ret = uiStrings::sTrace(mPlural);

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
    SeisTrcReader& currdr = *rdrs_.get( currdridx_ );

    const int rv = currdr.get( intrc_.info() );
    if ( rv == 0 )
    {
	if ( !nextReader() )
	    return Finished();

	return MoreToDo();
    }
    else if ( rv < 0 )
    {
	errmsg_ = currdr.errMsg();
	if ( currdridx_ == rdrs_.size()-1 )
	    return ErrorOccurred();

	 currdridx_++;
	 return MoreToDo();
    }
    else if ( rv == 2 )
	return WarningAvailable();

    worktrc_ = &intrc_;
    skipcurtrc_ = false;
    traceselected_.trigger();
    if ( skipcurtrc_ )
    {
	nrskipped_++;
	return WarningAvailable();
    }

    if ( !currdr.get(intrc_) )
    {
	errmsg_ = currdr.errMsg();
	if ( currdridx_ == rdrs_.size()-1 )
	    return ErrorOccurred();
    }

    return MoreToDo();
}


void SeisSingleTraceProc::prepareNullFilling()
{
    if ( rdrs_.isEmpty() || is2D() || isPS() )
    {
	fillnull_ = false;
	return;
    }

    const SeisTrcReader& rdr = *rdrs_.first();
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

	    if ( rv == 1 && prepareTrc() )
		break;
	}

	filltrc_ = new SeisTrc( *worktrc_ );
	filltrc_->setNrComponents( intrc_.nrComponents() );
	filltrc_->zero();
	filltrc_->info().seqnr_ = 0;
	filltrc_->info().offset = 0;
	filltrc_->info().azimuth = 0;
	filltrc_->info().refnr = filltrc_->info().pick = mUdf(float);
    }

    if ( fillbid_.inl() > fillhs_.stop_.inl() )
	return Finished();

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
	worktrc_->info().setPos( fillbid_ );
	worktrc_->info().coord = SI().transform( fillbid_ );
    }

    fillbid_.crl() += fillhs_.step_.crl();
    if ( fillbid_.crl() > fillhs_.stop_.crl() )
    {
	fillbid_.inl() += fillhs_.step_.inl();
	fillbid_.crl() = fillhs_.start_.crl();
    }

    return MoreToDo();
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
    if ( nrwr_ < 1 && wrr_->seisTranslator() )
    {
	mHandlePastLastReader( currdridx_-- );
	const SeisTrcReader& currdr = *rdrs_[currdridx_];
	SeisTrcTranslator& wrtr = *wrr_->seisTranslator();
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

	    wrtr.setDataName( datanm.getCompString() );
	}

	if ( is2D() )
	{
	    const int linenr = currdr.geomID().asInt();
	    wrtr.packetInfo().inlrg.set( linenr, linenr, 1 );
	    if ( currdr.selData() )
		wrtr.packetInfo().crlrg = currdr.selData()->crlRange();
	    else
		wrtr.packetInfo().crlrg = currdr.curTrcNrRange();
	}
	else
	{
	    if ( !wrr_->prepareWork(*worktrc_) )
	    {
		errmsg_ = wrr_->errMsg();
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

    if ( !wrr_->put(*worktrc_) )
    {
	errmsg_ = wrr_->errMsg();
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
	const int rv = fillnull_ ? getFillTrc() : getNextTrc();
	if ( rv < 1 )
	    return rv;
	else if ( rv > 1 )
	    continue;

	if ( !prepareTrc() )
	    continue;

	if ( !writeTrc() )
	    return ErrorOccurred();
    }

    return MoreToDo();
}


void SeisSingleTraceProc::wrapUp()
{
    currdridx_ = -1;
    if ( wrr_ )
	wrr_->close();

    deepErase( rdrs_ );
    deleteAndNullPtr( wrr_ );
}


//Deprecated implementations:

SeisSingleTraceProc::SeisSingleTraceProc( const IOObj& out, const char* nm,
					  const uiString& msg )
    : SeisSingleTraceProc(out,out,nm,nullptr,msg)
{
}


SeisSingleTraceProc::SeisSingleTraceProc( ObjectSet<IOObj> objset,
					  const IOObj& out, const char* nm,
					  ObjectSet<IOPar>* iopset,
					  const uiString& msg, int compnr )
    : SeisSingleTraceProc(out,out,nm,nullptr,msg,compnr)
{
    if ( objset.isEmpty() )
    {
	errmsg_ = tr("No input specified");
	return;
    }

    for ( int idx=0; idx<objset.size(); idx++ )
    {
	IOPar* iop = iopset && iopset->size() > idx ? (*iopset)[idx] : nullptr;
	inpsetup_.ioobj( *objset[idx] );
	if ( iop )
	    inpsetup_.usePar( *iop );

	addReader();
    }
}


bool SeisSingleTraceProc::setInput( const IOObj& in, const IOObj& out,
			const char* nm, const IOPar* iop, const uiString& msg )
{
    inpsetup_.ioobj( in );
    outsetup_.ioobj( out );
    initmsg_ = msg;
    execnm_.set( nm );
    setName( execnm_ );
    if ( iop )
    {
	inpsetup_.usePar( *iop );
	outsetup_.usePar( *iop );
    }

    return setInput();
}


bool SeisSingleTraceProc::addReader( const IOObj& ioobj, const IOPar* iop )
{
    inpsetup_.ioobj( ioobj );
    if ( iop )
	inpsetup_.usePar( *iop );

    return addReader();
}
