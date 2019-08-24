/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 23-3-1996
 * FUNCTION : SynthSeis
-*/


#include "synthseisgenerator.h"

#include "arrayndimpl.h"
#include "arrayndstacker.h"
#include "fourier.h"
#include "muter.h"
#include "reflectivitysampler.h"
#include "samplfunc.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "trckey.h"
#include "waveletmanager.h"
#include "keystrs.h"


SynthSeis::RayModel::RayModel( const RayTracerData& rtdata )
    : raytracerdata_(&rtdata)
{
}


SynthSeis::RayModel::~RayModel()
{
}


const RayTracerData& SynthSeis::RayModel::rayTracerOutput() const
{
    return *raytracerdata_.ptr();
}


bool SynthSeis::RayModel::hasZeroOffsetOnly() const
{
    return raytracerdata_->hasZeroOffsetOnly();
}


const TimeDepthModel& SynthSeis::RayModel::zeroOffsetD2T() const
{
    return raytracerdata_->getZeroOffsTDModel();
}


bool SynthSeis::RayModel::hasSampledReflectivities() const
{
    return !reflModels().isEmpty() &&
	   sampledreflmodels_.size() == reflModels().size();
}


ReflectivityModelSet& SynthSeis::RayModel::reflModels( bool sampled )
{
   return sampled ? sampledreflmodels_ :
    const_cast<RayTracerData*>( raytracerdata_.ptr() )->getReflectivities();
}


const ReflectivityModelSet& SynthSeis::RayModel::reflModels( bool sampled) const
{ return sampled ? sampledreflmodels_ : raytracerdata_->getReflectivities(); }


void SynthSeis::RayModel::forceReflTimes( const StepInterval<float>& si )
{
    if ( !raytracerdata_->hasReflectivity() )
	return;

    for ( int idx=0; idx<raytracerdata_->nrOffset(); idx++ )
    {
	ReflectivityModel& refmodel =
	   const_cast<ReflectivityModel&>(raytracerdata_->getReflectivity(idx));
	for ( int iref=0; iref<refmodel.size(); iref++ )
	{
	    const float twt = si.atIndex(iref);
	    refmodel[iref].time_ = twt;
	    refmodel[iref].correctedtime_ = twt;
	}
    }
}


bool SynthSeis::RayModelSet::hasZeroOffsetOnly() const
{
    for ( const auto* rm : *this )
    {
	if ( !rm->hasZeroOffsetOnly() )
	    return false;
    }

    return true;
}



#define mErrRet(msg, retval) { msg_ = msg; return retval; }
#define mpErrRet(msg, retval) { pErrMsg(msg); return retval; }

mImplClassFactory( SynthSeis::Generator, factory );

SynthSeis::GenBase::GenBase()
    : outputsampling_(mUdf(float),-mUdf(float),mUdf(float))
    , worksampling_(mUdf(float),-mUdf(float),mUdf(float))
{}


SynthSeis::GenBase::~GenBase()
{
}


void SynthSeis::GenBase::setWavelet( const Wavelet* wvlt )
{
    wavelet_ = wvlt;
}


void SynthSeis::GenBase::setOutSampling( const ZSampling& zsamp )
{
    outputsampling_ = zsamp;
}


void SynthSeis::GenBase::fillPar( IOPar& par ) const
{
    if ( wavelet_ )
	par.set( sKey::WaveletID(), WaveletMGR().getID(*wavelet_) );
    else
	par.removeWithKey( sKey::WaveletID() );

    par.setYN( sKeyFourier(), isfourier_ );
    par.setYN( sKeyNMO(), applynmo_ );
    par.setYN( sKeyInternal(), dointernalmultiples_ );
    if ( dointernalmultiples_ )
	par.set( sKeySurfRefl(), surfreflcoeff_ );
    if ( applynmo_ )
    {
	par.set( sKeyStretchLimit(), stretchlimit_ );
	par.set( sKeyMuteLength(), mutelength_ );
    }
    par.set( sKeyWorkRange(), worksampling_ );
    par.set( sKey::ZRange(), outputsampling_ );
}


