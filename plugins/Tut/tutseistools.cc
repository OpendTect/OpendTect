
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
-*/

static const char* rcsID = "$Id: tutseistools.cc,v 1.4 2007-05-24 08:32:07 cvsraman Exp $";

#include "tutseistools.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "ioobj.h"


Tut::SeisTools::SeisTools()
    : Executor("Tutorial tools: Direct Seismic")
    , inioobj_(0), outioobj_(0)
    , rdr_(0), wrr_(0)
    , trc_(*new SeisTrc)
{
    clear();
}


Tut::SeisTools::~SeisTools()
{
    clear();
    delete &trc_;
}


void Tut::SeisTools::clear()
{
    delete inioobj_; inioobj_ = 0;
    delete outioobj_; outioobj_ = 0;
    delete rdr_; rdr_ = 0;
    delete wrr_; wrr_ = 0;

    action_ = Scale;
    factor_ = 1; shift_ = 0;
    weaksmooth_ = false;
    totnr_ = -1; nrdone_ = 0;
}


void Tut::SeisTools::setInput( const IOObj& ioobj )
{
    delete inioobj_; inioobj_ = ioobj.clone();
}


void Tut::SeisTools::setOutput( const IOObj& ioobj )
{
    delete outioobj_; outioobj_ = ioobj.clone();
}


const char* Tut::SeisTools::message() const
{
    static const char* acts[] = { "Scaling", "Squaring", "Smoothing" };
    return errmsg_.isEmpty() ? acts[action_] : errmsg_.buf();
}


int Tut::SeisTools::totalNr() const
{
    if ( inioobj_ && totnr_ == -1 )
    {
	totnr_ = -2;
	SeisIOObjInfo ioobjinfo( *inioobj_ );
	SeisIOObjInfo::SpaceInfo spinf;
	if ( ioobjinfo.getDefSpaceInfo(spinf) )
	    totnr_ = spinf.expectednrtrcs;
    }
	
    return totnr_ < 0 ? -1 : totnr_;
}


bool Tut::SeisTools::createReader()
{
    rdr_ = new SeisTrcReader( inioobj_ );
    rdr_->forceFloatData( action_ != Smooth );
    if ( !rdr_->prepareWork() )
    {
	errmsg_ = rdr_->errMsg();
	return false;
    }
    return true;
}


bool Tut::SeisTools::createWriter()
{
    wrr_ = new SeisTrcWriter( outioobj_ );
    if ( !wrr_->prepareWork(trc_) )
    {
	errmsg_ = wrr_->errMsg();
	return false;
    }
    return true;
}


int Tut::SeisTools::nextStep()
{
    if ( !rdr_ )
	return createReader() ? Executor::MoreToDo : Executor::ErrorOccurred;

    int rv = rdr_->get( trc_.info() );
    if ( rv < 0 )
	{ errmsg_ = rdr_->errMsg(); return Executor::ErrorOccurred; }
    else if ( rv == 0 )
	return Executor::Finished;
    else if ( rv == 1 )
    {
	if ( !rdr_->get(trc_) )
	    { errmsg_ = rdr_->errMsg(); return Executor::ErrorOccurred; }

	handleTrace();

	if ( !wrr_ && !createWriter() )
	    return Executor::ErrorOccurred;
	if ( !wrr_->put(trc_) )
	    { errmsg_ = wrr_->errMsg(); return Executor::ErrorOccurred; }
    }

    return Executor::MoreToDo;
}


void Tut::SeisTools::handleTrace()
{
    switch ( action_ )
    {

    case Scale: {
	SeisTrcPropChg stpc( trc_ );
	stpc.scale( factor_, shift_ );
    } break;

    case Square: {
	for ( int icomp=0; icomp<trc_.nrComponents(); icomp++ )
	{
	    for ( int idx=0; idx<trc_.size(); idx++ )
	    {
		const float v = trc_.get( idx, icomp );
		trc_.set( idx, v*v, icomp );
	    }
	}
    } break;

    case Smooth: {
	const int smpgate = weaksmooth_ ? 3 : 5;
	for ( int icomp=0; icomp<trc_.nrComponents(); icomp++ )
	{
	    for ( int idx = smpgate/2; idx < trc_.size() - smpgate/2; idx++ )
	    {
	        float sum = 0;
		for( int ismp = idx-smpgate/2; ismp <= idx+smpgate/2; ismp++)
		    sum += trc_.get( ismp, icomp );
	        trc_.set( idx, sum/smpgate, icomp );
	    }
	}

    } break;

    }

    nrdone_++;
}
