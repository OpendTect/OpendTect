/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2001
-*/

static const char* rcsID = "$Id: seissingtrcproc.cc,v 1.18 2004-08-24 16:24:57 bert Exp $";

#include "seissingtrcproc.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "seisresampler.h"
#include "cubesampling.h"
#include "multiid.h"
#include "ioobj.h"
#include "iopar.h"
#include "ioman.h"
#include "scaler.h"
#include "ptrman.h"


#define mInitVars() \
	: Executor(nm) \
	, wrr_(out ? new SeisTrcWriter(out) : 0) \
	, msg_(msg) \
	, nrskipped_(0) \
	, intrc_(*new SeisTrc) \
	, nrwr_(0) \
	, wrrkey_(*new MultiID) \
	, totnr_(-1) \
    	, trcsperstep_(10) \
    	, scaler_(0) \
    	, skipnull_(false) \
    	, resampler_(0)


SeisSingleTraceProc::SeisSingleTraceProc( const SeisSelection& in,
					  const IOObj* out, const char* nm,
				       	  const char* msg )
	mInitVars()
{
    PtrMan<IOObj> inioobj = IOM().get( in.key_ );
    IOPar iop; in.fillPar( iop );
    setInput( inioobj, out, nm, &iop, msg );
}


SeisSingleTraceProc::SeisSingleTraceProc( const IOObj* in, const IOObj* out,
				       	  const char* nm, const IOPar* iop,
				       	  const char* msg )
	mInitVars()
{
    setInput( in, out, nm, iop, msg );
}


SeisSingleTraceProc::SeisSingleTraceProc( ObjectSet<IOObj> objset, 
					  const IOObj* out, const char* nm, 
					  ObjectSet<IOPar>* iopset, 
					  const char* msg )
	mInitVars()
{
    if ( !objset.size() )
    {
	curmsg_ = "No input specified";
	return;
    }

    nrobjs_ = objset.size();

    if ( iopset && iopset->size() != nrobjs_ )
	iopset->erase();

    init( objset, *iopset );
}


void SeisSingleTraceProc::setInput( const IOObj* in, const IOObj* out,
				    const char* nm, const IOPar* iop,
				    const char* msg )
{
    deepErase( rdrset_ );
    if ( !in )
    {
	curmsg_ = "Cannot find input seismic data object";
	return;
    }

    nrobjs_ = 1;

    ObjectSet<IOObj> objset_;
    ObjectSet<IOPar> iopset_;
    objset_ += in->clone();
    iopset_ += const_cast<IOPar*>(iop);
    init( objset_, iopset_ );
}


SeisSingleTraceProc::~SeisSingleTraceProc()
{
    delete wrr_;
    delete &intrc_;
    delete &wrrkey_;
    delete scaler_;
    delete resampler_;
    deepErase( rdrset_ );
}


bool SeisSingleTraceProc::init( ObjectSet<IOObj>& ioobjs,
				ObjectSet<IOPar>& iops )
{
    worktrc_ = &intrc_;
    if ( !wrr_ )
    {
	curmsg_ = "Cannot find write object";
        return false;
    }
    wrrkey_ = wrr_->ioObj()->key();
    currentobj_ = 0;

    totnr_ = 0;
    bool allszsfound = true;
    for ( int idx=0; idx<nrobjs_; idx++ )
    {
	SeisTrcReader* rdr_ = new SeisTrcReader( ioobjs[idx] );
	if ( !rdr_->ioObj() )
	{
	    curmsg_ = "Cannot find read object";
	    delete rdr_;
	    return false;
	}
	if ( wrr_->ioObj()->key() == rdr_->ioObj()->key() )
	{
	    curmsg_ = "Input and output are the same.";
	    delete rdr_;
	    return false;
	}

	const bool is3d = !SeisTrcTranslator::is2D( *rdr_->ioObj() );
	bool szdone = false;
	if ( iops.size() > idx && iops[idx] )
	{
	    rdr_->usePar( *iops[idx] );
	    if ( is3d && rdr_->selData() )
	    {
		totnr_ = rdr_->selData()->expectedNrTraces();
		szdone = true;
	    }
	}
	if ( is3d && !szdone )
	{
	    CubeSampling cs;
	    if ( SeisTrcTranslator::getRanges(*ioobjs[idx],cs) )
	    {
		totnr_ += cs.nrInl() * cs.nrCrl();
		szdone = true;
	    }
	}

	if ( !szdone )
	    allszsfound = false;
	rdrset_ += rdr_;
    }

    if ( !allszsfound || totnr_ < 3 ) totnr_ = -1;

    nextObj();
    return true;
}