bool SynthSeis::GenBase::usePar( const IOPar& par )
{
    DBKey waveletid;
    if ( par.get(sKey::WaveletID(),waveletid) )
	wavelet_ = WaveletMGR().fetch( waveletid );

    par.getYN( sKeyFourier(), isfourier_ );
    par.getYN( sKeyNMO(), applynmo_ );

    const bool founddoint = par.getYN( sKeyInternal(), dointernalmultiples_ );
    dosampledreflectivities_ = founddoint && dointernalmultiples_;
    if ( dointernalmultiples_ )
	par.get( sKeySurfRefl(), surfreflcoeff_ );

    if ( applynmo_ )
    {
	par.get( sKeyStretchLimit(), stretchlimit_);
	par.get( sKeyMuteLength(), mutelength_ );
    }

    par.get( sKeyWorkRange(), worksampling_ );
    par.get( sKey::ZRange(), outputsampling_ );

    return true;
}


bool SynthSeis::GenBase::isInputOK()
{
    if ( !wavelet_ )
	mErrRet(tr("Wavelet required to compute trace range from models"),
		false)

    const float outputsr = mIsUdf(outputsampling_.step) ? wavelet_->sampleRate()
							: outputsampling_.step;
    if ( !mIsEqual(wavelet_->sampleRate(),outputsr,1e-4f) )
    {
	Wavelet& wavelet = const_cast<Wavelet&>(*wavelet_);
	wavelet.setSampleRate( outputsr );
    }

    if ( wavelet_->samplePositions().width(false) < outputsr )
	mErrRet(tr("Wavelet length must be larger than output sampling rate"),
		false)

    return true;
}


bool SynthSeis::GenBase::setSamplingFromModels(
				const ObjectSet<ReflectivityModel>& models )
{
    ReflectivityModelSet::getTimeRange( models, worksampling_, false );
    const float outputsr = wavelet_->sampleRate();
    worksampling_.step = outputsr;
    addWaveletLength( worksampling_ );
    if ( applynmo_ )
    {
	ReflectivityModelSet::getTimeRange( models, outputsampling_, true );
	outputsampling_.step = outputsr;
	addWaveletLength( outputsampling_ );
	worksampling_.start = outputsampling_.start;
    }
    else
	outputsampling_ = worksampling_;

    return !outputsampling_.isUdf();
}


void SynthSeis::GenBase::addWaveletLength( ZSampling& zsamp )
{
    const float outputsr = zsamp.step;

    zsamp.scale( 1.f / outputsr );
    zsamp.start = mIsEqual( (float)mNINT32(zsamp.start), zsamp.start,
				1e-2f )
		      ? mNINT32(zsamp.start) : Math::Floor( zsamp.start);
    zsamp.stop = mIsEqual( (float)mNINT32(zsamp.stop), zsamp.stop,
			       1e-2f )
		     ? mNINT32(zsamp.stop) : Math::Ceil( zsamp.stop );
    zsamp.scale( outputsr );
    zsamp.start += wavelet_->samplePositions().start;
    zsamp.stop += wavelet_->samplePositions().stop;
}



SynthSeis::Generator::Generator()
    : GenBase()
    , SequentialTask("Synthetic Trace Generator")
{
}


SynthSeis::Generator::~Generator()
{
    freqwavelet_.setEmpty();
    freqreflectivities_.setEmpty();
}


SynthSeis::Generator* SynthSeis::Generator::create( bool advanced )
{
    SynthSeis::Generator* sg = 0;
    const BufferStringSet& fkys = SynthSeis::Generator::factory().getKeys();

    if ( !fkys.isEmpty() && advanced )
	sg = factory().create( fkys.get( fkys.size()-1 ) );

    if ( !sg )
	sg = new SynthSeis::Generator();

    return sg;
}


void SynthSeis::Generator::setWavelet( const Wavelet* wvlt )
{
    freqwavelet_.erase();
    GenBase::setWavelet( wvlt );
}


