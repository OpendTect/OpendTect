/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : January 2008
-*/

static const char* rcsID = "$Id: seiszaxisstretcher.cc,v 1.14 2011-03-21 01:22:06 cvskris Exp $";

#include "seiszaxisstretcher.h"

#include "genericnumer.h"
#include "posinfo.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seispacketinfo.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "valseriesinterpol.h"
#include "zaxistransform.h"


SeisZAxisStretcher::SeisZAxisStretcher( const IOObj& in, const IOObj& out,
					const CubeSampling& outcs,
					ZAxisTransform& ztf,
       					bool forward,
					bool inverse )
    : seisreader_( 0 )
    , seiswriter_( 0 )
    , sequentialwriter_( 0 )
    , nrwaiting_( 0 )
    , waitforall_( false )
    , nrthreads_( 0 )
    , curhrg_( false )
    , outcs_( outcs )
    , ztransform_( ztf )
    , voiid_( -1 )
    , forward_( forward )
    , stretchinverse_( inverse )
{
    SeisIOObjInfo info( in );
    is2d_ = info.is2D();

    ztransform_.ref();

    seisreader_ = new SeisTrcReader( &in );
    if ( !seisreader_->prepareWork(Seis::Scan) ||
	 !seisreader_->seisTranslator())
    {
	delete seisreader_;
	seisreader_ = 0;
	return;
    }

    if ( !is2d_ )
    {
	const SeisPacketInfo& spi = seisreader_->seisTranslator()->packetInfo();
	HorSampling storhrg; storhrg.set( spi.inlrg, spi.crlrg );
	outcs_.hrg.limitTo( storhrg );
    }
    
    CubeSampling cs( true );
    cs.hrg = outcs_.hrg;
    seisreader_->setSelData( new Seis::RangeSelData(cs) );


    if ( !is2d_ )
    {
	const SeisPacketInfo& packetinfo =
			        seisreader_->seisTranslator()->packetInfo();
	if ( packetinfo.cubedata )
	    totalnr_ = packetinfo.cubedata->totalSizeInside( cs.hrg );
	else
	    totalnr_ = cs.hrg.totalNr();
    }
    else
    {
	totalnr_ = cs.hrg.totalNr();
    }

    seiswriter_ = new SeisTrcWriter( &out );
    sequentialwriter_ = new SeisSequentialWriter( seiswriter_ );
}


SeisZAxisStretcher::~SeisZAxisStretcher()
{
    delete seisreader_;
    delete seiswriter_;
    delete sequentialwriter_;

    if ( voiid_>=0 )
	ztransform_.removeVolumeOfInterest( voiid_ );

    ztransform_.unRef();
}


bool SeisZAxisStretcher::isOK() const
{
    return seisreader_ && seiswriter_ && ztransform_.isOK();
}


void SeisZAxisStretcher::setLineKey( const char* lk )
{
    Seis::RangeSelData sd; sd.copyFrom( *seisreader_->selData() );
    sd.lineKey() = lk;
    seisreader_->setSelData( sd.clone() );
    if ( !seisreader_->prepareWork() )
    {
	delete seisreader_;
	seisreader_ = 0;
	return;
    }

    sd.copyFrom( seiswriter_->selData() ? *seiswriter_->selData()
	    				: *new Seis::RangeSelData(true) );
    sd.lineKey() = lk;
    seiswriter_->setSelData( sd.clone() );
}

class InverseSeisTrcFunction : public FloatMathFunction
{
public:
    InverseSeisTrcFunction(const SeisTrc& trc)
	: trc_(trc) {} 
    float       getValue(float z) const
    		{
		    const float val = trc_.getValue(z,0);
		    if ( !mIsUdf(val) )
			return 1.0 / val;
		    return val;
		}
protected:

    const SeisTrc&	trc_;
};


