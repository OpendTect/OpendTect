/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
-*/


#include "trckeyzsampling.h"
#include "tutseistools.h"
#include "seisprovider.h"
#include "seiswrite.h"
#include "seisioobjinfo.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "ioobj.h"
#include "bufstring.h"



Tut::SeisTools::SeisTools()
    : Executor("Tutorial tools: Direct Seismic")
    , inioobj_(0), outioobj_(0)
    , prov_(0), wrr_(0)
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
    delete prov_; prov_ = 0;
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


uiString Tut::SeisTools::message() const
{
    static uiString acts[] = { tr("Scaling"), tr("Squaring"), tr("Smoothing"),
				  tr("Changing") };
    return errmsg_.isEmpty() ? acts[action_] : errmsg_;
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


bool Tut::SeisTools::createProvider()
{
    uiRetVal uirv;
    prov_ = Seis::Provider::create( inioobj_->key(), &uirv );
    if ( !prov_ )
	{ errmsg_ = uirv; return false; }

    Seis::RangeSelData* sd = new Seis::RangeSelData( tkzs_ );
    prov_->setSelData( sd );
    prov_->forceFPData( action_ != Smooth );
    return true;
}


bool Tut::SeisTools::createWriter()
{
    wrr_ = new SeisTrcWriter( outioobj_ );
    if ( !wrr_->prepareWork(trcout_) )
    {
	errmsg_ = wrr_->errMsg();
	return false;
    }
    return true;
}


int Tut::SeisTools::nextStep()
{
    if ( !prov_ )
	return createProvider() ? Executor::MoreToDo()
				: Executor::ErrorOccurred();

    const uiRetVal uirv = prov_->getNext( trcin_ );
    if ( !uirv.isOK() )
    {
	if ( isFinished(uirv) )
	    return Executor::Finished();

	errmsg_ = uirv;
	return Executor::ErrorOccurred();
    }

    trcout_ = trcin_;
    handleTrace();

    if ( !wrr_ && !createWriter() )
	return Executor::ErrorOccurred();
    if ( !wrr_->put(trcout_) )
	{ errmsg_ = wrr_->errMsg(); return Executor::ErrorOccurred(); }

    return Executor::MoreToDo();
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
	trcout_.info().sampling_ = newsd_;
    } break;

    }

    nrdone_++;
}