bool SynthSeis::Generator::setModel( RayModel& model, int offsetidx )
{
    model_ = &model;
    offsetidx_ = offsetidx;

    nrdone_ = 0;
    msg_ = tr("Generating synthetic seismic");

    return true;
}


bool SynthSeis::Generator::isOK() const
{
    if ( !wavelet_ )
	mErrRet(uiStrings::phrCannotCreate(tr("synthetics without wavelet")),
								    false);
    const int wvltsz = wavelet_->size();
    if (wvltsz < 2)
	mErrRet(tr("Wavelet is too short - at minimum 3 samples are required"),
								    false);
    if ( !model_ || !model_->raytracerdata_->validOffsetIdx(offsetidx_) )
	mErrRet(uiStrings::phrCannotCreate(
		      tr("synthetics without reflectivity model")),false);

    if ( !trc_ )
	mErrRet(uiStrings::phrCannotCreate(
		    tr("synthetics without an output trace")),false);

    if ( convolvesize_ < 1 )
	return false;

#ifdef __debug__
    if ( isfourier_ )
    {
	if ( freqwavelet_.size() != convolvesize_ ||
	     freqreflectivities_.size() != convolvesize_ )
	    { pErrMsg("Invalid size"); DBG::forceCrash(true); }
    }
    if ( !tmpvals_ || trc_->size() != (outputsampling_.nrSteps()+1) )
	{ pErrMsg("Invalid size"); DBG::forceCrash(true); }

    const od_int64 sz = dynamic_cast<const ArrayValueSeries<float,float>* >(
						tmpvals_.ptr() )->size();
    if ( mCast(int,sz) != convolvesize_ )
	{ pErrMsg("Invalid size"); DBG::forceCrash(true); }
#endif

    return true;
}


uiString SynthSeis::Generator::nrDoneText() const
{
    return tr("Steps done");
}


od_int64 SynthSeis::Generator::totalNr() const
{
    /*
       1- Frequency reflectivities
       2- Convolution
      */
    return 2;
}


int SynthSeis::Generator::nextStep()
{
    if ( nrdone_++ == 0 && !isOK() )
	return ErrorOccurred();

    if ( nrdone_ == 1 )
	return computeReflectivities();
    else if ( nrdone_ == 2 )
	return computeTrace();
    else
	return ErrorOccurred();
}


bool SynthSeis::Generator::setConvolveSize()
{
    if ( worksampling_.isUdf() )
	return false;

    const int oldconvsize = worksampling_.nrSteps() + 1;
    convolvesize_ = oldconvsize;
    freqwavelet_.setEmpty();
    freqreflectivities_.setEmpty();
    fft_ = 0;
    if ( isfourier_ )
    {
	fft_ = Fourier::CC::createDefault();
	if ( !fft_ )
	    return false;

	convolvesize_ = fft_->getFastSize( oldconvsize );
	if ( convolvesize_ < 1 )
	    return false;

	if ( !freqwavelet_.setSize(convolvesize_,float_complex(0,0)) ||
	     !freqreflectivities_.setSize(convolvesize_,float_complex(0,0)) )
	{
	    convolvesize_ = 0;
	    return false;
	}
    }

    if ( convolvesize_ < 1 )
	mpErrRet( "Cannot determine convolution size", false );

    //Always allocate, even if not used by zero offset case
    tmpvals_ = new ArrayValueSeries<float,float>( convolvesize_ );

    return tmpvals_->isOK();
}


#define mPrepFFT( ft, arr, dir, sz )\
    ft->setInputInfo( Array1DInfoImpl(sz) );\
    ft->setDir( dir ); ft->setNormalization( !dir );\
    ft->setInput( arr ); ft->setOutput( arr )

bool SynthSeis::Generator::init()
{
    if ( !setConvolveSize() )
	return false;

    return isfourier_ ? genFreqWavelet() : true;
}


bool SynthSeis::Generator::copyInit( const Generator& oth )
{
    IOPar par;
    oth.fillPar( par );
    usePar( par );
    if ( !setConvolveSize() )
	return false;
    else if ( !isfourier_ )
	return true;

    freqwavelet_ = oth.freqwavelet_;
    return true;
}


