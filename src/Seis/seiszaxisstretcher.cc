/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seiszaxisstretcher.h"

#include "arrayndimpl.h"
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
					const TrcKeyZSampling& outcs,
					ZAxisTransform& ztf,
					bool ist2d,
					bool stretchz )
    : seisreader_(nullptr)
    , seisreadertdmodel_(nullptr)
    , seiswriter_(nullptr)
    , sequentialwriter_(nullptr)
    , nrwaiting_(0)
    , waitforall_(false)
    , nrthreads_(0)
    , curhrg_(false)
    , outcs_(outcs)
    , ztransform_(&ztf)
    , voiid_(-1)
    , ist2d_(ist2d)
    , stretchz_(stretchz)
    , isvrms_(false)
{
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

    const Seis::GeomType gt = info.geomType();
    seisreader_ = new SeisTrcReader( in, &gt );
    if ( !seisreader_->prepareWork(Seis::Scan) ||
	 !seisreader_->seisTranslator())
    {
	deleteAndZeroPtr( seisreader_ );
	return;
    }

    if ( !is2d_ )
    {
	const SeisPacketInfo& spi = seisreader_->seisTranslator()->packetInfo();
	TrcKeySampling storhrg; storhrg.set( spi.inlrg, spi.crlrg );
	outcs_.hsamp_.limitTo( storhrg );
    }

    TrcKeyZSampling cs( true );
    cs.hsamp_ = outcs_.hsamp_;
    seisreader_->setSelData( new Seis::RangeSelData(cs) );
    if ( seisreadertdmodel_ )
	seisreadertdmodel_->setSelData( new Seis::RangeSelData(cs) );


    if ( !is2d_ )
    {
	const SeisPacketInfo& packetinfo =
			        seisreader_->seisTranslator()->packetInfo();
	if ( packetinfo.cubedata )
	    totalnr_ = packetinfo.cubedata->totalSizeInside( cs.hsamp_ );
	else
	    totalnr_ = mCast( int, cs.hsamp_.totalNr() );
    }
    else
    {
	totalnr_ = mCast( int, cs.hsamp_.totalNr() );
    }

    seiswriter_ = new SeisTrcWriter( out, &gt );
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


void SeisZAxisStretcher::setGeomID( Pos::GeomID geomid )
{
    Seis::RangeSelData sd; sd.copyFrom( *seisreader_->selData() );
    sd.setGeomID( geomid );
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
    sd.setGeomID( geomid );
    seiswriter_->setSelData( sd.clone() );
}


#define mOpInverse(val,inv) \
  ( inv ? 1.0/val : val )

bool SeisZAxisStretcher::doWork( od_int64, od_int64, int )
{
    StepInterval<float> trcrg = outcs_.zsamp_;
    SamplingData<float> sd( trcrg );
    mAllocLargeVarLenArr( float, outputptr, trcrg.nrSteps()+1 );

    SeisTrc intrc;
    SeisTrc modeltrc;
    PtrMan<FloatMathFunction> intrcfunc = 0;
    PtrMan<ZAxisTransformSampler> sampler = 0;

    if ( !stretchz_ )
    {
	sampler = new ZAxisTransformSampler( *ztransform_, true, sd, is2d_ );
	if ( is2d_ && seisreader_ && seisreader_->selData() )
	    sampler->setLineName( Survey::GM().getName(
					    seisreader_->selData()->geomID()));
	intrcfunc = new SeisTrcFunction( intrc, 0 );

	if ( !intrcfunc )
	    return false;
    }

    TrcKey curtrckey;
    while ( shouldContinue() && getInputTrace( intrc, curtrckey ) )
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

		float* vrmsarr = tmpseistrcvsin.arr();
		PtrMan<Array1DImpl<float> > inparr = 0;
		if ( !vrmsarr )
		{
		    inparr = new Array1DImpl<float>( insz );
		    tmpseistrcvsin.copytoArray( *inparr );
		    vrmsarr = inparr->arr();
		}

		computeDix( vrmsarr, inputsd, insz, vintarr );

		for ( int ids=0; ids<insz; ids++ )
		    intrc.set( ids, vintarr[ids], 0 );

		usevint = true;
	    }

	    //Computed from the input trace, not the model
	    mAllocVarLenArr(float, twt, insz);
	    mAllocVarLenArr(float, depths, insz);

	    SamplingData<float> inputsd( intrc.info().sampling );
	    SeisTrcValueSeries inputvs( intrc, 0 );


	    ztransform_->transformTrc( curtrckey, inputsd, insz,
				       ist2d_ ? depths : twt );

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
			float prevdepth = depths[idx] = prevtwt*inputvs[idx]/2;
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
	    sampler->setTrcKey( curtrckey );
	    sampler->computeCache( Interval<int>( 0, outtrc->size()-1) );

	    ValueSeriesInterpolator<float>* interpol =
		new ValueSeriesInterpolator<float>( intrc.interpolator() );
	    interpol->udfval_ = mUdf(float);
	    intrc.setInterpolator( interpol );

	    reSample( *intrcfunc, *sampler, outputptr, outtrc->size() );
	    for ( int idx=outtrc->size()-1; idx>=0; idx-- )
		outtrc->set( idx, outputptr[idx], 0 );
	}

	outtrc->info().setTrcKey( intrc.info().trcKey() );
	outtrc->info().coord = intrc.info().coord;
	if ( !sequentialwriter_->submitTrace(outtrc,true) )
	    return false;

	addToNrDone( 1 );
    }

    return true;
}


bool SeisZAxisStretcher::getInputTrace( SeisTrc& trc, TrcKey& trckey )
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
	    deleteAndZeroPtr( seisreader_ );
	    return false;
	}

	trckey = trc.info().trcKey();
	if ( !outcs_.hsamp_.includes(trckey) )
	    continue;

	if ( curhrg_.isEmpty() || !curhrg_.includes(trckey) )
	{
	    waitforall_ = true;
	    while ( shouldContinue() && nrwaiting_!=nrthreads_-1 )
		readerlock_.wait();

	    waitforall_ = false;
	    readerlock_.signal( true );

	    if ( !shouldContinue() )
		return false;

	    if ( !loadTransformChunk( trckey.inl() ) )
		continue;
	}

	sequentialwriter_->announceTrace( trckey.position() );

	return true;
    }

    return false;
}


bool SeisZAxisStretcher::getModelTrace( SeisTrc& trc, TrcKey& trckey )
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
	    deleteAndZeroPtr( seisreadertdmodel_ );
	    return false;
	}

	trckey = trc.info().trcKey();
	if ( curhrg_.isEmpty() || !curhrg_.includes(trckey) )
	{
	    waitforall_ = true;
	    while ( shouldContinue() && nrwaiting_!=nrthreads_-1 )
		readerlockmodel_.wait();

	    waitforall_ = false;
	    readerlockmodel_.signal( true );

	    if ( !shouldContinue() )
		return false;

	    if ( !loadTransformChunk( trckey.inl() ) )
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
    int chunksize = is2d_ ? 1 : mMaxNrTrc/outcs_.hsamp_.nrCrl();
    if ( chunksize<1 ) chunksize = 1;

    curhrg_ = outcs_.hsamp_;
    curhrg_.start_.inl() = inl;
    curhrg_.stop_.inl() = curhrg_.start_.inl() + curhrg_.step_.inl() *
			 (chunksize-1);
    if ( curhrg_.stop_.inl()>outcs_.hsamp_.stop_.inl() )
	curhrg_.stop_.inl() = outcs_.hsamp_.stop_.inl();

    TrcKeyZSampling cs( outcs_ );
    cs.hsamp_ = curhrg_;

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
