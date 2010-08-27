/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Dec 2009
________________________________________________________________________

-*/

static const char* rcsID = "$Id: velocityvolumeconversion.cc,v 1.5 2010-08-27 17:59:42 cvskris Exp $";

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
    , veldesc_( desc )
    , input_( input.clone() )
    , output_( output.clone() )
    , reader_( 0 )
    , writer_( 0 )
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
    delete writer_;
    delete reader_;
    deepErase( outputs_ );
}

bool VolumeConverter::doFinish( bool res )
{
    if ( res )
	return writeTraces();

    delete reader_;
    reader_ = 0;

    delete writer_;
    writer_ = 0;

    return res;
}


bool VolumeConverter::doPrepare( int nrthreads )
{
    if ( errmsg_ )
	return false;

    maxbuffersize_ = 4*nrthreads;

    if ( !input_ || !output_ )
    {
	errmsg_ = "Either input or output cannot be found";
	return false;
    }

    delete writer_;
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
	    outputs_ += outputtrc;

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
    else
    {
	while ( activetraces_.size()>=maxbuffersize_ )
	    lock_.wait();
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

	activetraces_ += trc.info().binid;
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

    while ( true )
    {
	ObjectSet<SeisTrc> trctowrite;
	int idx = 0;
	while ( idx<activetraces_.size() )
	{
	    const BinID& bid = activetraces_[idx];

	    SeisTrc* trc = 0;
	    for ( int idy=0; idy<outputs_.size(); idy++ )
	    {
		if ( outputs_[idy]->info().binid==bid )
		{
		    trc = outputs_.remove( idy );
		    break;
		}
	    }

	    if ( !trc )
	    {
		idx--;
		break;
	    }

	    trctowrite += trc;
	    idx++;
	}

	if ( idx>=0 )
	    activetraces_.remove( 0, idx );

	if ( !trctowrite.size() )
	    return true;

	lock_.signal(true);
	lock_.unLock();

	for ( int idy=0; res && idy<trctowrite.size(); idy++ )
	{
	    if ( !writer_->put( *trctowrite[idy] ) )
	    {
		errmsg_ = "Cannot write output.";
		res = false;
	    }
	}

	addToNrDone( trctowrite.size() );
	deepErase( trctowrite );

	lock_.lock();
	if ( !res )
	    return res;
    }

    return res;
}

}; //namespace