bool SynthSeis::Generator::genFreqWavelet()
{
    //TODO add taper if wavelet length less than output trace size
    TypeSet<float> samps;
    wavelet_->getSamples( samps );
    for ( int idx=0; idx<samps.size(); idx++ )
    {
	int arrpos = idx - wavelet_->centerSample();
	if ( arrpos < 0 )
	    arrpos += convolvesize_;

	freqwavelet_[arrpos] = samps[idx];
    }

    mPrepFFT( fft_.ptr(), freqwavelet_.arr(), true, convolvesize_ );
    if ( !fft_->run(true) )
    {
	freqwavelet_.setEmpty();
	mErrRet(tr("Could not run FFT for the wavelet spectrum"),false);
    }

    return true;
}


int SynthSeis::Generator::computeReflectivities()
{
    const StepInterval<float> sampling( outputsampling_.start,
	    outputsampling_.atIndex( convolvesize_-1 ), outputsampling_.step );

    ReflectivitySampler sampler( *model_->reflModels(false).get(offsetidx_),
				 sampling, freqreflectivities_ );
    bool isok = sampler.executeParallel(false);
    if ( !isok )
	mErrRet( sampler.message(), false );

    if ( dosampledreflectivities_ )
	sampler.getReflectivities( *model_->reflModels(true).get(offsetidx_) );

    return MoreToDo();
}


int SynthSeis::Generator::computeTrace()
{
    SeisTrc& res = *trc_;
    res.zero();

    const float offset = model_->raytracerdata_->getOffset( offsetidx_ );
    const bool applynmo = applynmo_ && !mIsZero(offset,mDefEpsF);

    SeisTrcValueSeries trcvs( res, 0 );
    ValueSeries<float>& vs = applynmo ? *tmpvals_.ptr() : trcvs;
    const int trcsz = res.size();
    const int procsz = applynmo ? convolvesize_ : trcsz;

    if ( isfourier_ )
    {
	if ( !doFFTConvolve(vs,procsz) )
	    return ErrorOccurred();
    }
    else if ( !doTimeConvolve(vs,procsz) )
	return ErrorOccurred();

    if ( applynmo && !doNMOStretch(vs,convolvesize_,trcvs,trcsz) )
	return ErrorOccurred();

    trc_ = 0; //We do not need it anymore

    return Finished();
}


void SynthSeis::Generator::getWaveletTrace( Array1D<float>& trc, float z,
			      float scal, SamplingData<float>& sampling ) const
{
    const int sz = trc.getSize(0);
    for ( int idx=0; idx<sz; idx++ )
    {
	const float twt = sampling.atIndex( idx );
	trc.set( idx, scal * wavelet_->getValue( twt-z ) );
    }
}


bool SynthSeis::Generator::doFFTConvolve( ValueSeries<float>& res, int outsz )
{
    if ( !fft_ )
	mErrRet(tr("Cannot allocate memory for FFT"), false)

    mAllocLargeVarLenArr(float_complex, cres, convolvesize_);
    if ( !cres )
	mErrRet(tr("Cannot allocate memory for FFT"), false)

    if ( freqreflectivities_.size() != convolvesize_ ||
	 freqwavelet_.size() != convolvesize_ )
	mErrRet(tr("Incorrect size"), false)

    for ( int idx=0; idx<convolvesize_; idx++ )
	cres[idx] = freqreflectivities_[idx] * freqwavelet_[idx];

    mPrepFFT(fft_, cres, false, convolvesize_);
    if ( !fft_->run(true) )
	mErrRet(tr("Cannot run FFT for convolution"), false)

    sortOutput( cres, res, outsz );

    return true;
}


