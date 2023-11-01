/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seiszaxisstretcher.h"

#include "ailayer.h"
#include "arrayndimpl.h"
#include "genericnumer.h"
#include "ioman.h"
#include "paralleltask.h"
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
#include "valseriesinterpol.h"
#include "veldescimpl.h"
#include "zvalseriesimpl.h"


SeisZAxisStretcherNew::SeisZAxisStretcherNew( const IOObj& in, const IOObj& out,
					      const TrcKeyZSampling& outcs,
					      ZAxisTransform& ztf, bool ist2d,
					      const VelocityDesc* veldesc )
    : inpobj_(in)
    , outobj_(out)
    , curhrg_(false)
    , outcs_(outcs)
    , ztransform_(&ztf)
    , ist2d_(ist2d)
    , msg_(tr("Stretching data"))
    , srd_(SI().seismicReferenceDatum())
    , srduom_(UnitOfMeasure::surveyDefSRDStorageUnit())
{
    setRanges();
    if ( veldesc )
	setWorkers( *veldesc );
}


SeisZAxisStretcherNew::~SeisZAxisStretcherNew()
{
    delete seisreader_;
    delete sequentialwriter_;
    delete seiswriter_;
    delete worker_;
    delete vintworker_;

    if ( ztransform_ )
    {
	if ( voiid_>=0 )
	    ztransform_->removeVolumeOfInterest( voiid_ );
    }
}


bool SeisZAxisStretcherNew::isOK() const
{
    return ztransform_ && ztransform_->isOK() && totalnr_ >= 0;
}


uiString SeisZAxisStretcherNew::uiMessage() const
{
    return msg_;
}


uiString SeisZAxisStretcherNew::uiNrDoneText() const
{
    return ParallelTask::sTrcFinished();
}


void SeisZAxisStretcherNew::setRanges()
{
    const SeisIOObjInfo info( inpobj_ );
    if ( !info.isOK() )
	return;

    geomtype_ = info.geomType();
    if ( is2D(geomtype_) )
    {
	StepInterval<int> trcrg;
	ZSampling zrg;
	if ( !info.getRanges(outcs_.hsamp_.getGeomID(),trcrg,zrg) )
	    return;

	totalnr_ = trcrg.nrSteps() + 1;
    }
    else
    {
	TrcKeyZSampling storhrg;
	if ( !info.getRanges(storhrg) )
	    return;

	outcs_.hsamp_.limitTo( storhrg.hsamp_ );
	SeisTrcReader rdr( inpobj_, &geomtype_ );
	PosInfo::CubeData posinfo;
	if ( !rdr.prepareWork(Seis::Scan) || !rdr.get3DGeometryInfo(posinfo) )
	    return;

	totalnr_ = posinfo.totalSizeInside( outcs_.hsamp_ );
    }
}


bool SeisZAxisStretcherNew::init()
{
    if ( !isOK() )
	return false;

    const Pos::GeomID geomid = outcs_.hsamp_.getGeomID();
    delete seisreader_;
    seisreader_ = new SeisTrcReader( inpobj_, geomid, &geomtype_ );
    if ( !seisreader_->prepareWork() )
    {
	msg_ = seisreader_->errMsg();
	deleteAndNullPtr( seisreader_ );
	return false;
    }

    TrcKeyZSampling cs( true );
    cs.hsamp_ = outcs_.hsamp_;
    seisreader_->setSelData( new Seis::RangeSelData(cs) );

    delete seiswriter_;
    seiswriter_ = new SeisTrcWriter( outobj_, geomid, &geomtype_ );
    delete sequentialwriter_;
    sequentialwriter_ = new SeisSequentialWriter( seiswriter_ );

    zdomaininfo_ = &seisreader_->zDomain();
    if ( ztransform_->fromZDomainInfo().isDepth() )
	ztransform_->fromZDomainInfo().setDepthUnit(
					seisreader_->zDomain().depthType() );
    if ( ztransform_->toZDomainInfo().isDepth() )
	ztransform_->toZDomainInfo().setDepthUnit(
					seiswriter_->zDomain().depthType() );

    return true;
}


void SeisZAxisStretcherNew::setWorkers( const VelocityDesc& desc )
{
    deleteAndNullPtr( vintworker_ );
    delete worker_;
    worker_ = new Vel::Worker( desc, srd_, srduom_ );
    if ( desc.isInterval() &&
	 Vel::Worker::getUnit( desc ) == UnitOfMeasure::meterSecondUnit() )
	return;

    const VelocityDesc vintdesc( OD::VelocityType::Interval,
			     UnitOfMeasure::meterSecondUnit()->getLabel() );
    vintworker_ = new Vel::Worker( vintdesc, srd_, srduom_ );
}


void SeisZAxisStretcherNew::setUdfVal( float val )
{
    udfval_ = val;
}


