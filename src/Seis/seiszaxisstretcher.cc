/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : January 2008
-*/

static const char* rcsID mUnusedVar = "$Id: seiszaxisstretcher.cc,v 1.21 2012-05-02 15:11:48 cvskris Exp $";

#include "seiszaxisstretcher.h"

#include "genericnumer.h"
#include "ioman.h"
#include "posinfo.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seispacketinfo.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "velocitycalc.h"
#include "valseriesinterpol.h"
#include "zaxistransform.h"


SeisZAxisStretcher::SeisZAxisStretcher( const IOObj& in, const IOObj& out,
					const CubeSampling& outcs,
					ZAxisTransform& ztf,
       					bool forward,
					bool stretchz )
    : seisreader_( 0 )
    , seisreadertdmodel_( 0 )
    , seiswriter_( 0 )
    , sequentialwriter_( 0 )
    , nrwaiting_( 0 )
    , waitforall_( false )
    , nrthreads_( 0 )
    , curhrg_( false )
    , outcs_( outcs )
    , ztransform_( &ztf )
    , voiid_( -1 )
    , forward_( forward )
    , stretchz_( stretchz )
{
    init( in, out, 0 );
}


SeisZAxisStretcher::SeisZAxisStretcher( const IOObj& in, const IOObj& out,
					const CubeSampling& outcs,
					const MultiID& tdmodelmid,
       					bool forward,
					bool stretchz )
    : seisreader_( 0 )
    , seisreadertdmodel_( 0 )
    , seiswriter_( 0 )
    , sequentialwriter_( 0 )
    , nrwaiting_( 0 )
    , waitforall_( false )
    , nrthreads_( 0 )
    , curhrg_( false )
    , outcs_( outcs )
    , ztransform_( 0 )
    , voiid_( -1 )
    , forward_( forward )
    , stretchz_( stretchz )
{
    PtrMan<IOObj> tdmodel = 0;
    if ( stretchz_ )
	tdmodel = IOM().get( tdmodelmid );
    else
    {
	ztransform_ = forward_
	    		? (VelocityStretcher*) new Time2DepthStretcher
			: (VelocityStretcher*) new Depth2TimeStretcher;

	mDynamicCastGet( VelocityStretcher*, vstrans, ztransform_ )
	if ( !vstrans->setVelData( tdmodelmid ) || !ztransform_->isOK() )
	    return;       
    }

    init( in, out, tdmodel );
}