bool SynthSeis::Generator::doTimeConvolve( ValueSeries<float>& res, int outsz )
{
    const ReflectivityModel& rm = *model_->reflModels(
			    model_->hasSampledReflectivities() )[offsetidx_];
    Array1DImpl<float> output( outsz );
    ArrayNDStacker<float> trcstacker( output, 0.f );
    trcstacker.manageInputs( true ).doNormalize( false );

    for ( int iref=0; iref<rm.size(); iref++ )
    {
	const ReflectivitySpike& spike = rm[iref];
	if ( !spike.isDefined() )
	    continue;

	Array1DImpl<float>* newtrc = new Array1DImpl<float>( outsz );
	getWaveletTrace( *newtrc, spike.time_, spike.reflectivity_.real(),
			 trc_->info().sampling_ );
	trcstacker.addInput( newtrc );
    }

    if ( !trcstacker.executeParallel(false) )
	mErrRet( trcstacker.errMsg(), false )

    output.getAll( res );
    return true;
}


bool SynthSeis::Generator::doNMOStretch(const ValueSeries<float>& input,
			int insz, ValueSeries<float>& out, int outsz ) const
{
    const ReflectivityModel& reflmodel = *model_->reflModels().get( offsetidx_);
    if ( reflmodel.isEmpty() )
	return true;

    float mutelevel = outputsampling_.start;
    float firsttime = mUdf(float);

    PointBasedMathFunction stretchfunc( PointBasedMathFunction::Linear,
				      PointBasedMathFunction::ExtraPolGradient);

    for ( int idx=0; idx<reflmodel.size(); idx++ )
    {
	const ReflectivitySpike& spike = reflmodel[idx];

	if ( idx>0 )
	{
	    //check for crossing events
	    const ReflectivitySpike& spikeabove = reflmodel[idx-1];
	    if ( spike.time_ < spikeabove.time_ )
	    {
		mutelevel = mMAX(spike.correctedtime_, mutelevel );
	    }
	}

	firsttime = mMIN( firsttime, spike.correctedtime_ );
	stretchfunc.add( spike.correctedtime_, spike.time_ );
    }

    if ( firsttime > 0.f )
	stretchfunc.add( 0.f, 0.f );

    const SeisTrc& trc = *trc_;
    trc.info().sampling_.indexOnOrAfter( mutelevel );

    SampledFunctionImpl<float,ValueSeries<float> > samplfunc( input, insz,
							outputsampling_.start,
							outputsampling_.step );
    for ( int idx=0; idx<outsz; idx++ )
    {
	const float corrtime = trc.info().sampling_.atIndex( idx );
	const float uncorrtime = stretchfunc.getValue( corrtime );
	const float stretch = corrtime>0 ? uncorrtime/corrtime -1 : 0;
	if ( stretch>stretchlimit_ )
	    mutelevel = mMAX( corrtime, mutelevel );

	const float outval = samplfunc.getValue( uncorrtime );
	out.setValue( idx, mIsUdf(outval) ? 0 : outval );
    }

    if ( mutelevel>outputsampling_.start )
    {
	Muter muter( mutelength_/outputsampling_.step, false );
	muter.mute( out, trc.size(), outputsampling_.getfIndex( mutelevel ) );
    }

    return true;
}



void SynthSeis::Generator::sortOutput( float_complex* cres,
				    ValueSeries<float>& res, int outsz ) const
{
    const SeisTrc& trc = *trc_;
    const float step = trc.info().sampling_.step;
    float start = mCast( float, mCast( int, trc.startPos()/step ) ) * step;
    if ( start < trc.startPos() - 1e-4f )
	start += step;

    const float width = step * convolvesize_;
    const int nperiods = mCast( int, Math::Floor( start/width ) ) + 1;
    const SamplingData<float> fftsampling( start, step );
    SeisTrc fftout( convolvesize_ );
    fftout.info().sampling_ = fftsampling;
    fftout.zero();

    const float stoptwt = start + width;
    float twt = width * nperiods - step;
    for ( int idx=0; idx<convolvesize_; idx++ )
    {
	twt += step;
	if ( twt > stoptwt - 1e-4f )
	    twt -= width;

	const int idy = fftsampling.nearestIndex( twt );
	if ( idy<0 || idy>convolvesize_-1 )
	    continue;

	fftout.set( idy, cres[idx].real(), 0 );
    }

    for ( int idx=0; idx<outsz; idx++ )
    {
	twt = trc.samplePos( idx );
	res.setValue( idx, fftout.getValue( twt, 0 ) );
    }
}