bool SeisZAxisStretcher::doWork( od_int64, od_int64, int ) 
{
    StepInterval<float> trcrg = outcs_.zrg;
    SamplingData<double> sd( trcrg );
    ZAxisTransformSampler sampler( ztransform_, forward_, sd, is2d_ );

    ArrPtrMan<float> outputptr = new float[trcrg.nrSteps()+1];

    SeisTrc intrc;
    PtrMan<FloatMathFunction> trcfunc = 0;
    if ( stretchinverse_ )
	trcfunc = new InverseSeisTrcFunction( intrc );
    else
	trcfunc = new SeisTrcFunction( intrc, 0 );

    if ( !trcfunc )
	return false;

    BinID curbid;
    while ( shouldContinue() && getTrace( intrc, curbid ) )
    {
	SeisTrc* outtrc = new SeisTrc( trcrg.nrSteps()+1 );
	outtrc->info().sampling = sd;

	sampler.setBinID( curbid );
	sampler.computeCache( Interval<int>( 0, outtrc->size()-1) );

	ValueSeriesInterpolator<float>* interpol =
	    new ValueSeriesInterpolator<float>( intrc.interpolator() );
	interpol->udfval_ = mUdf(float);
	intrc.setInterpolator( interpol );

	reSample( *trcfunc, sampler, outputptr, outtrc->size() );

	for ( int idx=outtrc->size()-1; idx>=0; idx-- )
	{
	    float val = outputptr[idx];
	    if ( stretchinverse_ && !mIsUdf(val) )
		val = 1.0 / val;
	    outtrc->set( idx, val, 0 );
	}

	outtrc->info().nr = intrc.info().nr;
	outtrc->info().binid = intrc.info().binid;
	outtrc->info().coord = intrc.info().coord;
	if ( !sequentialwriter_->submitTrace( outtrc, true ) )
	    return false;

	reportNrDone( 1 );
    }

    return true;
}


bool SeisZAxisStretcher::getTrace( SeisTrc& trc, BinID& curbid )
{
    Threads::MutexLocker lock( readerlock_ );
    if ( waitforall_ )
    {
	nrwaiting_++;
	if ( nrwaiting_==nrthreads_-1 )
	    readerlock_.signal(true);

	while ( shouldContinue() && waitforall_ )
	    readerlock_.wait();

	nrwaiting_--;
    }

    if ( !seisreader_ )
    	return false;

    while ( shouldContinue() ) 
    {
	if ( !seisreader_->get(trc) )
	{
	    delete seisreader_;
	    seisreader_ = 0;
	    return false;
	}

	curbid = trc.info().binid;
	if ( is2d_ )
	{
	    curbid.inl = ztransform_.lineIndex(
				seisreader_->selData()->lineKey().lineName() );
	    curbid.crl = trc.info().nr;
	}

	if ( !outcs_.hrg.includes( curbid ) )
	    continue;

	if ( curhrg_.isEmpty() || !curhrg_.includes(curbid) )
	{
	    waitforall_ = true;
	    while ( shouldContinue() && nrwaiting_!=nrthreads_-1 )
		readerlock_.wait();

	    waitforall_ = false;
	    readerlock_.signal( true );

	    if ( !shouldContinue() )
		return false;

	    if ( !loadTransformChunk( curbid.inl ) )
		continue;
	}

	sequentialwriter_->announceTrace( trc.info().binid );

	return true;
    }

    return false;
}


bool SeisZAxisStretcher::doPrepare( int nrthreads )
{
    nrthreads_ = nrthreads;
    return true;
}

#define mMaxNrTrc	5000

bool SeisZAxisStretcher::loadTransformChunk( int inl )
{
    int chunksize = is2d_ ? 1 : mMaxNrTrc/outcs_.hrg.nrCrl();
    if ( chunksize<1 ) chunksize = 1;

    curhrg_ = outcs_.hrg;
    curhrg_.start.inl = inl;
    curhrg_.stop.inl = curhrg_.start.inl + curhrg_.step.inl * (chunksize-1);
    if ( curhrg_.stop.inl>outcs_.hrg.stop.inl )
	curhrg_.stop.inl = outcs_.hrg.stop.inl;

    CubeSampling cs( outcs_ );
    cs.hrg = curhrg_;

    if ( voiid_<0 )
	voiid_ = ztransform_.addVolumeOfInterest( cs, forward_ );
    else
	ztransform_.setVolumeOfInterest( voiid_, cs, forward_ );

    return ztransform_.loadDataIfMissing( voiid_ );
}


bool SeisZAxisStretcher::doFinish( bool success )
{
    if ( !sequentialwriter_->finishWrite() )
	return false;

    return success;
}
