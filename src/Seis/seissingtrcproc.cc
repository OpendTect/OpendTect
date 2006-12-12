/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2001
-*/

static const char* rcsID = "$Id: seissingtrcproc.cc,v 1.36 2006-12-12 17:47:28 cvsbert Exp $";

#include "seissingtrcproc.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seispsioprov.h"
#include "seispswrite.h"
#include "seis2dline.h"
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
	, wrr_(0) \
	, pswrr_(0) \
	, msg_(msg) \
	, nrskipped_(0) \
	, intrc_(*new SeisTrc) \
	, nrwr_(0) \
	, wrrkey_(*new MultiID) \
	, totnr_(-1) \
    	, trcsperstep_(10) \
    	, scaler_(0) \
    	, skipnull_(false) \
    	, resampler_(0) \
    	, is3d_(true) \
{ \
    if ( !mkWriter(out) ) return


SeisSingleTraceProc::SeisSingleTraceProc( const SeisSelection& in,
					  const IOObj* out, const char* nm,
				       	  const char* msg )
    mInitVars();

    PtrMan<IOObj> inioobj = IOM().get( in.key_ );
    IOPar iop; in.fillPar( iop );

    setInput( inioobj, out, nm, &iop, msg );
}


SeisSingleTraceProc::SeisSingleTraceProc( const IOObj* in, const IOObj* out,
				       	  const char* nm, const IOPar* iop,
				       	  const char* msg )
    mInitVars();

    setInput( in, out, nm, iop, msg );
}


SeisSingleTraceProc::SeisSingleTraceProc( ObjectSet<IOObj> objset, 
					  const IOObj* out, const char* nm, 
					  ObjectSet<IOPar>* iopset, 
					  const char* msg )
    mInitVars();

    if ( objset.isEmpty() )
    {
	curmsg_ = "No input specified";
	return;
    }

    nrobjs_ = objset.size();

    if ( iopset && iopset->size() != nrobjs_ )
	iopset->erase();

    init( objset, *iopset );
}


bool SeisSingleTraceProc::mkWriter( const IOObj* out )
{
    if ( !out ) return false;

    if ( strcmp(out->group(),mTranslGroupName(SeisPS)) )
    {
	wrr_ = new SeisTrcWriter( out );
	return true;
    }

    BufferString dstyp = "CBVS"; out->pars().get( "Data Store Type", dstyp );
    const SeisPSIOProvider* siop = SPSIOPF().provider( dstyp );
    if ( !siop )
    {
	curmsg_ = "Pre-Stack data store type '";
	curmsg_ += dstyp;
	curmsg_ += "' not found";
	return false;
    }

    BufferString dsimpl( out->fullUserExpr(false) );
    pswrr_ = siop->makeWriter( dsimpl );
    if ( !pswrr_ )
    {
	curmsg_ = "Cannot make PreStack writer for '";
	curmsg_ += dsimpl;
	curmsg_ += "'";
	return false;
    }

    pswrr_->usePar( out->pars() );
    return true;
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
    delete pswrr_;
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
    if ( wrr_ )
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
	const bool is3d = !rdr_->is2D();
	if ( wrr_ && is3d && wrr_->ioObj()->key() == rdr_->ioObj()->key() )
	{
	    curmsg_ = "Input and output are the same.";
	    delete rdr_;
	    return false;
	}

	bool szdone = false;
	if ( iops.size() > idx && iops[idx] )
	{
	    rdr_->usePar( *iops[idx] );
	    if ( rdr_->selData() )
	    {
		totnr_ = rdr_->selData()->expectedNrTraces( !is3d );
		szdone = true;
	    }
	}
	if ( is3d && !pswrr_ && !szdone )
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
	is3d_ = is3d;
    }

    if ( !allszsfound || totnr_ < 3 )
	totnr_ = -1;

    nextObj();
    return true;
}


void SeisSingleTraceProc::nextObj()
{
    SeisTrcReader* currdr = rdrset_[currentobj_];
    currdr->prepareWork();
    if ( wrr_ )
	wrr_->setLineKeyProvider( currdr->lineKeyProvider() );
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
    if ( resampler_ )
	resampler_->set2D( !is3d_ );
}


const char* SeisSingleTraceProc::message() const
{
    const char* msg = curmsg_.buf();
    if ( !*msg && currentobj_ < rdrset_.size() )
    {
	const SeisTrcReader* currdr = rdrset_[currentobj_];
	static BufferString ret;
	ret = "Handling ";
	if ( !currdr->is2D() )
	    ret += "data";
	else
	{
	    LineKey lk( currdr->lineKey() );
	    if ( lk.isEmpty() )
		ret += "data";
	    else
		{ ret += "'"; ret += lk; ret += "'"; }
	}
	msg = ret.buf();
    }

    return msg;
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
	const int sz = trc.size();
	for ( int isamp=0; isamp<sz; isamp++ )
	    trc.set( isamp, sclr.scale(trc.get(isamp,icomp)), icomp );
    }
}


int SeisSingleTraceProc::nextStep()
{
    if ( rdrset_.size() <= currentobj_ || !rdrset_[currentobj_]
	|| (!wrr_ && !pswrr_) )
	return -1;

    int retval = 1;
    for ( int idx=0; idx<trcsperstep_; idx++ )
    {
	SeisTrcReader* currdr = rdrset_[currentobj_];
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
	    { curmsg_ = currdr->errMsg(); return -1; }
	else if ( rv == 2 )
	    continue;

	worktrc_ = &intrc_;
	skipcurtrc_ = false;
	selcb_.doCall( this );
	if ( skipcurtrc_ ) { nrskipped_++; continue; }

	if ( !currdr->get(intrc_) )
	    { curmsg_ = currdr->errMsg(); return -1; }

	if ( resampler_ )
	{
	    worktrc_ = resampler_->get(intrc_);
	    if ( !worktrc_ ) { nrskipped_++; continue; }
	}

	if ( skipnull_ && worktrc_->isNull() )
	    skipcurtrc_ = true;

	if ( !skipcurtrc_ )
	    proccb_.doCall( this );
	if ( skipcurtrc_ ) { nrskipped_++; continue; }

	if ( scaler_ )
	    const_cast<SeisTrc*>(worktrc_)->data().scale( *scaler_ );

	if ( nrwr_ < 1 && wrr_ && wrr_->seisTranslator() )
	{
	    wrr_->seisTranslator()->setCurLineKey( currdr->lineKey() );
	    if ( currdr->is2D() )
		wrr_->seisTranslator()->packetInfo().crlrg
		    	= currdr->curTrcNrRange();
	}

	if ( wrr_ && !wrr_->put(*worktrc_) )
	    { curmsg_ = wrr_->errMsg(); return -1; }
	else if ( pswrr_ && !pswrr_->put(*worktrc_) )
	    { curmsg_ = pswrr_->errMsg(); return -1; }

	nrwr_++;
    }
    return 1;
}


void SeisSingleTraceProc::wrapUp()
{
    if ( wrr_ ) wrr_->close();
}