SynthSeis::MultiTraceGenerator::MultiTraceGenerator()
    : ParallelTask("Gather creator")
    , SynthSeis::GenBase()
    , totalnr_(1) //will be replaced
    , tk_(*new TrcKey)
{
    msg_ = tr("Generating synthetics");
}


SynthSeis::MultiTraceGenerator::~MultiTraceGenerator()
{
    delete &tk_;
}


void SynthSeis::MultiTraceGenerator::set( RayModel& model, SeisTrcBuf& trcs,
					  const TrcKey* tk )
{
    model_ = &model;
    trcs_ = &trcs;
    tk_ = tk ? *tk : TrcKey::udf();
    totalnr_ = model_->rayTracerOutput().nrOffset();
}


od_int64 SynthSeis::MultiTraceGenerator::nrIterations() const
{
    return totalnr_;
}


bool SynthSeis::MultiTraceGenerator::doPrepare( int nrthreads )
{
    const int trcsz = outputsampling_.nrSteps()+1;
    if ( !model_ || !trcs_ || trcsz<=0 )
	return false;

    IOPar genpar;
    fillPar( genpar );
    for ( int idx=0; idx<nrthreads; idx++ )
    {
	Generator* generator = Generator::create( dointernalmultiples_ );
	generators_ += generator;
	generator->setWavelet( wavelet_ );
	generator->usePar( genpar );
	if ( (idx==0 && !generator->init() ) ||
	     (idx>0 && !generator->copyInit(*generators_.get(0))) )
	    return false;
    }

    const int nrtraces = mCast(int,totalNr());
    if ( trcs_->size() != nrtraces )
    {
	const int initnrtraces = trcs_->size();
	for ( int idx=initnrtraces-1; idx>=nrtraces; idx-- )
	    delete trcs_->remove( idx );

	for ( int idx=initnrtraces; idx<nrtraces; idx++ )
	    trcs_->add( new SeisTrc( trcsz ) );
    }

    const bool withposition = !tk_.isUdf();
    for ( int idx=0; idx<nrtraces; idx++ )
    {
	SeisTrc& trc = *trcs_->get(idx);
	if ( trc.size() !=  trcsz )
	    trc.reSize( trcsz, false );
	trc.info().sampling_ = outputsampling_;
	trc.info().offset_ = model_->raytracerdata_->getOffset( idx );
	trc.info().coord_.setUdf();
	if ( withposition )
	    trc.info().setTrcKey( tk_ );
    }

    ReflectivityModelSet& sampledrefs = model_->sampledreflmodels_;
    if ( !dosampledreflectivities_ )
    {
	sampledrefs.setEmpty();
	return true;
    }

    if ( sampledrefs.size() != sampledrefs.size() )
    {
	const int initnrmodels = sampledrefs.size();
	for ( int idx=initnrmodels-1; idx>=nrtraces; idx-- )
	    sampledrefs.removeSingle(idx);

	for ( int idx=initnrmodels; idx<nrtraces; idx++ )
	    sampledrefs.add( new ReflectivityModel );
    }

    return true;
}


bool SynthSeis::MultiTraceGenerator::doWork( od_int64 start, od_int64 stop,
					     int thread )
{
    Generator& generator = *generators_[thread];
    for ( int idx=mCast(int,start); idx<=stop; idx++, addToNrDone(1) )
    {
	generator.setModel( *model_.ptr(), idx );
	generator.setOutput( *trcs_->get(idx) );
	if ( !generator.execute() )
	    mErrRet( generator.message(), false );
    }

    return true;
}


bool SynthSeis::MultiTraceGenerator::doFinish( bool success )
{
    deepErase( generators_ );
    trcs_ = 0;

    return success;
}
