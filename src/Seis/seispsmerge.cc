/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R. K. Singh
 * DATE     : Oct 2007
-*/

static const char* rcsID = "$Id: seispsmerge.cc,v 1.5 2008-01-15 16:19:43 cvsbert Exp $";

#include "segposinfo.h"
#include "seisbuf.h"
#include "seispsioprov.h"
#include "seispsmerge.h"
#include "seispsread.h"
#include "seispswrite.h"
#include "seistrc.h"
#include "ioobj.h"


SeisPSMerger::SeisPSMerger( ObjectSet<IOObj> objset, const IOObj* out,
       			    const HorSampling& subsel ) 
  	: Executor("Pre-Stack data Merger")
	, inobjs_(objset)
	, outobj_(out)
	, writer_(0)
	, msg_("Nothing")
	, totnr_(-1)
	, nrdone_(0)
{
    if ( inobjs_.isEmpty() )
    {
	msg_ = "No input specified";
	return;
    }

    nrobjs_ = inobjs_.size();
    init( subsel );
}


SeisPSMerger::~SeisPSMerger()
{
    delete iter_;
    delete writer_;
    delete outobj_;
    deepErase( inobjs_ );
    deepErase( readers_ );
}


void SeisPSMerger::init( const HorSampling& subsel )
{
    totnr_ = 0;
    HorSampling hs;
    const bool nosubsel = hs == subsel;
    for ( int idx=0; idx<nrobjs_; idx++ )
    {
	SeisPS3DReader* rdr = SPSIOPF().getReader( *inobjs_[idx] );
	if ( !rdr ) continue;

	StepInterval<int> inlrg;
	StepInterval<int> crlrg;
	rdr->posData().getInlRange( inlrg );
	rdr->posData().getCrlRange( crlrg );
	if ( !idx ) hs.set( (Interval<int>)inlrg, (Interval<int>)crlrg );
	else
	{
	    BinID start( inlrg.start, crlrg.start );
    	    BinID stop( inlrg.stop, crlrg.stop );
    	    hs.include( start ); hs.include( stop );
	}

	readers_ += rdr;
    }

    totnr_ = nosubsel ? hs.totalNr() : subsel.totalNr();
    iter_ = nosubsel ? new HorSamplingIterator( hs )
		     : new HorSamplingIterator( subsel );
    writer_ = SPSIOPF().getWriter( *outobj_ );
}


const char* SeisPSMerger::message() const
{
    const char* msg = msg_.buf();
    return msg;
}


const char* SeisPSMerger::nrDoneText() const
{
    return "Traces written";
}


int SeisPSMerger::nrDone() const
{
    return nrdone_;
}


int SeisPSMerger::totalNr() const
{
    return totnr_;
}


int SeisPSMerger::nextStep()
{
    const int ret = doNextPos();
    if ( ret == 0 ) return Executor::Finished;
    if ( ret == -1 ) return Executor::ErrorOccurred;

    nrdone_ ++;
    return Executor::MoreToDo;
}


int SeisPSMerger::doNextPos()
{
    if ( !iter_->next(curbid_) )
	return 0;

    for ( int idx=0; idx<readers_.size(); idx++ )
    {
	SeisTrcBuf* trcbuf = new SeisTrcBuf(true);
	if ( !readers_[idx]->getGather(curbid_, *trcbuf) )
	{
	    delete trcbuf;
	    continue;
	}

	for ( int tdx=0; tdx<trcbuf->size(); tdx++ )
	{
	    SeisTrc* trc = trcbuf->get( tdx );
	    if ( !writer_->put(*trc) )
		return -1;
	}

	delete trcbuf;
	break;
    }

    return 1;
}