bool SeisZAxisStretcherNew::doPrepare( int nrthreads )
{
    msg_ = tr("Stretching data");
    if ( !init() )
	return false;

    nrthreads_ = nrthreads;
    const MultiID transformmid = ztransform_->fromZDomainInfo().getID();
    if ( !transformmid.isUdf() )
    {
	autotransform_ = seisreader_->ioObj()->key() == transformmid;
	if ( worker_ && !autotransform_ )
	    deleteAndNullPtr( vintworker_ );
    }

    return true;
}


bool SeisZAxisStretcherNew::doWork( od_int64, od_int64, int )
{
    const ZSampling trcrg = outcs_.zsamp_;
    const SamplingData<float> sd( trcrg );
    const int outsz = trcrg.nrSteps()+1;
    const int icomp = 0;

    SeisTrc intrc;
    PtrMan<Array1D<float> > outputarr;
    PtrMan<FloatMathFunction> intrcfunc;
    PtrMan<ZAxisTransformSampler> sampler;
    float* outputptr = nullptr;
    const bool standardstretch = !worker_;
    if ( standardstretch )
    {
	auto* interpol =
		new ValueSeriesInterpolator<float>( intrc.interpolator() );
	interpol->udfval_ = udfval_;
	intrc.setInterpolator( interpol );
	intrcfunc = new SeisTrcFunction( intrc, icomp );

	outputarr = new Array1DImpl<float>( outsz );
	outputptr = outputarr->getData();
	if ( !intrcfunc || !outputptr )
	    return false;

	const bool is2d = is2D( geomtype_ );
	sampler = new ZAxisTransformSampler( *ztransform_, true, sd, is2d );
	if ( is2d && seisreader_ && seisreader_->selData() )
	    sampler->setLineName( Survey::GM().getName(
					    seisreader_->selData()->geomID()));
    }

    /*TODOVEL: One should provide an array of t0 to stretchVelocity
      if the velocity is of type RMS and statics are defined
      Difficult to do here without access to EarthModel    */
    while ( shouldContinue() && getInputTrace(intrc) )
    {
	auto* outtrc = new SeisTrc( outsz );
	outtrc->info().sampling = sd;
	outtrc->info().setTrcKey( intrc.info().trcKey() );
	outtrc->info().coord = intrc.info().coord;

	if ( standardstretch )
	    stretch( intrc, icomp, *intrcfunc, *sampler, outputptr, *outtrc );
	else
	{
	    bool res = false;
	    if ( autotransform_ )
		res = selfStretchVelocity( intrc, icomp, *outtrc );
	    if ( (autotransform_ && !res) || !autotransform_ )
		res = stretchVelocity( intrc, icomp, *outtrc );
	    if ( !res )
		outtrc->setAll( mUdf(float), icomp );
	}

	if ( !sequentialwriter_->submitTrace(outtrc,true) )
	    return false;

	addToNrDone( 1 );
    }

    return true;
}


bool SeisZAxisStretcherNew::doFinish( bool success )
{
    zdomaininfo_ = nullptr;
    deleteAndNullPtr( seisreader_ );

    if ( !sequentialwriter_->finishWrite() )
	return false;

    deleteAndNullPtr( sequentialwriter_ );
    deleteAndNullPtr( seiswriter_ );

    return success;
}


bool SeisZAxisStretcherNew::getInputTrace( SeisTrc& trc )
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

    TrcKey tk;
    while ( shouldContinue() )
    {
	if ( !seisreader_->get(trc) )
	{
	    deleteAndNullPtr( seisreader_ );
	    return false;
	}

	tk = trc.info().trcKey();
	if ( !outcs_.hsamp_.includes(tk) )
	    continue;

	if ( curhrg_.isEmpty() || !curhrg_.includes(tk) )
	{
	    waitforall_ = true;
	    while ( shouldContinue() && nrwaiting_!=nrthreads_-1 )
		readerlock_.wait();

	    waitforall_ = false;
	    readerlock_.signal( true );

	    if ( !shouldContinue() )
		return false;

	    if ( !loadTransformChunk(tk.inl()) )
		continue;
	}

	sequentialwriter_->announceTrace( tk.position() );

	return true;
    }

    return false;
}


#define mMaxNrTrc	5000

