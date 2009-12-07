
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Dec 2009
________________________________________________________________________

-*/

static const char* rcsID = "$Id: velocityvolumeconversion.cc,v 1.2 2009-12-07 17:52:44 cvskris Exp $";

#include "velocityvolumeconversion.h"

#include "ioman.h"
#include "ioobj.h"
#include "seisbounds.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "sorting.h"
#include "varlenarray.h"
#include "velocitycalc.h"

namespace Vel
{

const char* VolumeConverter::sKeyInput() { return "Input"; }
const char* VolumeConverter::sKeyOutput() { return "Output"; }

VolumeConverter::VolumeConverter( IOObj* input, IOObj* output,
				  const HorSampling& ranges,
				  const VelocityDesc& desc )
    : hrg_( ranges )
    , veldesc_( desc )
    , input_( input )
    , output_( output )
    , reader_( 0 )
    , writer_( 0 )
    , maxbuffersize_( 1000 )
{ }


VolumeConverter::~VolumeConverter()
{
    delete input_;
    delete output_;
    delete writer_;
    delete reader_;
}


bool VolumeConverter::doPrepare( int nrthreads )
{
    if ( !input_ || !output_ )
    {
	errmsg_ = "Either input or output cannot be found";
	return false;
    }

    delete [] reader_;
    reader_ = 0;

    delete [] writer_;
    writer_ = 0;

    VelocityDesc inpdesc;
    if ( !inpdesc.usePar( input_->pars() ) )
    {
	errmsg_ = "Cannot read velocity information on input.";
	return false;
    }

    veldesc_.fillPar( output_->pars() );
    if ( !IOM().commitChanges( *output_ ) )
    {
	errmsg_ = "Cannot write velocity information on output";
	return false;
    }

    if ( (inpdesc.type_ != VelocityDesc::Interval &&
	 inpdesc.type_!=VelocityDesc::RMS ) ||
         ( veldesc_.type_ != VelocityDesc::Interval &&
	 veldesc_.type_!=VelocityDesc::RMS ) ||
	 inpdesc.type_ == veldesc_.type_ )
    {
	errmsg_ = "Input/output velocities are not interval or RMS, "
	           "or are identical.";
	return false;
    }

    //Check input Vel

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

    writer_ = new SeisTrcWriter( output_ );

    curtrcs_.setSize( nrthreads, -1 );
    curtrcs_.fillWith( -1 );

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
	if ( veldesc_.type_==VelocityDesc::Interval )
	{
	    if ( !computeDix( inputptr, trc.info().sampling, trc.size(),
			      outptr ) )
	    {
		delete outputtrc;
		outputtrc = 0;
	    }
	}
	else
	{
	    if ( !computeVrms( inputptr, trc.info().sampling, trc.size(),
			      outptr ) )
	    {
		delete outputtrc;
		outputtrc = 0;
	    }
	}

	//Process trace

	Threads::MutexLocker lock( lock_ );	
	if ( outputtrc )
	{
	    while ( threadidx && outputs_.size()>=maxbuffersize_ )
		lock_.wait();

	    outputs_ += outputtrc;
	}

	res = getNewTrace( trc, threadidx );
    }

    return res==0;
}


char VolumeConverter::getNewTrace( SeisTrc& trc, int threadidx )
{
    if ( !threadidx ) //Thread doing the writing
    {
	if ( !writeTraces() )
	    return -1;
    }

    if ( !reader_ )
	return 0;

    int res = 2;
    while ( res==2 || !hrg_.includes( trc.info().binid ) )
	res = reader_->get( trc.info() );

    if ( res==1 )
    {
	if ( !reader_->get( trc ) )
	    return -1;

	curtrcs_[threadidx] = getTrcIdx( trc.info().binid );
	return 1;
    }

    if ( res==-1 )
	errmsg_ = "Cannot read input";

    delete reader_;
    reader_ = 0;

    return res;
}


bool VolumeConverter::writeTraces()
{
    bool res = true;
    while ( res )
    {
	int first = -1;
	for ( int idx=0; idx<curtrcs_.size(); idx++ )
	{
	    if ( curtrcs_[idx]==-1 )
		continue;

	    if ( first==-1 || curtrcs_[idx]<first )
		first = curtrcs_[idx];
	}

	if ( first==-1 )
	    return true;

	mAllocVarLenArr( SeisTrc*, trctowrite, outputs_.size() );
	TypeSet<int> trcidxs;
	for ( int idx=outputs_.size()-1; idx>=0; idx-- )
	{
	    const int trcidx = getTrcIdx( outputs_[idx]->info().binid );
	    if ( trcidx<first )
	    {
		trctowrite[trcidxs.size()] = outputs_[idx];
		outputs_.remove( idx );
		trcidxs += trcidx;
	    }
	}

	lock_.signal( true ); //Tell waiting threads that they may add to buf
	lock_.unLock();

	if ( trcidxs.size() )
	{
	    int* trcidxsptr = trcidxs.arr();
	    sort_coupled<int, SeisTrc*>( trcidxsptr,trctowrite,trcidxs.size() );

	    for ( int idx=0; res && idx<trcidxs.size(); idx++ )
	    {
		if ( !writer_->put( *trctowrite[idx] ) )
		{
		    errmsg_ = "Cannot write output.";
		    res = false;
		}
	    }

	    for ( int idx=0; idx<trcidxs.size(); idx++ )
		delete trctowrite[idx];

	    addToNrDone( trcidxs.size() );
	}

	lock_.lock();

	if ( !trcidxs.size() )
	    break;
    }

    return res;
}


int VolumeConverter::getTrcIdx( const BinID& bid ) const
{
    return bid.inl * hrg_.nrCrl() + bid.crl;
}

}; //namespace
