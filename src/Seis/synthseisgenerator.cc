/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 23-3-1996
 * FUNCTION : SynthSeis
-*/


#include "synthseisgenerator.h"

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
#include "keystrs.h"


#define mErrRet(msg, retval) { errmsg_ = msg; return retval; }
#define mpErrRet(msg, retval) { pErrMsg(msg); return retval; }

mImplClassFactory( SynthSeis::Generator, factory );

SynthSeis::GenBase::GenBase()
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


SynthSeis::GenBase::~GenBase()
{
}


void SynthSeis::GenBase::setWavelet( const Wavelet* wvlt )
{
    wavelet_ = wvlt;
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
    par.set( sKeySurfRefl(), surfreflcoeff_ );
    par.set( sKeyMuteLength(), mutelength_ );
    par.set( sKeyStretchLimit(), stretchlimit_ );
}


bool SynthSeis::GenBase::usePar( const IOPar& par )
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


bool SynthSeis::GenBase::setOutSampling( const StepInterval<float>& si )
{
    outputsampling_ = si;
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


bool SynthSeis::GenBase::getOutSamplingFromModels(
				const RflMdlSetRef& models,
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
	mErrRet(uiStrings::phrCannotCalculate(tr("trace size from models")),
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



SynthSeis::Generator::Generator()
    : outtrc_(*new SeisTrc)
    , reflmodel_(0)
    , progress_(-1)
    , convolvesize_(0)
{}


SynthSeis::Generator::~Generator()
{
    freqwavelet_.erase();
    freqreflectivities_.erase();

    delete &outtrc_;
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


bool SynthSeis::Generator::setModel( const ReflectivityModel& refmodel )
{
    reflmodel_ = &refmodel;

    freqreflectivities_.erase();
    convolvesize_ = 0;

    return true;
}


void SynthSeis::Generator::setWavelet( const Wavelet* wvlt )
{
    freqwavelet_.erase();
    GenBase::setWavelet( wvlt );
}


bool SynthSeis::Generator::setOutSampling( const StepInterval<float>& si )
{
    GenBase::setOutSampling( si );
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


int SynthSeis::Generator::nextStep()
{
    // Sanity checks
    if ( !wavelet_ )
	mErrRet(uiStrings::phrCannotCreate(tr("synthetics without wavelet")),
								    mErrOccRet);
    const int wvltsz = wavelet_->size();
    if (wvltsz < 2)
	mErrRet(tr("Wavelet is too short - at minimum 3 samples are required"),
								    mErrOccRet);

    if ( !reflmodel_ )
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


void SynthSeis::Generator::getSampledRM( ReflectivityModel& srefmdl ) const
{
    srefmdl = sampledreflmodel_;
}


int SynthSeis::Generator::setConvolveSize()
{
    StepInterval<float> cursampling( outputsampling_ );
    if ( cursampling.isUdf() || applynmo_ )
    {
	if ( !reflmodel_ )
	    mpErrRet( "No models given to make synthetics", false );
	if ( !isInputOK() )
	    return mErrOccRet;

	RefMan<ReflectivityModelSet> modls = new ReflectivityModelSet;
	modls->add( reflmodel_ );
	if ( !getOutSamplingFromModels(modls,cursampling) )
	    mErrRet(uiStrings::phrCannotCalculate(tr("trace size from model")),
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


int SynthSeis::Generator::genFreqWavelet()
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
	mErrRet(tr("Could not run FFT for the wavelet spectrum"), mErrOccRet);
    }

    return mMoreToDoRet;
}


bool SynthSeis::Generator::computeTrace( SeisTrc& res )
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


bool SynthSeis::Generator::doNMOStretch(const ValueSeries<float>& input,
			int insz, ValueSeries<float>& out, int outsz ) const
{
    out.setAll( 0 );

    if ( reflmodel_->isEmpty() )
	return true;

    float mutelevel = outputsampling_.start;
    float firsttime = mUdf(float);

    PointBasedMathFunction stretchfunc( PointBasedMathFunction::Linear,
				      PointBasedMathFunction::ExtraPolGradient);

    for ( int idx=0; idx<reflmodel_->size(); idx++ )
    {
	const ReflectivitySpike& spike = (*reflmodel_)[idx];

	if ( idx>0 )
	{
	    //check for crossing events
	    const ReflectivitySpike& spikeabove = (*reflmodel_)[idx-1];
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


bool SynthSeis::Generator::doFFTConvolve( ValueSeries<float>& res, int outsz )
{
    PtrMan<Fourier::CC> fft = Fourier::CC::createDefault();
    if ( !fft )
	mErrRet(tr("Cannot allocate memory for FFT"), false)

    mAllocLargeVarLenArr(float_complex, cres, convolvesize_);
    if ( !cres )
	mErrRet(tr("Cannot allocate memory for FFT"), false)

    for ( int idx=0; idx<convolvesize_; idx++ )
	cres[idx] = freqreflectivities_[idx] * freqwavelet_[idx];

    mPrepFFT(fft, cres, false, convolvesize_);
    if ( !fft->run(true) )
	mErrRet(tr("Cannot run FFT for convolution"), false)

    sortOutput( cres, res, outsz );

    return true;
}


bool SynthSeis::Generator::doTimeConvolve( ValueSeries<float>& res, int outsz )
{
    const ReflectivityModel& rm =
	!sampledreflmodel_.isEmpty() ? sampledreflmodel_ : *reflmodel_;
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
			 outtrc_.info().sampling_ );
	trcstacker.addInput( newtrc );
    }

    if ( !trcstacker.execute() )
	mErrRet( trcstacker.errMsg(), false )

    output.getAll( res );
    return true;
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


void SynthSeis::Generator::sortOutput( float_complex* cres,
				    ValueSeries<float>& res, int outsz ) const
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


bool SynthSeis::Generator::computeReflectivities()
{
    const StepInterval<float> sampling( outputsampling_.start,
	    outputsampling_.atIndex( convolvesize_-1 ), outputsampling_.step );

    ReflectivitySampler sampler( *reflmodel_, sampling, freqreflectivities_ );
    bool isok = sampler.execute();
    if ( !isok )
	mErrRet( sampler.message(), false );

    if ( dosampledreflectivities_ )
	sampler.getReflectivities( sampledreflmodel_ );

    return true;
}


bool SynthSeis::Generator::doWork()
{
    int res = mMoreToDoRet;
    while ( res == mMoreToDoRet )
	res = nextStep();

    return res == mFinishedRet;
}


SynthSeis::MultiTraceGenerator::MultiTraceGenerator()
    : totalnr_( -1 )
    , sampledreflmodels_(new ReflectivityModelSet)
{
}


SynthSeis::MultiTraceGenerator::~MultiTraceGenerator()
{
    deepErase( generators_ );
    deepErase( trcs_ );
}


bool SynthSeis::MultiTraceGenerator::doPrepare( int nrthreads )
{
    for ( int idx=0; idx<nrthreads; idx++ )
	generators_ += Generator::create( true );

    return true;
}


void SynthSeis::MultiTraceGenerator::setModels(
				RefMan<ReflectivityModelSet>& refmodels )
{
    totalnr_ = 0;
    models_ = refmodels;
    for ( int idx=0; idx<refmodels->size(); idx++ )
	totalnr_ += refmodels[idx].size();
}


od_int64 SynthSeis::MultiTraceGenerator::nrIterations() const
{ return models_->size(); }


bool SynthSeis::MultiTraceGenerator::doWork( od_int64 start, od_int64 stop,
					     int thread )
{
    Generator& generator = *generators_[thread];
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	generator.setModel( *(*models_)[idx] );

	IOPar par; fillPar( par ); generator.usePar( par );
	if ( wavelet_ )
	    generator.setWavelet( wavelet_ );
	generator.setOutSampling( outputsampling_ );
	if ( !generator.doWork() )
	    mErrRet( generator.errMsg(), false );

	Threads::Locker lckr( lock_ );
	trcs_ += new SeisTrc( generator.result() );
	ReflectivityModel* sampledrefmodel = new ReflectivityModel();
	generator.getSampledRM( *sampledrefmodel );
	sampledreflmodels_->add( sampledrefmodel );
	trcidxs_ += idx;
	lckr.unlockNow();

	addToNrDone( mCast(int,generator.currentProgress()) );
    }
    return true;
}


void SynthSeis::MultiTraceGenerator::getResult( ObjectSet<SeisTrc>& trcs )
{
    TypeSet<int> sortidxs;
    for ( int idtrc=0; idtrc<trcs_.size(); idtrc++ )
	sortidxs += idtrc;
    sort_coupled( trcidxs_.arr(), sortidxs.arr(), trcidxs_.size() );
    for ( int idtrc=0; idtrc<trcs_.size(); idtrc++ )
	trcs += trcs_[ sortidxs[idtrc] ];

    trcs_.erase();
}


void SynthSeis::MultiTraceGenerator::getSampledRMs(
				RefMan<ReflectivityModelSet>& sampledrms )
{
    TypeSet<int> sortidxs;
    for ( int idtrc=0; idtrc<sampledreflmodels_->size(); idtrc++ )
	sortidxs += idtrc;
    sort_coupled( trcidxs_.arr(), sortidxs.arr(), trcidxs_.size() );
    for ( int irm=0; irm<sampledreflmodels_->size(); irm++ )
    {
	const ReflectivityModel* model = sampledreflmodels_->get(sortidxs[irm]);
	sampledrms->add( model );
    }
}