void SeisZAxisStretcher::init( const IOObj& in, const IOObj& out,
			       const IOObj* tdmodel )
{
    if ( (stretchz_ && !tdmodel) || (!stretchz_ && !ztransform_) )
	return;

    SeisIOObjInfo info( in );
    is2d_ = info.is2D();

    if ( ztransform_ )
	ztransform_->ref();

    seisreader_ = new SeisTrcReader( &in );
    if ( !seisreader_->prepareWork(Seis::Scan) ||
	 !seisreader_->seisTranslator())
    {
	delete seisreader_;
	seisreader_ = 0;
	return;
    }

    if ( tdmodel )
    {
	seisreadertdmodel_ = new SeisTrcReader( tdmodel );
	if ( !seisreadertdmodel_->prepareWork(Seis::Scan) ||
	     !seisreadertdmodel_->seisTranslator())
	{
	    delete seisreadertdmodel_;
	    seisreadertdmodel_ = 0;
	    return;
	}
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
    if ( seisreadertdmodel_ )
	seisreadertdmodel_->setSelData( new Seis::RangeSelData(cs) );


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

    if ( seisreadertdmodel_ )
	delete seisreadertdmodel_;

    if ( ztransform_ )
    {
	if ( voiid_>=0 )
	    ztransform_->removeVolumeOfInterest( voiid_ );

	ztransform_->unRef();
    }
}


bool SeisZAxisStretcher::isOK() const
{
    return seisreader_ && seiswriter_
	&& ( seisreadertdmodel_ || (ztransform_ && ztransform_->isOK()) );
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

    if ( seisreadertdmodel_ )
    {
	seisreadertdmodel_->setSelData( sd.clone() );
	if ( !seisreadertdmodel_->prepareWork() )
	{
	    delete seisreadertdmodel_;
	    seisreadertdmodel_ = 0;
	    return;
	}
    }

    sd.copyFrom( seiswriter_->selData() ? *seiswriter_->selData()
	    				: *new Seis::RangeSelData(true) );
    sd.lineKey() = lk;
    seiswriter_->setSelData( sd.clone() );
}


#define mOpInverse(val,inv) \
  ( inv ? 1.0/val : val )

bool SeisZAxisStretcher::doWork( od_int64, od_int64, int ) 
{
    StepInterval<float> trcrg = outcs_.zrg;
    SamplingData<double> sd( trcrg );
    ArrPtrMan<float> outputptr = new float[trcrg.nrSteps()+1];

    SeisTrc intrc;
    SeisTrc modeltrc;
    PtrMan<FloatMathFunction> trcfunc = 0;
    PtrMan<ZAxisTransformSampler> sampler = 0;
    if ( !stretchz_ )
    {
	sampler = new ZAxisTransformSampler( *ztransform_, forward_, sd, is2d_);

	trcfunc = new SeisTrcFunction( intrc, 0 );

	if ( !trcfunc )
	    return false;
    }

    BinID curbid;
    while ( shouldContinue() && getInputTrace( intrc, curbid ) )
    {
	int outsz = trcrg.nrSteps()+1;
	int insz = intrc.size();
	SeisTrc* outtrc = new SeisTrc( outsz );
	outtrc->info().sampling = sd;

	if ( stretchz_ && ( forward_ || getModelTrace( modeltrc, curbid ) ) )
	{
	    if ( isvrms_ )                                                      
	    {                                                                   
		SeisTrcValueSeries tmpseistrcvsin( intrc, 0 );
		SamplingData<double> inputsd( intrc.info().sampling );          
		mAllocVarLenArr( float, vintarr, insz );                        
		if ( !vintarr ) return false;                                   

		computeDix( tmpseistrcvsin.arr(), inputsd, insz, vintarr );

		for ( int ids=0; ids<insz; ids++ )
		    intrc.set( ids, vintarr[ids], 0 );

		isvint_ = true;
	    }

	    SeisTrcValueSeries seistrcvsin( intrc, 0 );
	    if ( forward_ ) modeltrc = intrc;
	    SeisTrcValueSeries seistrcvsmodel( modeltrc, 0 );

	    // wanteddimvals = z computed from model, wanted dimension
	    mAllocVarLenArr( float, wanteddimvals, insz );

	    //truez and truezsampled = initial dimension
	    mAllocVarLenArr( float, truez, insz );		
	    mAllocVarLenArr( float, truezsampled, outsz );

	    if ( !wanteddimvals || !truez || !truezsampled
		    || modeltrc.size()<insz )
		return false;

	    truez[0] = 0;
	    SamplingData<double> modelsd( modeltrc.info().sampling );
	    if ( isvint_ )
	    {
		if ( forward_ )
		    TimeDepthConverter::calcDepths( seistrcvsmodel, insz,
						    modelsd, wanteddimvals );
		else
		    TimeDepthConverter::calcTimes( seistrcvsmodel, insz,
			    			   modelsd, wanteddimvals );

		for ( int idx=1; idx<insz; idx++ )
		{
		    float difftwt = wanteddimvals[idx]-wanteddimvals[idx-1];
		    truez[idx] = truez[idx-1] + difftwt
				 * mOpInverse( seistrcvsin.value(idx),forward_);
		}
	    }
	    else //should be Vavg logically
	    {
		for ( int idx=0; idx<insz; idx++ )
		{
		    wanteddimvals[idx] = modelsd.atIndex(idx)
			    * mOpInverse( 2.0, forward_ )
			    * mOpInverse( seistrcvsmodel.value(idx), !forward_);
		    if ( !idx ) continue;
		    truez[idx] = wanteddimvals[idx]
			    * mOpInverse( seistrcvsin.value(idx), forward_ );
		}
	    }

	    resampleZ( truez, wanteddimvals, insz, sd, outsz, truezsampled );
	    outtrc->set( 0, seistrcvsin.value(0), 0 );
	    for ( int idx=1; idx<outsz; idx++ )
	    {
		const float val = forward_
		    ? isvint_
			? sd.step / (truezsampled[idx] - truezsampled[idx-1])
			: sd.atIndex(idx) / truezsampled[idx]
		    : isvint_
			? (truezsampled[idx] - truezsampled[idx-1])/(sd.step/2)
			: truezsampled[idx] / sd.atIndex(idx);

		outtrc->set( idx, val, 0 );
	    }
	}
	else
	{
	    if ( !sampler ) return false;
	    sampler->setBinID( curbid );
	    sampler->computeCache( Interval<int>( 0, outtrc->size()-1) );

	    ValueSeriesInterpolator<float>* interpol =
		new ValueSeriesInterpolator<float>( intrc.interpolator() );
	    interpol->udfval_ = mUdf(float);
	    intrc.setInterpolator( interpol );

	    reSample( *trcfunc, *sampler, outputptr, outtrc->size() );
	    for ( int idx=outtrc->size()-1; idx>=0; idx-- )
		outtrc->set( idx, outputptr[idx], 0 );
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


bool SeisZAxisStretcher::getInputTrace( SeisTrc& trc, BinID& curbid )
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
	    //TODO verify how to get lineidx if it's necessary
	    curbid.inl = ztransform_ ? ztransform_->lineIndex(
			    seisreader_->selData()->lineKey().lineName() ) : 0;
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


bool SeisZAxisStretcher::getModelTrace( SeisTrc& trc, BinID& curbid )
{
    Threads::MutexLocker lock( readerlockmodel_ );
    if ( waitforall_ )
    {
	nrwaiting_++;
	if ( nrwaiting_==nrthreads_-1 )
	    readerlockmodel_.signal(true);

	while ( shouldContinue() && waitforall_ )
	    readerlockmodel_.wait();

	nrwaiting_--;
    }

    if ( !seisreadertdmodel_ )
    	return false;

    while ( shouldContinue() ) 
    {
	if ( !seisreadertdmodel_->get(trc) )
	{
	    delete seisreadertdmodel_;
	    seisreadertdmodel_ = 0;
	    return false;
	}

	curbid = trc.info().binid;
	if ( is2d_ )
	{
	    //TODO verify how to get lineidx if it's necessary
	    curbid.inl = ztransform_ ? ztransform_->lineIndex(
		    seisreadertdmodel_->selData()->lineKey().lineName() ) : 0;
	    curbid.crl = trc.info().nr;
	}

	if ( !outcs_.hrg.includes( curbid ) )
	    continue;

	if ( curhrg_.isEmpty() || !curhrg_.includes(curbid) )
	{
	    waitforall_ = true;
	    while ( shouldContinue() && nrwaiting_!=nrthreads_-1 )
		readerlockmodel_.wait();

	    waitforall_ = false;
	    readerlockmodel_.signal( true );

	    if ( !shouldContinue() )
		return false;

	    if ( !loadTransformChunk( curbid.inl ) )
		continue;
	}

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

    bool res = true;
    if ( ztransform_ )
    {
	if ( voiid_<0 )
	    voiid_ = ztransform_->addVolumeOfInterest( cs, forward_ );
	else
	    ztransform_->setVolumeOfInterest( voiid_, cs, forward_ );

	res = ztransform_->loadDataIfMissing( voiid_ );
    }

    return res;
}


bool SeisZAxisStretcher::doFinish( bool success )
{
    if ( !sequentialwriter_->finishWrite() )
	return false;

    return success;
}