bool SeisZAxisStretcherNew::loadTransformChunk( int inl )
{
    int chunksize = is2D(geomtype_) ? 1 : mMaxNrTrc/outcs_.hsamp_.nrCrl();
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


void SeisZAxisStretcherNew::stretch( const SeisTrc& inptrc, int icomp,
				     const FloatMathFunction& intrcfunc,
				     ZAxisTransformSampler& sampler,
				     float* outputptr, SeisTrc& outtrc ) const
{
    sampler.setTrcKey( inptrc.info().trcKey() );
    sampler.computeCache( Interval<int>( 0, outtrc.size()-1) );
    reSample( intrcfunc, sampler, outputptr, outtrc.size() );
    for ( int idx=0; idx<outtrc.size(); idx++ )
	outtrc.set( idx, outputptr[idx], icomp );
}


bool SeisZAxisStretcherNew::selfStretchVelocity( const SeisTrc& inptrc,
						 int icomp,
						 SeisTrc& outtrc ) const
{
    const int inpsz = inptrc.size();
    const SamplingData<float>& inputsd = inptrc.info().sampling;
    const VelocityDesc& desc = worker_->getDesc();
    const VelocityDesc* vintdesc = vintworker_ ? &vintworker_->getDesc()
					       : nullptr;
    const bool needconversion = vintworker_;

    PtrMan<const SeisTrcValueSeries> inputvs =
				     new SeisTrcValueSeries( inptrc, icomp );
    PtrMan<SeisTrc> ftrc;
    const Vel::Worker* worker = worker_;
    if ( !inputvs->arr() || needconversion )
    {
	ftrc = new SeisTrc( inptrc );
	ftrc->convertToFPs();
	if ( needconversion )
	{
	    worker = vintworker_;
	    if ( !ftrc->updateVelocities(desc,*vintdesc,*zdomaininfo_,
					 srd_,srduom_) )
		return false;
	}

	inputvs = new SeisTrcValueSeries( *ftrc, icomp );
	if ( !inputvs->arr() )
	    return false;
    }

    const RegularZValues trczvals( inputsd, inpsz, *zdomaininfo_ );
    PtrMan<ZValueSeries> velszvals = Vel::Worker::getZVals( trczvals,
							    srd_, srduom_ );
    ElasticModel tracemdl;
    if ( !tracemdl.createFromVel(*velszvals,inputvs->arr()) )
	return false;

    const int modsz = tracemdl.size();
    TypeSet<double> moddepths( modsz, mUdf(double) );
    TypeSet<double> modvels( modsz, mUdf(double) );
    double depth = srduom_->getSIValue( -srd_ );
    int idx = 0;
    for ( const auto* layer : tracemdl )
    {
	depth += layer->getThickness();
	moddepths[idx] = depth;
	modvels[idx++] = layer->getPVel();
    }

    const ArrayValueSeries<double,double> vels( modvels.arr(), false, modsz );
    velszvals = new ArrayZValues<double>( moddepths.arr(), modsz,
					  ZDomain::DepthMeter() );
    const RegularZValues zvals_out( outtrc.info().sampling, outtrc.size(),
				    ztransform_->toZDomainInfo() );
    SeisTrcValueSeries outputvs( outtrc, icomp );
    PtrMan<ValueSeries<double> > vout =
		    ScaledValueSeries<double,float>::getFrom( outputvs );
    if ( !worker->sampleVelocities(vels,*velszvals,zvals_out,*vout) )
	return false;

    return needconversion
	? outtrc.updateVelocities( *vintdesc, desc, zvals_out.zDomainInfo(),
				   srd_, srduom_ )
	: true;
}


bool SeisZAxisStretcherNew::stretchVelocity( const SeisTrc& inptrc, int icomp,
					     SeisTrc& outtrc ) const
{
    const int inpsz = inptrc.size();
    const SamplingData<float>& inputsd = inptrc.info().sampling;
    const SeisTrcValueSeries inputvs( inptrc, icomp );
    PtrMan<ValueSeries<double> > vels =
			ScaledValueSeries<double,float>::getFrom(
				const_cast<SeisTrcValueSeries&>( inputvs ) );

    ArrayValueSeries<float,float> zoutvals( inpsz );
    if ( !zoutvals.isOK() )
	return false;

    // First compute the transformed z of the trace sampling
    // from the transform
    ztransform_->transformTrc( inptrc.info().trcKey(), inputsd, inpsz,
			       zoutvals.arr() );

    /* Then compute the back transformed z of these values
       from the velocity trace. The resulting Z are thus in the same domain
       as the trace sampling */
    const ArrayZValues<float> velszvals( zoutvals.arr(), inpsz,
					 ztransform_->toZDomainInfo() );

    const RegularZValues zvals_out( outtrc.info().sampling, outtrc.size(),
				    ztransform_->toZDomainInfo() );
    SeisTrcValueSeries outputvs( outtrc, icomp );
    PtrMan<ValueSeries<double> > vout =
		    ScaledValueSeries<double,float>::getFrom( outputvs );

    return worker_->sampleVelocities( *vels, velszvals, zvals_out, *vout );
}


// Obsolete implementation
#include "velocitycalc.h"

mStartAllowDeprecatedSection

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
	deleteAndNullPtr( seisreader_ );
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
	deleteAndNullPtr( seisreader_ );
	return;
    }

    if ( seisreadertdmodel_ )
    {
	seisreadertdmodel_->setSelData( sd.clone() );
	if ( !seisreadertdmodel_->prepareWork() )
	{
	    deleteAndNullPtr( seisreadertdmodel_ );
	    return;
	}
    }

    sd.copyFrom( seiswriter_->selData() ? *seiswriter_->selData()
					: *new Seis::RangeSelData(true) );
    sd.setGeomID( geomid );
    seiswriter_->setSelData( sd.clone() );
}


void SeisZAxisStretcher::setUdfVal( float val )
{
    udfval_ = val;
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
	    interpol->udfval_ = udfval_;
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
	    deleteAndNullPtr( seisreader_ );
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
	    deleteAndNullPtr( seisreadertdmodel_ );
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

mStopAllowDeprecatedSection
