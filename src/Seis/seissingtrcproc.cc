/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2001
-*/

static const char* rcsID = "$Id: seissingtrcproc.cc,v 1.12 2003-12-10 14:09:09 bert Exp $";

#include "seissingtrcproc.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "multiid.h"
#include "ioobj.h"
#include "iopar.h"
#include "ioman.h"
#include "binidsel.h"
#include "scaler.h"


SeisSingleTraceProc::SeisSingleTraceProc( const IOObj* in, const IOObj* out,
				       	  const char* nm, const IOPar* iop,
				       	  const char* msg )
	: Executor(nm)
	, wrr_(out ? new SeisTrcWriter(out) : 0)
	, msg_(msg)
	, starter_(0)
	, nrskipped_(0)
	, intrc_(new SeisTrc)
	, nrwr_(0)
	, wrrkey_(*new MultiID)
	, totnr_(-1)
    	, trcsperstep_(10)
    	, scaler_(0)
    	, skipnull_(false)
{
    outtrc_ = intrc_;

    if ( !wrr_ )
    {
	curmsg_ = "Cannot find write object";
        return;
    }

    if ( !in )
    {
	curmsg_ = "No input specified";
	return;
    }

    wrrkey_ = out->key();
    currentobj_ = 0;
    nrobjs_ = 1;

    ObjectSet<IOObj> objset_;
    ObjectSet<IOPar> iopset_;
    objset_ += in->clone();
    iopset_ += const_cast<IOPar*>(iop);
    if ( !init( objset_, iopset_ ) )
	return;

    nextObj();
}


SeisSingleTraceProc::SeisSingleTraceProc( ObjectSet<IOObj> objset, 
					  const IOObj* out, const char* nm, 
					  ObjectSet<IOPar>* iopset, 
					  const char* msg )
	: Executor(nm)
	, wrr_(out ? new SeisTrcWriter(out) : 0)
	, msg_(msg)
	, starter_(0)
	, nrskipped_(0)
	, intrc_(new SeisTrc)
	, nrwr_(0)
	, wrrkey_(*new MultiID)
	, totnr_(-1)
    	, trcsperstep_(10)
    	, scaler_(0)
    	, skipnull_(false)
{
    outtrc_ = intrc_;

    if ( !wrr_ )
    {
	curmsg_ = "Cannot find write object";
        return;
    }

    if ( !objset.size() )
    {
	curmsg_ = "No input specified";
	return;
    }

    wrrkey_ = out->key();
    nrobjs_ = objset.size();
    currentobj_ = 0;

    if ( iopset && iopset->size() != nrobjs_ )
	iopset->erase();

    if ( !init( objset, *iopset ) )
	return;

    nextObj();
}


SeisSingleTraceProc::~SeisSingleTraceProc()
{
    delete wrr_;
    delete starter_;
    delete intrc_;
    delete &wrrkey_;
    delete scaler_;
    deepErase( rdrset_ );
}


bool SeisSingleTraceProc::init( ObjectSet<IOObj>& os, ObjectSet<IOPar>& is )
{
    totnr_ = 0;
    for ( int idx=0; idx<nrobjs_; idx++ )
    {
	SeisTrcReader* rdr_ = new SeisTrcReader( os[idx] );
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

	if ( is.size() && is[idx] )
	{
	    rdr_->usePar( *is[idx] );
	    if ( rdr_->trcSel() )
	    {
		if ( rdr_->trcSel()->bidsel )
		{
		    BinIDProvider* prov = rdr_->trcSel()->bidsel->provider();
		    totnr_ += prov->size();
		    delete prov;
		}
		else
		{
		    int nr = rdr_->trcSel()->intv.width();
		    if ( nr < 10000000 ) totnr_ = nr + 1;
		}
	    }
	}
	
	rdrset_ += rdr_;
    }

    if ( totnr_ < 3 ) totnr_ = -1;
    return true;
}


void SeisSingleTraceProc::nextObj()
{
    if ( starter_ )
	{ delete starter_; starter_ = 0; }
    starter_ = rdrset_[currentobj_]->starter();

    return;
}


void SeisSingleTraceProc::setScaler( Scaler* newsclr )
{
    if ( newsclr == scaler_ ) return;
    delete scaler_; scaler_ = newsclr;
    for ( int idx=0; idx<rdrset_.size(); idx++ )
	rdrset_[idx]->forceFloatData( scaler_ ? true : false );
}


const char* SeisSingleTraceProc::message() const
{
    return starter_ ? starter_->message() : (const char*)curmsg_;
}


int SeisSingleTraceProc::nrDone() const
{
    return starter_ ? starter_->nrDone() : nrwr_;
}


const char* SeisSingleTraceProc::nrDoneText() const
{
    return starter_ ? starter_->nrDoneText() : "Traces written";
}


int SeisSingleTraceProc::totalNr() const
{
    if ( starter_ ) return starter_->totalNr();
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

    else if ( starter_ )
    {
	int rv = starter_->doStep();
	if ( rv ) return rv;
	delete starter_;
	starter_ = 0;
	curmsg_ = msg_;
	return 1;
    }

    int retval = 1;
    for ( int idx=0; idx<trcsperstep_; idx++ )
    {
	int rv = rdrset_[currentobj_]->get( intrc_->info() );
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

	skipcurtrc_ = false;
	selcb_.doCall( this );
	if ( skipcurtrc_ ) { nrskipped_++; continue; }

	if ( !rdrset_[currentobj_]->get(*intrc_) )
	    { curmsg_ = rdrset_[currentobj_]->errMsg(); return -1; }

	if ( skipnull_ )
	{
	    const int nrcomps = intrc_->data().nrComponents();
	    for ( int icomp=0; icomp<nrcomps; icomp++ )
	    {
		if ( !intrc_->isNull(icomp) )
		    break;
		if ( icomp == nrcomps-1 )
		    skipcurtrc_ = true;
	    }
	}

	if ( !skipcurtrc_ )
	    proccb_.doCall( this );
	if ( skipcurtrc_ ) { nrskipped_++; continue; }

	if ( scaler_ )
	    scaleTrc( *const_cast<SeisTrc*>(outtrc_), *scaler_ );

	if ( !wrr_->put(*outtrc_) )
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
