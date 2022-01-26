/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : SynthSeis
-*/


#include "synthseis.h"

#include "ailayer.h"
#include "arrayndalgo.h"
#include "arrayndstacker.h"
#include "fourier.h"
#include "ioman.h"
#include "muter.h"
#include "raytracerrunner.h"
#include "reflectivitysampler.h"
#include "samplfunc.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "wavelet.h"


namespace Seis
{

// SynthGenDataPack

SynthGenDataPack::SynthGenDataPack( const ReflectivityModelSet& refmodels,
				    GeomType gt, const TypeSet<float>& offsets,
				    const ZSampling& zrg )
    : refmodels_(&refmodels)
    , gt_(gt)
    , offsets_(offsets)
    , outputsampling_(zrg)
    , synthgenpars_(*new IOPar)
{
}


SynthGenDataPack::~SynthGenDataPack()
{
    delete &synthgenpars_;
    if ( freqsampledrefset_ )
	deepErase( *freqsampledrefset_ );
    delete freqsampledrefset_;

    if ( timesampledrefset_ )
	deepErase( *timesampledrefset_ );
    delete timesampledrefset_;
}


const ReflectivityModelSet& SynthGenDataPack::getModels() const
{
    return *refmodels_.ptr();
}


bool SynthGenDataPack::hasSameParams( const SynthGenDataPack& oth ) const
{
    return refmodels_->hasSameParams( *oth.refmodels_.ptr() ) &&
	   offsets_ == oth.offsets_ &&
	   outputsampling_.isEqual(oth.outputsampling_,1e-8f) &&
	   ( (freqsampledrefset_ && oth.freqsampledrefset_) ||
	     (!freqsampledrefset_ && !oth.freqsampledrefset_) ) &&
	   ( (timesampledrefset_ && oth.timesampledrefset_) ||
	     (!timesampledrefset_ && !oth.timesampledrefset_) );
}


bool SynthGenDataPack::hasSameParams( const IOPar& reflpars,
				      const IOPar& synthgenpars ) const
{
    return refmodels_->hasSameParams( reflpars ) &&
	   SynthGenerator::areEquivalent( synthgenpars, synthgenpars_ );
}


const ReflecSet* SynthGenDataPack::getSampledReflectivitySet( int imdl,
							     bool isfreq ) const
{
    if ( isfreq )
	return freqsampledrefset_ && freqsampledrefset_->validIdx( imdl )
	       ? freqsampledrefset_->get( imdl ) : nullptr;

    return timesampledrefset_ && timesampledrefset_->validIdx( imdl )
	   ? timesampledrefset_->get( imdl ) : nullptr;
}


bool SynthGenDataPack::isStack() const
{
    return !Seis::isPS( gt_ );
}


bool SynthGenDataPack::isPS() const
{
    return Seis::isPS( gt_ );
}


bool SynthGenDataPack::isCorrected() const
{
    bool corrected = true;
    synthgenpars_.getYN( SynthGenBase::sKeyNMO(), corrected );

    return corrected;
}


int SynthGenDataPack::getOffsetIdx( float offset ) const
{
    if ( offset <= 0.f )
	return -1;

    return offsets_.indexOf( offset );
}


#define mErrRet(msg, retval) { errmsg_ = msg; return retval; }
#define mpErrRet(msg, retval) { pErrMsg(msg); return retval; }

mImplFactory( SynthGenerator, SynthGenerator::factory );

SynthGenBase::SynthGenBase()
    : stretchlimit_( cStdStretchLimit() ) //20%
    , mutelength_( cStdMuteLength() )  //20ms
    , outputsampling_(mUdf(float),-mUdf(float),mUdf(float))
{}


SynthGenBase::~SynthGenBase()
{
    if ( waveletismine_ )
	delete wavelet_;
}


bool SynthGenBase::setWavelet( const Wavelet* wvlt, OD::PtrPolicy pol )
{
    if ( waveletismine_ )
	{ deleteAndZeroPtr( wavelet_ ); }
    if ( !wvlt )
	mErrRet(tr("No valid wavelet given"), false);
    if ( pol != OD::CopyPtr )
	wavelet_ = wvlt;
    else
	wavelet_ = new Wavelet( *wvlt );

    waveletismine_ = pol != OD::UsePtr;
    return true;
}


void SynthGenBase::fillPar( IOPar& par ) const
{
    if ( wavelet_ )
    {
	PtrMan<IOObj> wvlobj = Wavelet::getIOObj( wavelet_->name() );
	if ( wvlobj )
	    par.set( sKey::WaveletID(), wvlobj->key() );
    }

    par.setYN( sKeyFourier(), isfourier_ );
    par.setYN( sKeyTimeRefs(), dosampledtimereflectivities_ );
    par.setYN( sKeyNMO(), applynmo_ );
    if ( applynmo_ )
    {
	par.set( sKeyMuteLength(), mutelength_ );
	par.set( sKeyStretchLimit(), stretchlimit_ );
    }
}


bool SynthGenBase::usePar( const IOPar& par )
{
    MultiID waveletid;
    if ( par.get(sKey::WaveletID(),waveletid) )
    {
	waveletismine_ = true;
	IOObj* ioobj = IOM().get( waveletid );
	wavelet_ = Wavelet::get( ioobj );
    }

    par.getYN( sKeyTimeRefs(), dosampledtimereflectivities_ );
    par.getYN( sKeyNMO(), applynmo_ );
    par.getYN( sKeyFourier(), isfourier_ );
    if ( applynmo_ )
    {
	par.get( sKeyStretchLimit(), stretchlimit_ );
	par.get( sKeyMuteLength(), mutelength_ );
    }

    return true;
}


bool SynthGenBase::setOutSampling( const ZSampling& si )
{
    outputsampling_ = si;
    return true;
}


bool SynthGenBase::isInputOK()
{
    if ( !wavelet_ )
	mErrRet(tr("Wavelet required to compute trace range from model(s)"),
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


// SynthGenerator

SynthGenerator::SynthGenerator()
    : SynthGenBase()
{}


SynthGenerator::~SynthGenerator()
{
    delete uncorrsampling_;
}


bool SynthGenerator::areEquivalent( const IOPar& iop, const IOPar& othiop )
{
    PtrMan<SynthGenerator> synthgen1 = create( true );
    PtrMan<SynthGenerator> synthgen2 = create( true );
    if ( !synthgen1 || !synthgen2 )
	return false;

    synthgen1->usePar( iop );
    synthgen2->usePar( othiop );
    return synthgen1->isEquivalent( *synthgen2.ptr() );
}


SynthGenerator* SynthGenerator::create( bool advanced )
{
    SynthGenerator* sg = nullptr;
    const BufferStringSet& fnms = SynthGenerator::factory().getNames();

    if ( !fnms.isEmpty() && advanced )
	sg = factory().create( fnms.get( fnms.size()-1 ) );

    if ( !sg )
	sg = new SynthGenerator();

    return sg;
}


bool SynthGenerator::isEquivalent( const SynthGenerator& oth ) const
{
    if ( isfourier_ != oth.isfourier_ || applynmo_ != oth.applynmo_ )
	return false;

    if ( applynmo_ &&
	 ( !mIsEqual(stretchlimit_,oth.stretchlimit_,1e-5f) ||
	   !mIsEqual(mutelength_,oth.mutelength_,1e-5f)) )
	 return false;

    return true;
}


void SynthGenerator::cleanup()
{
    refmodel_ = nullptr;
    spikestwt_ = nullptr;
    spikescorrectedtwt_ = nullptr;
    outtrc_ = nullptr;
    sampledfreqreflectivities_ = nullptr;
    sampledtimereflectivities_ = nullptr;
    csampledfreqreflectivities_ = nullptr;
    csampledtimereflectivities_ = nullptr;
}


bool SynthGenerator::needSampledReflectivities() const
{
    return outputSampledFreqReflectivities() ||
	   outputSampledTimeReflectivities();
}


bool SynthGenerator::outputSampledFreqReflectivities() const
{
    return isfourier_;
}


bool SynthGenerator::outputSampledTimeReflectivities() const
{
    return !outputSampledFreqReflectivities() &&
	   needSampledTimeReflectivities();
}


bool SynthGenerator::needSampledTimeReflectivities() const
{
    return dosampledtimereflectivities_;
}


void SynthGenerator::setModel( const ReflectivityModelTrace& refmodel,
			       const float* spikestwt,
			       const float* spikescorrectedtwt,
			       SeisTrc& outtrc )
{
    refmodel_ = &refmodel;
    spikestwt_ = spikestwt;
    spikescorrectedtwt_ = spikescorrectedtwt;
    outtrc_ = &outtrc;
}


void SynthGenerator::setSampledFreqReflectivities(
			       ReflectivityModelTrace* reflectivities )
{
    if ( !outputSampledFreqReflectivities() )
	return;

    sampledfreqreflectivities_ = reflectivities;
    csampledfreqreflectivities_ = nullptr;
    useexistingrefs_ = false;
}


void SynthGenerator::setSampledTimeReflectivities(
			       ReflectivityModelTrace* reflectivities )
{
    if ( !needSampledTimeReflectivities() )
	return;

    sampledtimereflectivities_ = reflectivities;
    csampledtimereflectivities_ = nullptr;
    useexistingrefs_ = false;
}


void SynthGenerator::useSampledFreqReflectivities(
				const ReflectivityModelTrace* reflectivities )
{
    if ( !outputSampledFreqReflectivities() )
	return;

    csampledfreqreflectivities_ = reflectivities;
    sampledfreqreflectivities_ = nullptr;
    useexistingrefs_ = true;
}


void SynthGenerator::useSampledTimeReflectivities(
				const ReflectivityModelTrace* reflectivities )
{
    if ( !needSampledTimeReflectivities() )
	return;

    csampledtimereflectivities_ = reflectivities;
    sampledtimereflectivities_ = nullptr;
    useexistingrefs_ = true;
}


bool SynthGenerator::setWavelet( const Wavelet* wvlt, OD::PtrPolicy pol )
{
    freqwavelet_ = nullptr;

    return SynthGenBase::setWavelet( wvlt, pol );
}


#define mPrepFFT( ft, arr, dir, sz )\
    ft->setInputInfo( Array1DInfoImpl(sz) );\
    ft->setDir( dir ); ft->setNormalization( !dir );\
    ft->setInput( arr ); ft->setOutput( arr )

#define mErrOccRet SequentialTask::ErrorOccurred()
#define mMoreToDoRet SequentialTask::MoreToDo()
#define mFinishedRet SequentialTask::Finished()


int SynthGenerator::nextStep()
{
    // Sanity checks
    if ( !wavelet_ )
	mErrRet(tr("Cannot make synthetics without wavelet"), mErrOccRet);

    const int wvltsz = wavelet_->size();
    if ( wvltsz < 2 )
	mErrRet(tr("Wavelet is too short - at minimum 3 samples are required"),
	mErrOccRet);

    if ( !refmodel_ )
	mErrRet(tr("Cannot make synthetics without reflectivity model"),
	mErrOccRet);

    if ( !spikestwt_ || (applynmo_ && !spikescorrectedtwt_) || !outtrc_ )
	mErrRet(tr("Cannot make synthetics without appropriate data"),
	mErrOccRet);

    const bool dosampledrefs = needSampledReflectivities();
    if ( convolvesize_ == 0 && dosampledrefs )
	return setConvolveSize();

    if ( isfourier_ && !freqwavelet_ )
	return genFreqWavelet() ? mMoreToDoRet : mErrOccRet;

    if ( dosampledrefs && !reflectivitiesdone_ )
	return computeReflectivities() ? mMoreToDoRet : mErrOccRet;

    return computeTrace() ? mFinishedRet : mErrOccRet;
}


const ReflectivityModelTrace* SynthGenerator::getSampledFreqReflectivities()
									  const
{
    return hasExistingReflectivities() ? csampledfreqreflectivities_.ptr()
				       : sampledfreqreflectivities_.ptr();
}


ReflectivityModelTrace* SynthGenerator::getSampledFreqReflectivities()
{
    return hasExistingReflectivities() ? nullptr
				       : sampledfreqreflectivities_.ptr();
}


const ReflectivityModelTrace* SynthGenerator::getSampledTimeReflectivities()
									  const
{
    return hasExistingReflectivities() ? csampledtimereflectivities_.ptr()
				       : sampledtimereflectivities_.ptr();
}


ReflectivityModelTrace* SynthGenerator::getSampledTimeReflectivities()
{
    return hasExistingReflectivities() ? nullptr
				       : sampledtimereflectivities_.ptr();
}


const ReflectivitySampler* SynthGenerator::refSampler() const
{
    return refsampler_.ptr();
}


void SynthGenerator::setUnCorrSampling( const ZSampling* uncorrzrg )
{
    delete uncorrsampling_;
    uncorrsampling_ = uncorrzrg ? new ZSampling(*uncorrzrg) : nullptr;
}


int SynthGenerator::setConvolveSize()
{
    const SynthGenerator& cgen = const_cast<const SynthGenerator&>( *this );
    const ReflectivityModelTrace* freqreftrc =
				  cgen.getSampledFreqReflectivities();
    const ReflectivityModelTrace* timereftrc =
				  cgen.getSampledTimeReflectivities();
    if ( (outputSampledFreqReflectivities() && !freqreftrc) ||
	 (outputSampledTimeReflectivities() && !timereftrc) )
	mpErrRet( "Cannot determine convolution size", mErrOccRet );

    convolvesize_ = freqreftrc ? freqreftrc->size() : timereftrc->size();
    creflectivities_ = new ReflectivityModelTrace( convolvesize_ );
    if ( !creflectivities_ || !creflectivities_->isOK() )
	mErrRet(tr("Cannot allocate memory for FFT"), mErrOccRet);

    temprefs_ = creflectivities_->arr();
    refsampler_ = new ReflectivitySampler();

    if ( freqwavelet_ && convolvesize_ != freqwavelet_->size() )
	freqwavelet_ = nullptr;

    return mMoreToDoRet;
}


bool SynthGenerator::genFreqWavelet()
{
    PtrMan<Fourier::CC> fft = Fourier::CC::createDefault();
    if ( !fft )
	mErrRet(tr("Cannot allocate memory for FFT"), false);

    if ( convolvesize_ == 0 )
    {
	const ZSampling& zrg = uncorrsampling_ ? *uncorrsampling_
					       : outputsampling_;
	convolvesize_ = fft->getFastSize( zrg.nrSteps()+1 );
    }

    freqwavelet_ = new ReflectivityModelTrace( convolvesize_ );
    if ( !freqwavelet_ || !freqwavelet_->isOK() )
	mErrRet(tr("Cannot allocate memory for FFT"), false);

    float_complex* freqwavarr = freqwavelet_->arr();

    //TODO add taper if wavelet length less than output trace size
    for ( int idx=0; idx<wavelet_->size(); idx++ )
    {
	int arrpos = idx - wavelet_->centerSample();
	if ( arrpos < 0 )
	    arrpos += convolvesize_;

	freqwavarr[arrpos] = wavelet_->samples()[idx];
    }

    mPrepFFT( fft, freqwavarr, true, convolvesize_ );
    if ( !fft->run(true) )
    {
	freqwavelet_ = nullptr;
	mErrRet(tr("Error running FFT for the wavelet spectrum"), false);
    }

    return true;
}


bool SynthGenerator::computeReflectivities()
{
    if ( hasExistingReflectivities() )
    {
	if ( outputSampledFreqReflectivities() && !csampledfreqreflectivities_ )
	    mpErrRet( "Missing frequency reflectivities cache", false );

	if ( outputSampledTimeReflectivities() && !csampledtimereflectivities_ )
	    mpErrRet( "Missing time reflectivities cache", false );

	reflectivitiesdone_ = true;
	return true;
    }

    if ( outputSampledFreqReflectivities() && !sampledfreqreflectivities_ )
	mpErrRet( "Missing frequency reflectivities data", false );
    if ( outputSampledTimeReflectivities() && !sampledtimereflectivities_ )
	mpErrRet( "Missing time reflectivities data", false );

    if ( !refsampler_ )
	refsampler_ = new ReflectivitySampler();

    const SamplingData<float>& outputsampling = outtrc_->info().sampling;
    const ZSampling sampling = outputsampling.interval( convolvesize_ );
    ReflectivityModelTrace* temprefs = needSampledTimeReflectivities()
				     ? creflectivities_.ptr() : nullptr;

    refsampler_->setInput( *refmodel_.ptr(), spikestwt_, sampling );
    if ( outputSampledFreqReflectivities() )
	refsampler_->setFreqOutput( *sampledfreqreflectivities_.ptr() );
    if ( needSampledTimeReflectivities() )
	refsampler_->setTimeOutput( *sampledtimereflectivities_.ptr(),
				    temprefs->arr(), temprefs->size() );

    const bool isok = refsampler_->executeParallel( false );
    if ( !isok )
	mErrRet( refsampler_->uiMessage(), false );

    if ( outputSampledFreqReflectivities() )
	csampledfreqreflectivities_ = sampledfreqreflectivities_.ptr();
    else if ( outputSampledTimeReflectivities() )
	csampledtimereflectivities_ = sampledtimereflectivities_.ptr();

    reflectivitiesdone_ = true;

    return true;
}


bool SynthGenerator::computeTrace()
{
    SeisTrc& res = *outtrc_;
    res.zero();

    PtrMan<ValueSeries<float> > tmpvs = applynmo_
			? new ArrayValueSeries<float, float>( convolvesize_ )
			: nullptr;

    SeisTrcValueSeries trcvs( res, 0 );
    ValueSeries<float>& vs = tmpvs ? *tmpvs : trcvs;

    if ( isfourier_ )
    {
	if ( !doFFTConvolve(vs,tmpvs ? convolvesize_ : res.size()) )
	    return false;
    }
    else if ( !doTimeConvolve(vs,tmpvs ? convolvesize_ : res.size()) )
	return false;

    if ( applynmo_ )
    {
	if ( !doNMOStretch(vs,convolvesize_,trcvs,res.size()) )
	    return false;
    }

    return true;
}


bool SynthGenerator::doFFTConvolve( ValueSeries<float>& res, int outsz )
{
    float_complex* temprefs = getTempRefs();
    if ( !csampledfreqreflectivities_ || !freqwavelet_ || !temprefs )
	mErrRet(tr("Cannot find data for Frequency convolution"), false)

    PtrMan<Fourier::CC> fft = Fourier::CC::createDefault();
    if ( !fft )
	mErrRet(tr("Cannot allocate memory for FFT"), false)

    ArrayMath::ProdExec<float_complex> prodex( convolvesize_,
					   csampledfreqreflectivities_->arr(),
					   freqwavelet_->arr(),
					   temprefs, true );
    prodex.executeParallel( false );

    mPrepFFT( fft, temprefs, false, convolvesize_ );
    if ( !fft->run(true) )
	mErrRet(tr("Cannot run FFT for convolution"), false)

    sortOutput( getTempRefs(), res, outsz );

    return true;
}


void SynthGenerator::sortOutput( const float_complex* cres,
				 ValueSeries<float>& res, int outsz ) const
{
    const SamplingData<float>& trcsampling = outtrc_->info().sampling;
    const ZSampling twtrg = trcsampling.interval( outsz );
    const float step = trcsampling.step;
    float start = mCast( float, mCast( int, trcsampling.start/step ) ) * step;
    if ( start < trcsampling.start - 1e-4f )
	start += step;

    const float width = step * convolvesize_;
    const int nperiods = mCast( int, Math::Floor( start/width ) ) + 1;
    const SamplingData<float> fftsampling( start, step );

    const float stoptwt = start + width;
    float twt = width * nperiods - step;
    for ( int idx=0; idx<convolvesize_; idx++ )
    {
	twt += step;
	if ( twt > stoptwt - 1e-4f )
	    twt -= width;

	const int idy = trcsampling.nearestIndex( twt );
	if ( idy<0 || idy>outsz-1 )
	    continue;

	res.setValue( idy, cres[idx].real() );
    }
}


bool SynthGenerator::doTimeConvolve( ValueSeries<float>& res, int outsz )
{
    ObjectSet<Array1D<float> > wavelettrcs;
    int nrspikes = 0;
    const bool issampled = outputSampledTimeReflectivities();
    const ReflectivityModelTrace* rm = issampled
				  ? csampledtimereflectivities_.ptr()
				  : refmodel_.ptr();
    const float_complex* refsarr = rm ? rm->arr() : nullptr;
    const float* twtarr = spikestwt_;
    if ( !refsarr || (issampled && !outtrc_) || (!issampled && !twtarr) )
	mErrRet(tr("Cannot find data for Time convolution"), false)

    const SamplingData<float>& trcsampling = outtrc_->info().sampling;
    for ( int iref=0; iref<rm->size(); iref++ )
    {
	const float_complex ref = refsarr[iref];
	const float twt = issampled ? trcsampling.atIndex( iref )
				    : twtarr[iref];
	if ( mIsUdf(ref) || mIsUdf(twt) )
	    continue;

	wavelettrcs += new Array1DImpl<float> ( outsz );
	getWaveletTrace( trcsampling, twt, ref.real(),*wavelettrcs[nrspikes++]);
    }

    Array1DImpl<float> output( outsz );
    Array1DStacker<float, Array1D<float> > stktrcs( wavelettrcs, output );
    if ( !stktrcs.execute() )
	mErrRet( mToUiStringTodo(stktrcs.errMsg()), false )

    output.getAll( res );
    return true;
}


void SynthGenerator::getWaveletTrace( const SamplingData<float>& sampling,
				      float z, float scal,
				      Array1D<float>& trc ) const
{
    const int sz = trc.info().getSize(0);
    for ( int idx=0; idx<sz; idx++ )
    {
	const float twt = sampling.atIndex( idx );
	trc.set( idx, scal * wavelet_->getValue( twt-z ) );
    }
}


bool SynthGenerator::doNMOStretch( const ValueSeries<float>& input, int insz,
				   ValueSeries<float>& out, int outsz ) const
{
    if ( refmodel_->size() < 1 )
	return true;

    const SamplingData<float>& trcsampling = outtrc_->info().sampling;
    const ZSampling outputsampling = trcsampling.interval( outsz );

    float mutelevel = trcsampling.start;
    float firsttime = mUdf(float);

    PointBasedMathFunction stretchfunc( PointBasedMathFunction::Linear,
				      PointBasedMathFunction::ExtraPolGradient);

    for ( int idx=0; idx<refmodel_->size(); idx++ )
    {
	const float spikecorrectedtime = spikescorrectedtwt_[idx];
	const float spiketime = spikestwt_[idx];
	if ( idx>0 )
	{
	    //check for crossing events
	    if ( spiketime < spikestwt_[idx-1] )
		mutelevel = mMAX(spikecorrectedtime, mutelevel );
	}

	firsttime = mMIN( firsttime, spikecorrectedtime );
	stretchfunc.add( spikecorrectedtime, spiketime );
    }

    if ( firsttime > 0.f )
	stretchfunc.add( 0.f, 0.f );

    SampledFunctionImpl<float,ValueSeries<float> > samplfunc( input,
					    insz, trcsampling.start,
					    trcsampling.step );

    for ( int idx=0; idx<outsz; idx++ )
    {
	const float corrtime = trcsampling.atIndex( idx );
	const float uncorrtime = stretchfunc.getValue( corrtime );
	const float stretch = corrtime>0 ? uncorrtime/corrtime -1.f : 0.f;
	if ( stretch>stretchlimit_ )
	    mutelevel = mMAX( corrtime, mutelevel );

	const float outval = samplfunc.getValue( uncorrtime );
	out.setValue( idx, mIsUdf(outval) ? 0.f : outval );
    }

    if ( mutelevel>outputsampling.start )
    {
	Muter muter( mutelength_/outputsampling.step, false );
	muter.mute( out, outtrc_->size(),
		    outputsampling.getfIndex( mutelevel ) );
    }

    return true;
}


bool SynthGenerator::doWork()
{
    reflectivitiesdone_ = false;
    int res = mMoreToDoRet;
    while ( res == mMoreToDoRet )
	res = nextStep();

    cleanup();
    return res == mFinishedRet;
}


#undef mErrRet
#define mErrRet(msg, retval) { msg_ = msg; return retval; }

// MultiTraceSynthGenerator

MultiTraceSynthGenerator::MultiTraceSynthGenerator(
					const SynthGenerator& synthgen,
					const ReflectivityModelBase& refmodel,
					SeisTrcBuf& trcs )
    : defsynthgen_(synthgen)
    , refmodel_(refmodel)
    , trcs_(trcs)
    , totalnr_(trcs.size())
{
}


MultiTraceSynthGenerator::~MultiTraceSynthGenerator()
{
    deepErase( synthgens_ );
}


void MultiTraceSynthGenerator::setSampledReflectivitySet(
						ReflecSet* freqreflectivityset,
						ReflecSet* timereflectivityset )
{
    freqreflectivityset_ = freqreflectivityset;
    timereflectivityset_ = timereflectivityset;
    cfreqreflectivityset_ = nullptr;
    ctimereflectivityset_ = nullptr;
}


void MultiTraceSynthGenerator::useSampledReflectivitySet(
				    const ReflecSet* freqreflectivityset,
				    const ReflecSet* timereflectivityset )
{
    cfreqreflectivityset_ = freqreflectivityset;
    ctimereflectivityset_ = timereflectivityset;
    freqreflectivityset_ = nullptr;
    timereflectivityset_ = nullptr;
}


uiString MultiTraceSynthGenerator::uiMessage() const
{
    return msg_;
}


od_int64 MultiTraceSynthGenerator::nrIterations() const
{ return totalnr_; }


bool MultiTraceSynthGenerator::doPrepare( int nrthreads )
{
    deepErase( synthgens_ );
    IOPar iop;
    defsynthgen_.fillPar( iop );
    for ( int idx=0; idx<nrthreads; idx++ )
    {
	auto* synthgen = SynthGenerator::create( true );
	if ( !synthgen )
	    return false;

	synthgen->usePar( iop );
	if ( defsynthgen_.wavelet_ )
	    synthgen->setWavelet( defsynthgen_.wavelet_, OD::UsePtr );
	if ( defsynthgen_.freqwavelet_.ptr() )
	{
	    auto* freqwvlt =
		const_cast<ReflectivityModelTrace*>(
					defsynthgen_.freqwavelet_.ptr() );
	    synthgen->freqwavelet_ = freqwvlt;;
	}

	synthgen->setUnCorrSampling( defsynthgen_.uncorrsampling_ );
	synthgens_.add( synthgen );
    }

    return true;
}


bool MultiTraceSynthGenerator::doWork(od_int64 start, od_int64 stop, int thread)
{
    SynthGenerator& synthgen = *synthgens_[thread];
    for ( int idx=mCast(int,start); idx<=stop; idx++, addToNrDone(1) )
    {
	const ReflectivityModelTrace* refmodel =
					refmodel_.getReflectivities(idx);
	const float* timesarr_ = refmodel_.getReflTimes( idx );
	const float* correctedtimesarr = refmodel_.getReflTimes();
	if ( !refmodel || !timesarr_ || !correctedtimesarr ||
	     idx >= trcs_.size() )
	    return false;

	synthgen.setModel( *refmodel, timesarr_, correctedtimesarr,
			   *trcs_.get(idx) );
	if ( cfreqreflectivityset_ )
	    synthgen.useSampledFreqReflectivities(
					cfreqreflectivityset_->get(idx) );
	else if ( freqreflectivityset_ )
	    synthgen.setSampledFreqReflectivities(
					freqreflectivityset_->get(idx) );

	if ( ctimereflectivityset_ )
	    synthgen.useSampledTimeReflectivities(
					ctimereflectivityset_->get(idx) );
	else if ( timereflectivityset_ )
	    synthgen.setSampledTimeReflectivities(
					timereflectivityset_->get(idx) );

	if ( !synthgen.doWork() )
	    mErrRet( synthgen.errMsg(), false );
    }

    return true;
}


bool MultiTraceSynthGenerator::doFinish( bool success )
{
    deepErase( synthgens_ );
    return success;
}



ConstRefMan<ReflectivityModelSet>
RaySynthGenerator::getRefModels( const TypeSet<ElasticModel>& emodels,
				 const IOPar& raypars, uiString& msg,
				 TaskRunner* taskrun,
			 const ObjectSet<const TimeDepthModel>* forcedtdmodels )
{
    return RayTracerRunner::getRefModels( emodels, raypars, msg, taskrun,
					  forcedtdmodels );
}



RaySynthGenerator::RaySynthGenerator( const ReflectivityModelSet& refmodels )
    : ParallelTask("Synthetics generation")
    , synthgen_(SynthGenerator::create(true))
    , refmodels_(&refmodels)
{
    msg_ = tr("Generating synthetics");
    refmodels_->getOffsets( offsets_ );
}


RaySynthGenerator::RaySynthGenerator( const SynthGenDataPack& synthresdp )
    : ParallelTask("Synthetics generation")
    , synthgen_(SynthGenerator::create(true))
    , synthresdp_(&synthresdp)
    , refmodels_(&synthresdp.getModels())
{
    msg_ = tr( "Generating synthetics" );
    synthresdp_->getModels().getOffsets( offsets_ );
    if ( synthgen_ )
    {
	synthgen_->setOutSampling( synthresdp.outputsampling_ );
	synthgen_->usePar( synthresdp.synthgenpars_ );
    }
}


RaySynthGenerator::~RaySynthGenerator()
{
    delete synthgen_;
    deepErase( results_ );
}


bool RaySynthGenerator::usePar( const IOPar& par )
{
    if ( !synthgen_ )
	return false;

    synthgen_->usePar( par );

    return true;
}


bool RaySynthGenerator::setWavelet( const Wavelet* wvlt, OD::PtrPolicy pol )
{
    return synthgen_ && synthgen_->setWavelet( wvlt, pol );
}


bool RaySynthGenerator::setOutSampling( const ZSampling& zrg )
{
    return synthgen_ && synthgen_->setOutSampling( zrg );
}


void RaySynthGenerator::setMuteLength( float val )
{
    if ( synthgen_ )
	synthgen_->setMuteLength( val );
}


void RaySynthGenerator::setStretchLimit( float val )
{
    if ( synthgen_ )
	synthgen_->setStretchLimit( val );
}


void RaySynthGenerator::enableFourierDomain( bool yn )
{
    if ( synthgen_ )
	synthgen_->enableFourierDomain( yn );
}


void RaySynthGenerator::doSampledTimeReflectivity( bool yn )
{
    if ( synthgen_ )
	synthgen_->doSampledTimeReflectivity( yn );
}


od_int64 RaySynthGenerator::nrIterations() const
{
    return refmodels_ ? refmodels_->nrModels() : -1;
}


uiString RaySynthGenerator::uiMessage() const
{
    return msg_;
}


uiString RaySynthGenerator::uiNrDoneText() const
{
    return tr("Models done");
}


bool RaySynthGenerator::doPrepare( int /* nrthreads */ )
{
    msg_ = tr("Generating synthetics");
    const int nrmodels = refmodels_ ? refmodels_->nrModels() : 0;
    if ( nrmodels < 1 )
	mErrRet( tr("No models given to make synthetics"), false );

    const int nrtrcsperpos = refmodels_->get(0)->nrRefModels();
    if ( nrtrcsperpos < 1 )
	mErrRet( tr("No reflectivity models given to make synthetics"), false );

    const bool singletrcperpos = nrtrcsperpos == 1;
    if ( offsets_.isEmpty() && singletrcperpos )
	{ pErrMsg("Should not happen"); offsets_ += 0.f; }
    else if ( offsets_.size() != nrtrcsperpos )
	mErrRet( tr("Inconsistent offset/angle distribution"), false );

    const bool zerooffset = singletrcperpos && mIsZero(offsets_[0],1e-1f);
    const bool isoffsetdomain = refmodels_->get(0)->isOffsetDomain();
    rettype_ = isoffsetdomain &&
		(!singletrcperpos || (singletrcperpos && !zerooffset))
	     ? LinePS : Line;

    if ( !synthgen_ || !synthgen_->isInputOK() )
	mErrRet( tr("Incorrect parameters for synthetic generation"), false );

    const bool skipnmo = !Seis::isPS(rettype_) ||
			(Seis::isPS(rettype_) && singletrcperpos && zerooffset);

    ZSampling outputzrg = synthgen_->outputsampling_;
    PtrMan<ZSampling> uncorrsampling;
    if ( !synthresdp_ || outputzrg.isUdf() )
    {
	ZSampling cursampling( outputzrg );
	if ( !skipnmo )
	    uncorrsampling = new ZSampling;

	ZSampling cursamplingtmp( outputzrg );
	getOutSamplingFromModel( cursamplingtmp, nullptr );

	getOutSamplingFromModel( cursampling, uncorrsampling.ptr() );
	outputzrg.include( cursampling, false );
	outputzrg.step = cursampling.step;
	if ( !skipnmo && !synthgen_->applynmo_ )
	    outputzrg = *uncorrsampling.ptr();

	synthgen_->setOutSampling( outputzrg );
	synthgen_->setUnCorrSampling( uncorrsampling.ptr() );
    }

    if ( synthgen_->isfourier_ && !synthgen_->genFreqWavelet() )
    {
	msg_ = synthgen_->errMsg();
	mErrRet(msg_, mErrOccRet);
    }

    deepErase( results_ );
    const ZSampling& uncorrzrg = uncorrsampling ? *uncorrsampling.ptr()
						: outputzrg;
    const bool dosampledfreqrefs = !synthresdp_ &&
				synthgen_->outputSampledFreqReflectivities();
    const bool dosampledtimerefs = !synthresdp_ &&
				synthgen_->needSampledTimeReflectivities();
    for ( int imdl=0; imdl<nrmodels; imdl++ )
    {
	auto* newres = new SynthRes( outputzrg, uncorrzrg, offsets_,
				     imdl, dosampledfreqrefs,dosampledtimerefs);
	if ( !newres || !newres->isOK() )
	{
	    delete newres;
	    return false;
	}

	results_.add( newres );
    }

    return true;
}


bool RaySynthGenerator::doWork( od_int64 start, od_int64 stop,
				int /* threadidx*/ )
{
    if ( !synthgen_ || !refmodels_ ||
	 !results_.validIdx(start) || !results_.validIdx(stop) )
	return false;

    for ( int idx=mCast(int,start); idx<=stop; idx++, addToNrDone(1) )
    {
	if ( !shouldContinue() || !refmodels_->get(idx) )
	    return false;

	SynthRes& res = *results_.get( idx );
	MultiTraceSynthGenerator multitracegen( *synthgen_,
						*refmodels_->get( idx ),
						res.outtrcs_ );
	if ( synthresdp_ )
	{
	    multitracegen.useSampledReflectivitySet(
			    synthresdp_->getSampledReflectivitySet(idx,true),
			    synthresdp_->getSampledReflectivitySet(idx,false) );
	}
	else
	{
	    multitracegen.setSampledReflectivitySet(
				   res.freqsampledreflectivityset_,
				   res.timesampledreflectivityset_ );
	}

	if ( !multitracegen.execute() )
	    mErrRet( multitracegen.uiMessage(), false )
    }

    return true;
}


bool RaySynthGenerator::doFinish( bool success )
{
    if ( !success )
	deepErase( results_ );

    return success;
}


bool RaySynthGenerator::getOutSamplingFromModel( ZSampling& nmozrg,
						 ZSampling* uncorrzrg ) const
{
    const ZSampling wvltzrg = synthgen_->wavelet_->samplePositions();
    const float outputsr = mIsUdf(nmozrg.step) ? wvltzrg.step : nmozrg.step;
    nmozrg.set( mUdf(float), -mUdf(float), outputsr );
    if ( uncorrzrg )
	*uncorrzrg = nmozrg;

    const int nrmodels = refmodels_->nrModels();
    for ( int imdl=0; imdl<nrmodels; imdl++ )
    {
	const ReflectivityModelBase& model = *refmodels_->get( imdl );
	const int nrspikes = model.nrSpikes();
	const float* twtarr = model.getReflTimes();
	for ( int idz=0; idz<nrspikes; idz++ )
	{
	    if ( !model.isSpikeDefined(0,idz) )
		continue;

	    const float twt = twtarr[idz];
	    nmozrg.include( twt, false );
	    break;
	}

	for ( int idz=nrspikes-1; idz>=0; idz-- )
	{
	    if ( !model.isSpikeDefined(0,idz) )
		continue;

	    const float twt = twtarr[idz];
	    nmozrg.include( twt, false );
	    break;
	}

	if ( !uncorrzrg )
	    continue;

	for ( int ioff=0; ioff<model.nrRefModels(); ioff++ )
	{
	    const float* uncorrarr = model.getReflTimes( ioff );
	    for ( int idz=0; idz<nrspikes; idz++ )
	    {
		if ( model.isSpikeDefined(ioff,idz) )
		    uncorrzrg->include( uncorrarr[idz], false );
	    }
	}
    }

    const bool hassampling = !nmozrg.isUdf();
    if ( !hassampling )
    {
	for ( int imdl=0; imdl<nrmodels; imdl++ )
	{
	    const ReflectivityModelBase& model = *refmodels_->get( imdl );
	    const TimeDepthModel& tdmodel = model.getDefaultModel();
	    nmozrg.include( tdmodel.getFirstTime() );
	    nmozrg.include( tdmodel.getLastTime() );
	    if ( !uncorrzrg )
		continue;

	    for ( int ioff=0; ioff<model.nrRefModels(); ioff++ )
	    {
		const TimeDepthModel* t2dmodel = model.get( ioff );
		if ( !t2dmodel )
		    continue;

		uncorrzrg->include( t2dmodel->getFirstTime() );
		uncorrzrg->include( t2dmodel->getLastTime() );
	    }
	}

	if ( nmozrg.isUdf() )
	    mErrRet( tr("Cannot determine trace size from model(s)"), false );
    }

    ObjectSet<ZSampling> zrgs( &nmozrg );
    if ( uncorrzrg )
    {
	uncorrzrg->start = nmozrg.start;
	zrgs.add( uncorrzrg );
    }

    for ( auto* obj : zrgs )
    {
	ZSampling& zrg = *obj;
	zrg.scale( 1.f / outputsr );
	zrg.start = mIsEqual( (float)mNINT32(zrg.start), zrg.start,
				1e-2f )
		      ? mNINT32(zrg.start) : Math::Floor( zrg.start);
	zrg.stop = mIsEqual( (float)mNINT32(zrg.stop), zrg.stop,
			       1e-2f )
		     ? mNINT32(zrg.stop) : Math::Ceil( zrg.stop );
	zrg.scale( outputsr );
	if ( hassampling )
	{
	    zrg.start += wvltzrg.start;
	    zrg.stop += wvltzrg.stop;
	}

	zrg.step = outputsr;
    }

    return true;
}


SeisTrc* RaySynthGenerator::stackedTrc( int imdl, bool cansteal ) const
{
    if ( !results_.validIdx(imdl) )
	return nullptr;

    const SynthRes& result = *results_.get( imdl );
    const SeisTrcBuf& outtrcs = result.outtrcs_;
    if ( outtrcs.isEmpty() )
	return nullptr;

    if ( cansteal && outtrcs.size() == 1 )
	return const_cast<SeisTrcBuf&>( outtrcs ).remove(0);

    auto* trc = new SeisTrc( *outtrcs.first() );
    SeisTrcPropChg stckr( *trc );
    for ( int idx=1; idx<outtrcs.size(); idx++ )
    {
	const SeisTrc* outtrc = outtrcs.get( idx );
	if ( !outtrc || outtrc->isNull() )
	    continue;

	stckr.stack( *outtrc, false, mCast(float,idx) );
    }

    return trc;
}


bool RaySynthGenerator::getStackedTraces( SeisTrcBuf& seisbuf,
					  bool steal ) const
{
    if ( results_.size() != totalNr() )
	return false;

#ifdef __debug__
    if ( !seisbuf.isOwner() )
	{ pErrMsg("Should own the traces"); DBG::forceCrash(true); }
#endif

    seisbuf.erase();
    for ( int idx=0; idx<results_.size(); idx++ )
    {
	SeisTrc* trc = stackedTrc( idx, steal );
	if ( !trc )
	    return false;

	seisbuf.add( trc );
    }

    return true;
}


bool RaySynthGenerator::getTraces( SeisTrcBuf& seisbuf, int imdl,
				   bool steal ) const
{
    if ( !results_.validIdx(imdl) )
	return false;

    const SynthRes& result = *results_.get( imdl );
    if ( steal )
	seisbuf.stealTracesFrom( result.outtrcs_ );
    else
	result.outtrcs_.copyInto( seisbuf );

    return true;
}


bool RaySynthGenerator::getTraces( ObjectSet<SeisTrcBuf>& seisbufs,
				   bool steal ) const
{
    for ( int idx=0; idx<totalNr(); idx++ )
    {
	auto* tbuf = new SeisTrcBuf( true );
	if ( !getTraces(*tbuf,idx,steal) )
	    return false;

	seisbufs += tbuf;
    }

    return true;
}


const SynthGenDataPack* RaySynthGenerator::getAllResults()
{
    if ( synthresdp_ )
	return synthresdp_.ptr();

    auto* ret = new SynthGenDataPack( *refmodels_.ptr(), rettype_, offsets_,
				      synthgen_->outputsampling_ );
    synthgen_->fillPar( ret->synthgenpars_ );
    if ( !synthgen_->needSampledReflectivities() )
	return ret;

    if ( synthgen_->outputSampledFreqReflectivities() )
	ret->freqsampledrefset_ = new ObjectSet<const ReflecSet>;
    if ( synthgen_->outputSampledTimeReflectivities() )
	ret->timesampledrefset_ = new ObjectSet<const ReflecSet>;

    for ( int imdl=0; imdl<refmodels_->nrModels(); imdl++ )
    {
	SynthRes& res = *results_.get( imdl );
	if ( ret->freqsampledrefset_ && res.freqsampledreflectivityset_ )
	{
	    ret->freqsampledrefset_->add( res.freqsampledreflectivityset_ );
	    res.freqsampledreflectivityset_ = nullptr;
	}

	if ( ret->timesampledrefset_ && res.timesampledreflectivityset_ )
	{
	    ret->timesampledrefset_->add( res.timesampledreflectivityset_ );
	    res.timesampledreflectivityset_ = nullptr;
	}
    }

    return ret;
}


RaySynthGenerator::SynthRes::SynthRes( const ZSampling& outputzrg,
				       const ZSampling& uncorrzrg,
				       const TypeSet<float>& offsets,
				       int seqnr,
				       bool dosampledfreqrefs,
				       bool dosampledtimerefs )
    : outtrcs_(*new SeisTrcBuf(true))
{
    const int nrtrcs = offsets.size();
    setTraces( outputzrg, offsets, seqnr );

    if ( !dosampledfreqrefs && !dosampledtimerefs )
	return;

    PtrMan<Fourier::CC> fft = new Fourier::CC;
    if ( !fft )
    {
	pErrMsg("Cannot determine convolution size without FFT transform");
	return;
    }

    const int fftsz = fft->getFastSize( uncorrzrg.nrSteps()+1 );
    if ( dosampledfreqrefs )
    {
	delete freqsampledreflectivityset_;
	freqsampledreflectivityset_ = new ReflecSet;
	for ( int itrc=0; itrc<nrtrcs; itrc++ )
	    freqsampledreflectivityset_->add(new ReflectivityModelTrace(fftsz));
    }

    if ( dosampledtimerefs )
    {
	delete timesampledreflectivityset_;
	timesampledreflectivityset_ = new ReflecSet;
	for ( int itrc=0; itrc<nrtrcs; itrc++ )
	    timesampledreflectivityset_->add(new ReflectivityModelTrace(fftsz));
    }
}


RaySynthGenerator::SynthRes::~SynthRes()
{
    delete &outtrcs_;
    delete freqsampledreflectivityset_;
    delete timesampledreflectivityset_;
}


bool RaySynthGenerator::SynthRes::isOK() const
{
    if ( outtrcs_.size() < 1 )
	return false;

    if ( freqsampledreflectivityset_ )
    {
	if ( freqsampledreflectivityset_->size() != outtrcs_.size() )
	    return false;

	for ( const auto* reflectivities : *freqsampledreflectivityset_ )
	{
	    if ( !reflectivities->isOK() )
		return false;
	}
    }

    if ( timesampledreflectivityset_ )
    {
	if ( timesampledreflectivityset_->size() != outtrcs_.size() )
	    return false;

	for ( const auto* reflectivities : *timesampledreflectivityset_ )
	{
	    if ( !reflectivities->isOK() )
		return false;
	}
    }

    return true;
}


void RaySynthGenerator::SynthRes::setTraces( const ZSampling& zrg,
				const TypeSet<float>& offsets, int seqnr )
{
    const int nrtrcs = offsets.size();
    const int trcsz = zrg.nrSteps()+1;
    for ( int itrc=0; itrc<nrtrcs; itrc++ )
    {
	auto* newtrc = new SeisTrc( trcsz );
	SeisTrcInfo& info = newtrc->info();
	info.setGeomSystem( OD::GeomSynth ).setTrcNr( seqnr+1 ).calcCoord();
	info.sampling = zrg;
	info.offset = offsets[itrc];
	info.seqnr_ = seqnr+1;
	outtrcs_.add( newtrc );
    }
}

} // namespace Seis
