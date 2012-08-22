/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Dec 2009
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: velocityvolumeconversion.cc,v 1.16 2012-08-22 12:37:14 cvshelene Exp $";

#include "velocityvolumeconversion.h"

#include "binidvalset.h"
#include "ioman.h"
#include "ioobj.h"
#include "seisbounds.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seiswrite.h"
#include "seispacketinfo.h"
#include "sorting.h"
#include "varlenarray.h"
#include "velocitycalc.h"

namespace Vel
{

const char* VolumeConverter::sKeyInput() { return "Input"; }
const char* VolumeConverter::sKeyOutput() { return "Output"; }

VolumeConverter::VolumeConverter( const IOObj& input, const IOObj& output,
				  const HorSampling& ranges,
				  const VelocityDesc& desc )
    : hrg_( ranges )
    , veloutpdesc_( desc )
    , input_( input.clone() )
    , output_( output.clone() )
    , reader_( 0 )
    , writer_( 0 )
    , sequentialwriter_(0)
{
    reader_ = new SeisTrcReader( input_ );
    if ( !reader_->prepareWork() )
    {
	delete reader_;
	reader_ = 0;
	errmsg_ = "Cannot read input";
	return;
    }

    const SeisPacketInfo& packetinfo =
	    reader_->seisTranslator()->packetInfo();

    if ( packetinfo.cubedata )
    {
	BinIDValueSet bivs( 0, false );
	bivs.add( *packetinfo.cubedata );
	bivs.remove( hrg_, false );
	totalnr_ = bivs.totalSize();
    }
    else
    {
	totalnr_ = hrg_.totalNr();
    }

    Seis::SelData* seldata = new Seis::RangeSelData( hrg_ );
    reader_->setSelData( seldata );
}


VolumeConverter::~VolumeConverter()
{
    delete input_;
    delete output_;

    if ( writer_ ) delete writer_;
    if ( sequentialwriter_ ) delete sequentialwriter_;
    if ( reader_ ) delete reader_;
}

bool VolumeConverter::doFinish( bool res )
{
    delete reader_;
    reader_ = 0;

    if ( !sequentialwriter_->finishWrite() )
	res = false;

    delete sequentialwriter_;
    sequentialwriter_ = 0;

    delete writer_;
    writer_ = 0;

    return res;
}


bool VolumeConverter::doPrepare( int nrthreads )
{
    if ( errmsg_ )
	return false;

    if ( !input_ || !output_ )
    {
	errmsg_ = "Either input or output cannot be found";
	return false;
    }

    delete writer_;
    writer_ = 0;

    if ( !velinpdesc_.usePar( input_->pars() ) )
    {
	errmsg_ = "Cannot read velocity information on input.";
	return false;
    }

    veloutpdesc_.fillPar( output_->pars() );
    if ( !IOM().commitChanges( *output_ ) )
    {
	errmsg_ = "Cannot write velocity information on output";
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
	errmsg_ = "Input/output velocities are not interval, RMS, or Avg "
	           "or are identical.";
	return false;
    }

    //Check input Vel

    if ( !reader_ )
    {
	reader_ = new SeisTrcReader( input_ );
	if ( !reader_->prepareWork() )
	{
	    delete reader_;
	    reader_ = 0;
	    errmsg_ = "Cannot read input";
	    return false;
	}

	Seis::SelData* seldata = new Seis::RangeSelData( hrg_ );
	reader_->setSelData( seldata );
    }

    writer_ = new SeisTrcWriter( output_ );
    sequentialwriter_ = new SeisSequentialWriter( writer_ );

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
	const SamplingData<double>& sd = trc.info().sampling;
	TypeSet<float> timesamps;
	const float sampstep = trc.info().sampling.step;
	for ( int idx=0; idx<trc.size(); idx++ )
	    timesamps += trc.startPos() + idx * sampstep;

	float* interptr = 0;
	if ( velinpdesc_.type_ != VelocityDesc::Interval )
	{
	    if ( veloutpdesc_ != VelocityDesc::Interval )
	    {
		mTryAlloc(interptr,float[trc.size()]);
		if ( !interptr )
		{
		    errmsg_ = "Not enough memory";
		    delete outputtrc;
		    outputtrc = 0;
		    return false;
		}
	    }
	    if ( velinpdesc_.type_ == VelocityDesc::Avg ) 
	    {
		 if ( !computeVint( inputptr, timesamps[0], timesamps.arr(),
			trc.size(), interptr ? interptr : outptr ) )
		{
		    delete outputtrc;
		    outputtrc = 0;
		}
	    }
	    else 
	    {
		if ( !computeDix( inputptr, sd, trc.size(), 
					interptr ? interptr :outptr ) )
		{
		    delete outputtrc;
		    outputtrc = 0;
		}
	    }	
	}

	if ( veloutpdesc_.type_ == VelocityDesc::Avg )
	{
	    if ( !computeVavg( interptr ? interptr : inputptr, timesamps[0],\
				timesamps.arr(), trc.size(), outptr ) )
	    {
		delete outputtrc;
		outputtrc = 0;
	    }
	}
	else if ( veloutpdesc_.type_ == VelocityDesc::RMS )
	{
	    if ( !computeVrms( interptr ? interptr : inputptr, sd, trc.size(), 
				outptr ) )
	    {
		delete outputtrc;
		outputtrc = 0;
	    }
	}
	delete [] interptr;

	//Process trace
	sequentialwriter_->submitTrace( outputtrc, true );
	addToNrDone( 1 );

	Threads::MutexLocker lock( lock_ );	
	res = getNewTrace( trc, threadidx );
    }

    return res==0;
}


char VolumeConverter::getNewTrace( SeisTrc& trc, int threadidx )
{
    if ( !reader_ )
	return 0;

    int res = 2;
    while ( res==2 || !hrg_.includes( trc.info().binid ) )
	res = reader_->get( trc.info() );

    if ( res==1 )
    {
	if ( !reader_->get( trc ) )
	    return -1;

	sequentialwriter_->announceTrace( trc.info().binid );
	return 1;
    }

    if ( res==-1 )
	errmsg_ = "Cannot read input";

    delete reader_;
    reader_ = 0;

    return res;
}


}; //namespace
