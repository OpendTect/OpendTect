/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2001
-*/

static const char* rcsID = "$Id: seissingtrcproc.cc,v 1.1 2001-10-17 09:00:06 bert Exp $";

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


SeisSingleTraceProc::SeisSingleTraceProc( const IOObj* in, const IOObj* out,
				       	  const char* nm, const IOPar* iop,
				       	  const char* msg )
	: Executor(nm)
	, rdr_(in ? new SeisTrcReader(in) : 0)
	, wrr_(out ? new SeisTrcWriter(out) : 0)
	, msg_(msg)
	, starter_(0)
	, nrskipped_(0)
	, intrc_(new SeisTrc)
	, nrwr_(0)
	, wrrkey_(*new MultiID)
	, totnr_(-1)
{
    outtrc_ = intrc_;

    if ( !rdr_ || !wrr_ )
    {
	curmsg_ = rdr_ ? "Cannot find write object" : "Cannot find read object";
        return;
    }

    if ( iop )
    {
	rdr_->usePar( *iop );
	if ( rdr_->trcSel() )
	{
	    if ( rdr_->trcSel()->bidsel )
	    {
		BinIDProvider* prov = rdr_->trcSel()->bidsel->provider();
		totnr_ = prov->size();
		delete prov;
	    }
	    else
	    {
		int nr = rdr_->trcSel()->intv.width();
		if ( nr < 10000000 ) totnr_ = nr + 1;
	    }
	}
    }
    starter_ = rdr_->starter();
    wrrkey_ = out->key();
}


SeisSingleTraceProc::~SeisSingleTraceProc()
{
    delete rdr_;
    delete wrr_;
    delete starter_;
    delete intrc_;
    delete &wrrkey_;
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


int SeisSingleTraceProc::nextStep()
{
    if ( !rdr_ || !wrr_ )
	return -1;
    else if ( starter_ )
    {
	int rv = starter_->nextStep();
	if ( rv ) return rv;
	delete starter_;
	starter_ = 0;
	curmsg_ = msg_;
	return 1;
    }

    int rv = rdr_->get( intrc_->info() );
    if ( !rv )
	{ wrapUp(); return 0; }
    else if ( rv < 0 )
	{ curmsg_ = rdr_->errMsg(); return -1; }
    else if ( rv == 2 )
	return 1;

    skipcurtrc_ = false;
    selcb_.doCall( this );
    if ( skipcurtrc_ ) { nrskipped_++; return 1; }

    if ( !rdr_->get(*intrc_) )
	{ curmsg_ = rdr_->errMsg(); return -1; }
    proccb_.doCall( this );
    if ( skipcurtrc_ ) { nrskipped_++; return 1; }

    //TODO do explicit execute of writer starter
    if ( !wrr_->put(*outtrc_) )
	{ curmsg_ = wrr_->errMsg(); return -1; }

    nrwr_++;
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

}
