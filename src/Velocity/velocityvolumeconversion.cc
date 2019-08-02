/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Dec 2009
________________________________________________________________________

-*/


#include "velocityvolumeconversion.h"

#include "binnedvalueset.h"
#include "ioobj.h"
#include "ioobjtags.h"
#include "seisprovider.h"
#include "seisrangeseldata.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seisstorer.h"
#include "seispacketinfo.h"
#include "sorting.h"
#include "uistrings.h"
#include "varlenarray.h"
#include "velocitycalc.h"

namespace Vel
{

const char* VolumeConverter::sKeyInput() { return sKey::Input(); }
const char* VolumeConverter::sKeyOutput() { return sKey::Output(); }

VolumeConverter::VolumeConverter( const IOObj& input, const IOObj& output,
				  const TrcKeySampling& ranges,
				  const VelocityDesc& desc )
    : tks_( ranges )
    , veloutpdesc_( desc )
    , input_( input.clone() )
    , output_( output.clone() )
    , provider_( 0 )
    , storer_( 0 )
    , sequentialstorer_(0)
{
    uiRetVal uirv;
    provider_ = Seis::Provider::create( *input_, &uirv );
    if ( !provider_ )
	{ errmsg_ = uirv; return; }

    totalnr_ = provider_->totalNr();
    provider_->setSelData( new Seis::RangeSelData(tks_) );
}


VolumeConverter::~VolumeConverter()
{
    delete input_;
    delete output_;
    delete storer_;
    delete sequentialstorer_;
    delete provider_;
}

bool VolumeConverter::doFinish( bool res )
{
    deleteAndZeroPtr(  provider_ );

    if ( !sequentialstorer_->finishWrite() )
	res = false;

    deleteAndZeroPtr( sequentialstorer_ );
    deleteAndZeroPtr( storer_ );
    return res;
}


bool VolumeConverter::doPrepare( int nrthreads )
{
    if ( !errmsg_.isEmpty() )
	return false;

    if ( !input_ || !output_ )
    {
	errmsg_ = tr("Either input or output cannot be found");
	return false;
    }

    delete storer_;
    storer_ = 0;

    if ( !GetVelocityTag( *input_, velinpdesc_ ) )
    {
	errmsg_ = tr("Cannot read velocity information on input.");
	return false;
    }

    if ( !SetVelocityTag( *output_, veloutpdesc_ ) )
    {
	errmsg_ = tr("Cannot write velocity information on output");
	return false;
    }

    if ( ( velinpdesc_.type_ != VelocityDesc::Interval &&
	 velinpdesc_.type_ != VelocityDesc::RMS &&
	 velinpdesc_.type_ != VelocityDesc::Avg ) ||
         ( veloutpdesc_.type_ != VelocityDesc::Interval &&
	 veloutpdesc_.type_ != VelocityDesc::RMS &&
	 veloutpdesc_.type_ != VelocityDesc::Avg ) ||
	 velinpdesc_.type_ == veloutpdesc_.type_ )
    {
	errmsg_ = tr("Input/output velocities are not interval, RMS, or Avg "
	             "or are identical.");
	return false;
    }

    //Check input Vel

    if ( !provider_ )
    {
	uiRetVal uirv;
	provider_ = Seis::Provider::create( *input_, &uirv );
	if ( !provider_ )
	    { errmsg_ = uirv; return false; }

	provider_->setSelData( new Seis::RangeSelData(tks_) );
    }

    storer_ = new Seis::Storer( *output_ );
    sequentialstorer_ = new Seis::SequentialStorer( *storer_ );

    return true;
}


bool VolumeConverter::doWork( od_int64, od_int64, int threadidx )
{
    char res = 1;
    SeisTrc trc;

    lock_.lock();
    res = getNewTrace( trc, threadidx );
    lock_.unLock();

    ArrPtrMan<float> owndata = 0;
    SeisTrcValueSeries inputvs( trc, 0 );
    const float* inputptr = inputvs.arr();
    if ( !inputptr )
    {
	owndata = new float[trc.size()];
	inputptr = owndata.ptr();
    }

    const SamplingData<double> sd =
			       getDoubleSamplingData( trc.info().sampling_ );
    const int trcsz = trc.size();
    TypeSet<double> timevals( trcsz, 0.f );
    for ( int idx=0; idx<trc.size(); idx++ )
	timevals[idx] = sd.atIndex( idx );
    const double* timesamps = timevals.arr();

    while ( res==1 )
    {
	if ( !shouldContinue() )
	    return false;

	if ( owndata )
	{
	    for ( int idx=0; idx<trc.size(); idx++ )
		owndata[idx] = inputvs.value( idx );
	}

	SeisTrc* outputtrc = new SeisTrc( trc );
	float* outptr = (float*) outputtrc->data().getComponent( 0 )->data();
	float* interptr = 0;
	if ( velinpdesc_.type_ != VelocityDesc::Interval )
	{
	    if ( veloutpdesc_ != VelocityDesc::Interval )
	    {
		mTryAlloc(interptr,float[trcsz]);
		if ( !interptr )
		{
		    errmsg_ = tr("Not enough memory");
		    deleteAndZeroPtr( outputtrc );
		    return false;
		}
	    }
	    float* targetptr = interptr ? interptr : outptr;
	    if ( (velinpdesc_.type_ == VelocityDesc::Avg &&
		  !computeVint(inputptr,timesamps,trcsz,targetptr) ) ||
		 (velinpdesc_.type_ == VelocityDesc::RMS &&
		  !computeDix(inputptr,sd,trcsz,targetptr) ) )
	    {
		deleteAndZeroPtr( outputtrc );
	    }
	}

	const float* srcptr = interptr ? interptr : inputptr;
	if ( (veloutpdesc_.type_ == VelocityDesc::Avg &&
	      !computeVavg(srcptr,timesamps,trcsz,outptr) ) ||
	     (veloutpdesc_.type_ == VelocityDesc::RMS &&
	      !computeVrms(srcptr,sd,trcsz,outptr) ) )
	{
	    deleteAndZeroPtr( outputtrc );
	}

	delete [] interptr;

	//Process trace
	sequentialstorer_->submitTrace( outputtrc, true );
	addToNrDone( 1 );

	Threads::MutexLocker lock( lock_ );
	res = getNewTrace( trc, threadidx );
    }

    return res==0;
}


char VolumeConverter::getNewTrace( SeisTrc& trc, int threadidx )
{
    if ( !provider_ )
	return 0;

    const uiRetVal uirv = provider_->getNext( trc );
    if ( !uirv.isOK() )
    {
	if ( isFinished(uirv) )
	    return 0;

	errmsg_ = uirv;
	return -1;
    }

    sequentialstorer_->announceTrace( trc.info().binID() );

    delete provider_;
    provider_ = 0;

    return 1;
}

} // namespace Vel
