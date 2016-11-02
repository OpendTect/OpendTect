/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : SynthSeis
-*/


#include "synthseis.h"

#include "arrayndimpl.h"
#include "arrayndstacker.h"
#include "factory.h"
#include "fourier.h"
#include "muter.h"
#include "reflectivitysampler.h"
#include "reflectivitymodel.h"
#include "samplfunc.h"
#include "seistrc.h"
#include "waveletmanager.h"

namespace Seis
{

#define mErrRet(msg, retval) { errmsg_ = msg; return retval; }
#define mpErrRet(msg, retval) { pErrMsg(msg); return retval; }

mImplFactory( SynthGenerator, SynthGenerator::factory );

SynthGenBase::SynthGenBase()
    : wavelet_(0)
    , isfourier_(true)
    , stretchlimit_( cStdStretchLimit() ) //20%
    , mutelength_( cStdMuteLength() )  //20ms
    , applynmo_(false)
    , outputsampling_(mUdf(float),-mUdf(float),mUdf(float))
    , dointernalmultiples_(false)
    , dosampledreflectivities_(false)
    , surfreflcoeff_(1)
{}


SynthGenBase::~SynthGenBase()
{
}


void SynthGenBase::setWavelet( const Wavelet* wvlt )
{
    wavelet_ = wvlt;
}


void SynthGenBase::fillPar( IOPar& par ) const
{
    if ( wavelet_ )
	par.set( sKey::WaveletID(), WaveletMGR().getID(*wavelet_) );
    else
	par.removeWithKey( sKey::WaveletID() );

    par.setYN( sKeyFourier(), isfourier_ );
    par.setYN( sKeyNMO(), applynmo_ );
    par.setYN( sKeyInternal(), dointernalmultiples_ );
    par.set( sKeySurfRefl(), surfreflcoeff_ );
    par.set( sKeyMuteLength(), mutelength_ );
    par.set( sKeyStretchLimit(), stretchlimit_ );
}


bool SynthGenBase::usePar( const IOPar& par )
{
    DBKey waveletid;
    if ( par.get(sKey::WaveletID(),waveletid) )
	wavelet_ = WaveletMGR().fetch( waveletid );

    const bool doint = par.getYN( sKeyInternal(), dointernalmultiples_ );
    if ( doint )
	dosampledreflectivities_ = dointernalmultiples_;

    return par.getYN( sKeyNMO(), applynmo_ )
	&& par.getYN( sKeyFourier(), isfourier_ )
	&& doint
        && par.get( sKeySurfRefl(), surfreflcoeff_ )
	&& par.get( sKeyStretchLimit(), stretchlimit_)
        && par.get( sKeyMuteLength(), mutelength_ );
}


bool SynthGenBase::setOutSampling( const StepInterval<float>& si )
{
    outputsampling_ = si;
    return true;
}


bool SynthGenBase::isInputOK()
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


bool SynthGenBase::getOutSamplingFromModel(
				const RefMan<ReflectivityModelSet>& models,
				StepInterval<float>& sampling, bool usenmo )
{
    const float outputsr = mIsUdf(outputsampling_.step) ? wavelet_->sampleRate()
							: outputsampling_.step;
    sampling.set( mUdf(float), -mUdf(float), outputsr );
    for ( int imod=0; imod<models->size(); imod++ )
    {
	const ReflectivityModel& model = *models->get( imod );
	for ( int idz=0; idz<model.size(); idz++ )
	{
	    const ReflectivitySpike& spike = model[idz];
	    if ( !spike.isDefined() )
		continue;

	    const float twt = spike.time( usenmo );
	    sampling.include( twt, false );
	    if ( usenmo )
		break;
	}
	if ( !usenmo )
	    continue;

	for ( int idz=model.size()-1; idz>=0; idz-- )
	{
	    const ReflectivitySpike& spike = model[idz];
	    if ( !spike.isDefined() )
		continue;

	    const float twt = spike.time( usenmo );
	    sampling.include( twt, false );
	    break;
	}
    }

    if ( sampling.isUdf() )
	mErrRet(uiStrings::phrCannotCompute(tr("trace size from models")), 
									false)

    sampling.scale( 1.f / outputsr );
    sampling.start = mIsEqual( (float)mNINT32(sampling.start), sampling.start,
				1e-2f )
		      ? mNINT32(sampling.start) : Math::Floor( sampling.start);
    sampling.stop = mIsEqual( (float)mNINT32(sampling.stop), sampling.stop,
			       1e-2f )
		     ? mNINT32(sampling.stop) : Math::Ceil( sampling.stop );
    sampling.scale( outputsr );
    sampling.start += wavelet_->samplePositions().start;
    sampling.stop += wavelet_->samplePositions().stop;
    sampling.step = outputsr;

    return true;
}



SynthGenerator::SynthGenerator()
    : outtrc_(*new SeisTrc)
    , refmodel_(0)
    , progress_(-1)
    , convolvesize_(0)
{}


SynthGenerator::~SynthGenerator()
{
    freqwavelet_.erase();
    freqreflectivities_.erase();

    delete &outtrc_;
}


SynthGenerator* SynthGenerator::create( bool advanced )
{
    SynthGenerator* sg = 0;
    const BufferStringSet& fnms = SynthGenerator::factory().getNames();

    if ( !fnms.isEmpty() && advanced )
	sg = factory().create( fnms.get( fnms.size()-1 ) );

    if ( !sg )
	sg = new SynthGenerator();

    return sg;
}


bool SynthGenerator::setModel( const ReflectivityModel& refmodel )
{
    refmodel_ = &refmodel;

    freqreflectivities_.erase();
    convolvesize_ = 0;

    return true;
}


void SynthGenerator::setWavelet( const Wavelet* wvlt )
{
    freqwavelet_.erase();
    SynthGenBase::setWavelet( wvlt );
}


bool SynthGenerator::setOutSampling( const StepInterval<float>& si )
{
    SynthGenBase::setOutSampling( si );
    outtrc_.reSize( si.nrSteps()+1, false );
    outtrc_.info().sampling_ = si;

    convolvesize_ = 0;
    freqwavelet_.erase();

    return true;
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
	mErrRet(uiStrings::phrCannotCreate(tr("synthetics without wavelet")), 
								    mErrOccRet);
    const int wvltsz = wavelet_->size();
    if (wvltsz < 2)
	mErrRet(tr("Wavelet is too short - at minimum 3 samples are required"),
								    mErrOccRet);

    if (!refmodel_)
	mErrRet(uiStrings::phrCannotCreate(
		      tr("synthetics without reflectivity model")), mErrOccRet);

    if ( convolvesize_ == 0 )
	return setConvolveSize();

    if ( isfourier_ && freqwavelet_.isEmpty() )
	return genFreqWavelet();

    if ( freqreflectivities_.isEmpty() )
	return computeReflectivities() ? mMoreToDoRet : mErrOccRet;

    return computeTrace( outtrc_ ) ? mFinishedRet : mErrOccRet;
}


void SynthGenerator::getSampledRM( ReflectivityModel& sampledrefmodel ) const
{
    sampledrefmodel = sampledrefmodel_;
}


int SynthGenerator::setConvolveSize()
{
    StepInterval<float> cursampling( outputsampling_ );
    if ( cursampling.isUdf() || applynmo_ )
    {
	RefMan<ReflectivityModelSet> mod = new ReflectivityModelSet;
	mod->add( refmodel_ );
	if ( mod->isEmpty() )
	    mpErrRet( "No models given to make synthetics", false );

	if ( !SynthGenBase::isInputOK() )
	    return mErrOccRet;

	if ( !SynthGenBase::getOutSamplingFromModel(mod,cursampling) )
	    mErrRet(uiStrings::phrCannotCompute(tr("trace size from model")),
								mErrOccRet)

	cursampling.start = outputsampling_.start;
    }
    else
    {
	cursampling.include( outputsampling_ );
	outputsampling_ = cursampling;
    }

    const int oldconvsize = applynmo_ ? cursampling.nrSteps() + 1
				      : outputsampling_.nrSteps() + 1;
    if ( isfourier_ )
    {
	PtrMan<Fourier::CC> fft = Fourier::CC::createDefault();
	convolvesize_ = fft->getFastSize( oldconvsize );
    }
    else
	convolvesize_ = oldconvsize;


    if ( convolvesize_ < oldconvsize || convolvesize_ < 1 )
	mpErrRet( "Cannot determine convolution size", mErrOccRet );

    return mMoreToDoRet;
}


int SynthGenerator::genFreqWavelet()
{
    PtrMan<Fourier::CC> fft = Fourier::CC::createDefault();

    freqwavelet_.setSize( convolvesize_, float_complex(0,0) );
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

    mPrepFFT( fft, freqwavelet_.arr(), true, convolvesize_ );
    if ( !fft->run(true) )
    {
	freqwavelet_.erase();
	mErrRet(tr("Error running FFT for the wavelet spectrum"), mErrOccRet);
    }

    return mMoreToDoRet;
}


bool SynthGenerator::computeTrace( SeisTrc& res )
{
    res.zero();

    PtrMan<ValueSeries<float> > tmpvs = applynmo_
	? new ArrayValueSeries<float, float>( convolvesize_ )
	: 0;

    SeisTrcValueSeries trcvs( res, 0 );
    ValueSeries<float>& vs = tmpvs ? *tmpvs : trcvs;

    if ( isfourier_ )
    {
	if ( !doFFTConvolve( vs, tmpvs ? convolvesize_ : outtrc_.size() ) )
	    return false;
    }
    else if ( !doTimeConvolve( vs, tmpvs ? convolvesize_ : outtrc_.size() ) )
	return false;

    if ( applynmo_ )
    {
	if ( !doNMOStretch( vs, convolvesize_, trcvs, outtrc_.size() ))
	    return false;
    }

    return true;
}


bool SynthGenerator::doNMOStretch(const ValueSeries<float>& input, int insz,
				  ValueSeries<float>& out, int outsz ) const
{
    out.setAll( 0 );

    if ( !refmodel_->size() )
	return true;

    float mutelevel = outputsampling_.start;
    float firsttime = mUdf(float);

    PointBasedMathFunction stretchfunc( PointBasedMathFunction::Linear,
				      PointBasedMathFunction::ExtraPolGradient);

    for ( int idx=0; idx<refmodel_->size(); idx++ )
    {
	const ReflectivitySpike& spike = (*refmodel_)[idx];

	if ( idx>0 )
	{
	    //check for crossing events
	    const ReflectivitySpike& spikeabove = (*refmodel_)[idx-1];
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

    outtrc_.info().sampling_.indexOnOrAfter( mutelevel );

    SampledFunctionImpl<float,ValueSeries<float> > samplfunc( input,
	    insz, outputsampling_.start,
	    outputsampling_.step );

    for ( int idx=0; idx<outsz; idx++ )
    {
	const float corrtime = outtrc_.info().sampling_.atIndex( idx );
	const float uncorrtime = stretchfunc.getValue( corrtime );

	const float stretch = corrtime>0
	    ? uncorrtime/corrtime -1
	    : 0;

	if ( stretch>stretchlimit_ )
	    mutelevel = mMAX( corrtime, mutelevel );

	const float outval = samplfunc.getValue( uncorrtime );
	out.setValue( idx, mIsUdf(outval) ? 0 : outval );
    }


    if ( mutelevel>outputsampling_.start )
    {
	Muter muter( mutelength_/outputsampling_.step, false );
	muter.mute( out, outtrc_.size(),
		    outputsampling_.getfIndex( mutelevel ) );
    }

    return true;
}


bool SynthGenerator::doFFTConvolve( ValueSeries<float>& res, int outsz )
{
    PtrMan<Fourier::CC> fft = Fourier::CC::createDefault();
    if ( !fft )
	mErrRet(tr("Cannot allocate memory for FFT"), false)

	mAllocLargeVarLenArr(float_complex, cres, convolvesize_);
    if ( !cres )
	mErrRet(tr("Cannot allocate memory for FFT"), false)

	for (int idx = 0; idx<convolvesize_; idx++)
	    cres[idx] = freqreflectivities_[idx] * freqwavelet_[idx];

    mPrepFFT(fft, cres, false, convolvesize_);
    if ( !fft->run(true) )
	mErrRet(tr("Cannot run FFT for convolution"), false)

    sortOutput( cres, res, outsz );

    return true;
}


bool SynthGenerator::doTimeConvolve( ValueSeries<float>& res, int outsz )
{
    ObjectSet<Array1D<float> > wavelettrcs;
    int nrspikes = 0;
    const ReflectivityModel& rm =
	!sampledrefmodel_.isEmpty() ? sampledrefmodel_ : *refmodel_;
    for ( int iref=0; iref<rm.size(); iref++ )
    {
	const ReflectivitySpike& spike = rm[iref];
	if ( !spike.isDefined() )
	    continue;

	wavelettrcs += new Array1DImpl<float> ( outsz );
	getWaveletTrace( *wavelettrcs[nrspikes], spike.time_,
			 spike.reflectivity_.real(), outtrc_.info().sampling_ );
	nrspikes++;
    }

    Array1DImpl<float> output( outsz );
    Array1DStacker<float, Array1D<float> > stktrcs( wavelettrcs, output );
    if ( !stktrcs.execute() )
	mErrRet( stktrcs.errMsg(), false )

    output.getAll( res );
    return true;
}


void SynthGenerator::getWaveletTrace( Array1D<float>& trc, float z,
				      float scal,
				      SamplingData<float>& sampling ) const
{
    const int sz = trc.info().getSize(0);
    for ( int idx=0; idx<sz; idx++ )
    {
	const float twt = sampling.atIndex( idx );
	trc.set( idx, scal * wavelet_->getValue( twt-z ) );
    }
}


void SynthGenerator::sortOutput( float_complex* cres, ValueSeries<float>& res,
				 int outsz ) const
{
    const float step = outtrc_.info().sampling_.step;
    float start = mCast( float, mCast( int, outtrc_.startPos()/step ) ) * step;
    if ( start < outtrc_.startPos() - 1e-4f )
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
	twt = outtrc_.samplePos( idx );
	res.setValue( idx, fftout.getValue( twt, 0 ) );
    }
}


bool SynthGenerator::computeReflectivities()
{
    const StepInterval<float> sampling( outputsampling_.start,
	    outputsampling_.atIndex( convolvesize_-1 ), outputsampling_.step );

    ReflectivitySampler sampler( *refmodel_, sampling, freqreflectivities_ );
    bool isok = sampler.execute();
    if ( !isok )
	mErrRet( sampler.uiMessage(), false );

    if ( dosampledreflectivities_ )
	sampler.getReflectivities( sampledrefmodel_ );

    return true;
}


bool SynthGenerator::doWork()
{
    int res = mMoreToDoRet;
    while ( res == mMoreToDoRet )
	res = nextStep();

    return res == mFinishedRet;
}


//MultiTraceSynthGenerator
MultiTraceSynthGenerator::MultiTraceSynthGenerator()
    : totalnr_( -1 )
    , sampledrefmodels_(new ReflectivityModelSet)
{
}


MultiTraceSynthGenerator::~MultiTraceSynthGenerator()
{
    deepErase( synthgens_ );
    deepErase( trcs_ );
}


bool MultiTraceSynthGenerator::doPrepare( int nrthreads )
{
    for ( int idx=0; idx<nrthreads; idx++ )
    {
	SynthGenerator* synthgen = SynthGenerator::create( true );
	synthgens_ += synthgen;
    }

    return true;
}


void MultiTraceSynthGenerator::setModels(
				RefMan<ReflectivityModelSet>& refmodels )
{
    totalnr_ = 0;
    models_ = refmodels;
    for ( int idx=0; idx<refmodels->size(); idx++ )
	totalnr_ += refmodels[idx].size();
}


od_int64 MultiTraceSynthGenerator::nrIterations() const
{ return models_->size(); }


bool MultiTraceSynthGenerator::doWork(od_int64 start, od_int64 stop, int thread)
{
    SynthGenerator& synthgen = *synthgens_[thread];
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	synthgen.setModel( *(*models_)[idx] );

	IOPar par; fillPar( par ); synthgen.usePar( par );
	if ( wavelet_ )
	    synthgen.setWavelet( wavelet_ );
	synthgen.setOutSampling( outputsampling_ );
	if ( !synthgen.doWork() )
	    mErrRet( synthgen.errMsg(), false );

	Threads::Locker lckr( lock_ );
	trcs_ += new SeisTrc( synthgen.result() );
	ReflectivityModel* sampledrefmodel = new ReflectivityModel();
	synthgen.getSampledRM( *sampledrefmodel );
	sampledrefmodels_->add( sampledrefmodel );
	trcidxs_ += idx;
	lckr.unlockNow();

	addToNrDone( mCast(int,synthgen.currentProgress()) );
    }
    return true;
}


void MultiTraceSynthGenerator::getResult( ObjectSet<SeisTrc>& trcs )
{
    TypeSet<int> sortidxs;
    for ( int idtrc=0; idtrc<trcs_.size(); idtrc++ )
	sortidxs += idtrc;
    sort_coupled( trcidxs_.arr(), sortidxs.arr(), trcidxs_.size() );
    for ( int idtrc=0; idtrc<trcs_.size(); idtrc++ )
	trcs += trcs_[ sortidxs[idtrc] ];

    trcs_.erase();
}


void MultiTraceSynthGenerator::getSampledRMs(
				RefMan<ReflectivityModelSet>& sampledrms )
{
    TypeSet<int> sortidxs;
    for ( int idtrc=0; idtrc<sampledrefmodels_->size(); idtrc++ )
	sortidxs += idtrc;
    sort_coupled( trcidxs_.arr(), sortidxs.arr(), trcidxs_.size() );
    for ( int irm=0; irm<sampledrefmodels_->size(); irm++ )
    {
	const ReflectivityModel* model = sampledrefmodels_->get(sortidxs[irm]);
	sampledrms->add( model );
    }
}

} // namespace Seis
