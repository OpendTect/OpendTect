/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tutseistools.h"

#include "ioobj.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seiswrite.h"
#include "trckeyzsampling.h"
#include "uistrings.h"



Tut::SeisTools::SeisTools()
    : Executor("Tutorial tools: Direct Seismic")
    , inioobj_(0), outioobj_(0)
    , rdr_(0), wrr_(0)
    , trcin_(*new SeisTrc)
    , trcout_(*new SeisTrc)
{
    clear();
}


Tut::SeisTools::~SeisTools()
{
    clear();
    delete &trcin_;
    delete &trcout_;
}


void Tut::SeisTools::clear()
{
    delete inioobj_; inioobj_ = 0;
    delete outioobj_; outioobj_ = 0;
    delete rdr_; rdr_ = 0;
    delete wrr_; wrr_ = 0;

    action_ = Scale;
    factor_ = 1; shift_ = 0;
    newsd_.start = 0; newsd_.step = 1;
    weaksmooth_ = false;
    totnr_ = -1; nrdone_ = 0;
}


void Tut::SeisTools::setInput( const IOObj& ioobj )
{ delete inioobj_; inioobj_ = ioobj.clone(); }

void Tut::SeisTools::setOutput( const IOObj& ioobj )
{ delete outioobj_; outioobj_ = ioobj.clone(); }

void Tut::SeisTools::setRange( const TrcKeyZSampling& cs )
{ tkzs_ = cs; }


uiString Tut::SeisTools::uiMessage() const
{
    return errmsg_.isEmpty() ? uiStrings::sProcessing() : errmsg_;
}


od_int64 Tut::SeisTools::totalNr() const
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
    if ( !inioobj_ )
    {
	errmsg_ = uiStrings::phrCannotFindObjInDB();
	return false;
    }

    const Seis::GeomType gt = Seis::geomTypeOf( tkzs_.is2D(), false );
    const Pos::GeomID gid = tkzs_.hsamp_.getGeomID();
    rdr_ = new SeisTrcReader( *inioobj_, gid, &gt );
    PtrMan<Seis::SelData> sd = new Seis::RangeSelData( tkzs_ );
    if ( sd && !sd->isAll() )
	rdr_->setSelData( sd.release() );

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
    if ( !outioobj_ )
    {
	errmsg_ = uiStrings::phrCannotFindObjInDB();
	return false;
    }

    const Seis::GeomType gt = rdr_->geomType();
    const Pos::GeomID gid = tkzs_.hsamp_.getGeomID();
    wrr_ = new SeisTrcWriter( *outioobj_, gid, &gt );
    if ( !wrr_->prepareWork(trcout_) )
    {
	errmsg_ = wrr_->errMsg();
	return false;
    }

    return true;
}


int Tut::SeisTools::nextStep()
{
    if ( !rdr_ )
	return createReader() ? MoreToDo()
			      : ErrorOccurred();

    int rv = rdr_->get( trcin_.info() );
    if ( rv < 0 )
    {
	errmsg_ = rdr_->errMsg();
	return ErrorOccurred();
    }
    else if ( rv == 0 )
	return Finished();
    else if ( rv == 1 )
    {
	if ( !rdr_->get(trcin_) )
	{
	    errmsg_ = rdr_->errMsg();
	    return ErrorOccurred();
	}

	trcout_ = trcin_;
	handleTrace();

	if ( !wrr_ && !createWriter() )
	    return ErrorOccurred();
	if ( !wrr_->put(trcout_) )
	    { errmsg_ = wrr_->errMsg(); return ErrorOccurred(); }
    }

    return MoreToDo();
}


void Tut::SeisTools::handleTrace()
{
    switch ( action_ )
    {

    case Scale: {
	SeisTrcPropChg stpc( trcout_ );
	stpc.scale( factor_, shift_ );
    } break;

    case Square: {
	for ( int icomp=0; icomp<trcin_.nrComponents(); icomp++ )
	{
	    for ( int idx=0; idx<trcin_.size(); idx++ )
	    {
		const float v = trcin_.get( idx, icomp );
		trcout_.set( idx, v*v, icomp );
	    }
	}
    } break;

    case Smooth: {
	const int sgate = weaksmooth_ ? 3 : 5;
	const int sgate2 = sgate/2;
	for ( int icomp=0; icomp<trcin_.nrComponents(); icomp++ )
	{
	    for ( int idx=0; idx<trcin_.size(); idx++ )
	    {
	        float sum = 0;
		int count = 0;
		for( int ismp=idx-sgate2; ismp<=idx+sgate2; ismp++)
		{
		    const float val = trcin_.get( ismp, icomp );
		    if ( !mIsUdf(val) )
		    {
			sum += val;
			count++;
		    }
		}
		if ( count )
		    trcout_.set( idx, sum/count, icomp );
	    }
	}

    } break;
    case ChgSD: {
	trcout_.info().sampling = newsd_;
    } break;

    }

    nrdone_++;
}
