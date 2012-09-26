/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : January 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

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
    , ist2d_( forward )
    , stretchz_( stretchz )
    , isvrms_(false)
{
    init( in, out );
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
    , ist2d_( forward )
    , stretchz_( stretchz )
    , isvrms_(false)
{
    ztransform_ = ist2d_
	    		? (VelocityStretcher*) new Time2DepthStretcher
			: (VelocityStretcher*) new Depth2TimeStretcher;

    mDynamicCastGet( VelocityStretcher*, vstrans, ztransform_ )
    if ( !vstrans->setVelData( tdmodelmid ) || !ztransform_->isOK() )
	return;       

    init( in, out );
}


void SeisZAxisStretcher::init( const IOObj& in, const IOObj& out )
{
    if ( !ztransform_ )
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
    SamplingData<float> sd( trcrg );
    ArrPtrMan<float> outputptr = new float[trcrg.nrSteps()+1];

    SeisTrc intrc;
    SeisTrc modeltrc;
    PtrMan<FloatMathFunction> intrcfunc = 0;
    PtrMan<ZAxisTransformSampler> sampler = 0;
    
    if ( !stretchz_ )
    {
	sampler = new ZAxisTransformSampler( *ztransform_, true, sd, is2d_ );
	intrcfunc = new SeisTrcFunction( intrc, 0 );

	if ( !intrcfunc )
	    return false;
    }

    BinID curbid;
    while ( shouldContinue() && getInputTrace( intrc, curbid ) )
    {
	int outsz = trcrg.nrSteps()+1;
	int insz = intrc.size();
	SeisTrc* outtrc = new SeisTrc( outsz );
	outtrc->info().sampling = sd;

	if ( stretchz_ )
	{
	    bool usevint = isvint_;
	    
	    if ( isvrms_ )                                                      
	    {                                                                   
		SeisTrcValueSeries tmpseistrcvsin( intrc, 0 );
		SamplingData<double> inputsd( intrc.info().sampling );          
		mAllocVarLenArr( float, vintarr, insz );                        
		if ( !mIsVarLenArrOK(vintarr) ) return false;                                   

		computeDix( tmpseistrcvsin.arr(), inputsd, insz, vintarr );

		for ( int ids=0; ids<insz; ids++ )
		    intrc.set( ids, vintarr[ids], 0 );

		usevint = true;
	    }

	    //Computed from the input trace, not the model
	    mAllocVarLenArr(float, twt, insz);
	    mAllocVarLenArr(float, depths, insz);
	    
	    SamplingData<float> inputsd( intrc.info().sampling );
	    SeisTrcValueSeries inputvs( intrc, 0 );
	    
	    
	    ztransform_->transform( curbid, inputsd, insz,
				    ist2d_ ? depths : twt 	);
	    
	    
	    if ( ist2d_ )
	    {
		int idx=0;
		for ( ; idx<insz; idx++ )
		{
		    if ( !mIsUdf(depths[idx]) && !mIsUdf(inputvs[idx]) )
			break;
		    twt[idx] = mUdf(float);
		}
		
		//Fill twt using depth array and input-values
	    	if ( usevint )
		{
		    if ( idx!=insz )
		    {
			float prevdepth = depths[idx];
		    	float prevtwt = twt[idx] = 2*prevdepth/inputvs[idx];
		    	idx++;
		    	for ( ; idx<insz; idx++)
		    	{
			    if ( mIsUdf(depths[idx]) || mIsUdf(inputvs[idx]) )
				twt[idx] = mUdf(float);
			    else
			    {
			        prevtwt = twt[idx] = prevtwt +
				    (depths[idx]-prevdepth)*2/inputvs[idx];
				prevdepth = depths[idx];
			    }
			}
		    }
		}
		else //Vavg
		{
		    for ( ; idx<insz; idx++ )
		    {
			if ( mIsUdf(depths[idx]) || mIsUdf(inputvs[idx]) )
			    twt[idx] = mUdf(float);
			else
			    twt[idx] = depths[idx]*2/inputvs[idx];
		    }
		    
		}
	    }
	    else
	    {
		int idx=0;
		for ( ; idx<insz; idx++ )
		{
		    if ( !mIsUdf(twt[idx]) && !mIsUdf(inputvs[idx]) )
			break;
		    depths[idx] = mUdf(float);
		}

		//Fill depth using twt array and input-values
		if ( usevint )
		{
		    if ( idx!=insz )
		    {
			float prevtwt = twt[idx];
		    	float prevdepth = depths[idx] = prevtwt * inputvs[idx] / 2;
			idx++;
		    	for ( ; idx<insz; idx++ )
		    	{
			    if ( mIsUdf(twt[idx]) || mIsUdf(inputvs[idx] ) )
				depths[idx] = mUdf(float);
			    else
			    {
			    	prevdepth = depths[idx] = prevdepth +
		    		    (twt[idx]-prevtwt) * inputvs[idx]/2;
				prevtwt = twt[idx];
			    }
		    	}
		    }
		}
		else //Vavg
		{
		    for ( ; idx<insz; idx++ )
		    {
			if ( mIsUdf(depths[idx]) || mIsUdf(inputvs[idx]) )
			    twt[idx] = mUdf(float);
			else
			    depths[idx] = twt[idx] * inputvs[idx]/2;
		    }
		}
	    }
	    
	    PointBasedMathFunction dtfunc( PointBasedMathFunction::Linear,
				PointBasedMathFunction::ExtraPolGradient );
	    PointBasedMathFunction tdfunc( PointBasedMathFunction::Linear,
				PointBasedMathFunction::ExtraPolGradient );
	    float prevdepth;
	    float prevtwt;

	    if ( ist2d_ )
	    {
	    	for ( int idx=0; idx<insz; idx++ )
		{
		    if ( mIsUdf(depths[idx]) || mIsUdf(twt[idx]) )
			continue;
				
		    dtfunc.add( depths[idx], twt[idx] );
		}
		
		prevdepth = sd.atIndex( 0 );
		prevtwt = dtfunc.size()
		    ? dtfunc.getValue( prevdepth )
		    : mUdf(float); 
	    }
	    else
	    {
		for ( int idx=0; idx<insz; idx++ )
		{
		    if ( mIsUdf(depths[idx]) || mIsUdf(twt[idx]) )
			continue;
		    
		    tdfunc.add( twt[idx], depths[idx] );
		}
		
		prevtwt = sd.atIndex( 0 );
		prevdepth = tdfunc.size()
		    ? tdfunc.getValue( prevtwt )
		    : mUdf(float);
	    }
		
	    const bool alludf = ist2d_ ? dtfunc.isEmpty() : tdfunc.isEmpty();
	    for ( int idx=1; idx<outsz; idx++ )
	    {
		float vel;
		
		if ( alludf )
		    vel = mUdf(float);
		else
		{
		    float curdepth;
		    float curtwt;

		    if ( ist2d_ )
		    {
		    	curdepth = sd.atIndex( idx );
		    	curtwt = dtfunc.getValue( curdepth );
		    }
		    else
		    {
		    	curtwt = sd.atIndex( idx );
		    	curdepth = tdfunc.getValue( curtwt );
		    }
		
	    	
		    if ( usevint )
		    {
		    	vel = (curdepth-prevdepth)/(curtwt-prevtwt) * 2;
		    	prevtwt = curtwt;
		    	prevdepth = curdepth;
		    }
		    else
		    {
		    	vel = curdepth/curtwt * 2;
		    }
		}
		    
		outtrc->set( idx, vel, 0 );
		    
	    }
	    	    
	    outtrc->set( 0, outtrc->get( 1, 0 ), 0 );
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

	    reSample( *intrcfunc, *sampler, outputptr, outtrc->size() );
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
	    voiid_ = ztransform_->addVolumeOfInterest( cs, true );
	else
	    ztransform_->setVolumeOfInterest( voiid_, cs, true );

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