void SeisSingleTraceProc::nextObj()
{
    rdrset_[currentobj_]->prepareWork();
    return;
}


void SeisSingleTraceProc::setScaler( Scaler* newsclr )
{
    if ( newsclr == scaler_ ) return;
    delete scaler_; scaler_ = newsclr;
    for ( int idx=0; idx<rdrset_.size(); idx++ )
	rdrset_[idx]->forceFloatData( scaler_ ? true : false );
}


void SeisSingleTraceProc::setResampler( SeisResampler* r )
{
    delete resampler_; resampler_ = r;
}


const char* SeisSingleTraceProc::message() const
{
    return (const char*)curmsg_;
}


int SeisSingleTraceProc::nrDone() const
{
    return nrwr_;
}


const char* SeisSingleTraceProc::nrDoneText() const
{
    return "Traces written";
}


int SeisSingleTraceProc::totalNr() const
{
    return totnr_-nrskipped_ < 0 ? -1 : totnr_-nrskipped_;
}


static void scaleTrc( SeisTrc& trc, Scaler& sclr )
{
    for ( int icomp=0; icomp<trc.data().nrComponents(); icomp++ )
    {
	const int sz = trc.size( icomp );
	for ( int isamp=0; isamp<sz; isamp++ )
	    trc.set( isamp, sclr.scale(trc.get(isamp,icomp)), icomp );
    }
}


int SeisSingleTraceProc::nextStep()
{
    if ( rdrset_.size() <= currentobj_ || !rdrset_[currentobj_] || !wrr_ )
	return -1;

    int retval = 1;
    for ( int idx=0; idx<trcsperstep_; idx++ )
    {
	int rv = rdrset_[currentobj_]->get( intrc_.info() );
	if ( !rv )
	{ 
	    currentobj_++;
	    if ( currentobj_ == nrobjs_ )
		{ wrapUp(); return 0; }
	    nextObj();
	    return 1;
	}
	else if ( rv < 0 )
	    { curmsg_ = rdrset_[currentobj_]->errMsg(); return -1; }
	else if ( rv == 2 )
	    continue;

	worktrc_ = &intrc_;
	skipcurtrc_ = false;
	selcb_.doCall( this );
	if ( skipcurtrc_ ) { nrskipped_++; continue; }

	if ( !rdrset_[currentobj_]->get(intrc_) )
	    { curmsg_ = rdrset_[currentobj_]->errMsg(); return -1; }

	if ( resampler_ )
	    worktrc_ = resampler_->get(intrc_);

	if ( skipnull_ && worktrc_->isNull() )
	    skipcurtrc_ = true;

	if ( !skipcurtrc_ )
	    proccb_.doCall( this );
	if ( skipcurtrc_ ) { nrskipped_++; continue; }

	if ( scaler_ )
	    scaleTrc( *const_cast<SeisTrc*>(worktrc_), *scaler_ );

	if ( !wrr_->put(*worktrc_) )
	    { curmsg_ = wrr_->errMsg(); return -1; }

	nrwr_++;
    }
    return 1;
}


void SeisSingleTraceProc::wrapUp()
{
    IOPar* iopar = IOM().getAux( wrrkey_ );
    if ( iopar )
    {
	wrr_->fillAuxPar( *iopar );
	IOM().putAux( wrrkey_, iopar );
	delete iopar;
    }
    wrr_->close();
}
