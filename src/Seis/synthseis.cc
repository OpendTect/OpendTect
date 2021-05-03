/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : SynthSeis
-*/


#include "synthseis.h"

#include "arraynd.h"
#include "arrayndimpl.h"
#include "arrayndstacker.h"
#include "factory.h"
#include "fourier.h"
#include "genericnumer.h"
#include "ioman.h"
#include "muter.h"
#include "reflectivitysampler.h"
#include "raytrace1d.h"
#include "raytracerrunner.h"
#include "reflectivitymodel.h"
#include "samplfunc.h"
#include "seistrc.h"
#include "seisbuf.h"
#include "seistrcprop.h"
#include "sorting.h"
#include "survinfo.h"
#include "velocitycalc.h"
#include "wavelet.h"
#include "ioobj.h"


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
    , waveletismine_(false)
    , applynmo_(false)
    , outputsampling_(mUdf(float),-mUdf(float),mUdf(float))
    , dointernalmultiples_(false)
    , dosampledreflectivities_(false)
    , surfreflcoeff_(1)
{}


SynthGenBase::~SynthGenBase()
{
    if ( waveletismine_ )
	delete wavelet_;
}


bool SynthGenBase::setWavelet( const Wavelet* wvlt, OD::PtrPolicy pol )
{
    if ( waveletismine_ )
	{ delete wavelet_; wavelet_ = 0; }
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
    par.setYN( sKeyNMO(), applynmo_ );
    par.setYN( sKeyInternal(), dointernalmultiples_ );
    par.set( sKeySurfRefl(), surfreflcoeff_ );
    par.set( sKeyMuteLength(), mutelength_ );
    par.set( sKeyStretchLimit(), stretchlimit_ );
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


bool SynthGenBase::getOutSamplingFromModel(
			     const ObjectSet<const ReflectivityModel>& models,
			     StepInterval<float>& sampling,
			     bool usenmo )
{
    const float outputsr = mIsUdf(outputsampling_.step) ? wavelet_->sampleRate()
							: outputsampling_.step;
    sampling.set( mUdf(float), -mUdf(float), outputsr );
    for ( int imod=0; imod<models.size(); imod++ )
    {
	const ReflectivityModel& model = *models[imod];
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
	mErrRet(tr("Cannot determine trace size from model(s)"), false)

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


bool SynthGenerator::setWavelet( const Wavelet* wvlt, OD::PtrPolicy pol )
{
    freqwavelet_.erase();

    return SynthGenBase::setWavelet( wvlt, pol );
}


bool SynthGenerator::setOutSampling( const StepInterval<float>& si )
{
    SynthGenBase::setOutSampling( si );
    outtrc_.reSize( si.nrSteps()+1, false );
    outtrc_.info().sampling = si;

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
	mErrRet(tr("Cannot make synthetics without wavelet"), mErrOccRet);
    const int wvltsz = wavelet_->size();
    if (wvltsz < 2)
	mErrRet(tr("Wavelet is too short - at minimum 3 samples are required"),
	mErrOccRet);

    if (!refmodel_)
	mErrRet(tr("Cannot make synthetics without reflectivity model"),
	mErrOccRet);

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
	ObjectSet<const ReflectivityModel> mod;
	mod += refmodel_;
	if ( mod.isEmpty() )
	    mpErrRet( "No models given to make synthetics", false );

	if ( !SynthGenBase::isInputOK() )
	    return mErrOccRet;

	if ( !SynthGenBase::getOutSamplingFromModel(mod,cursampling) )
	    mErrRet(tr("Cannot determine trace size from model"), mErrOccRet)

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
    for ( int idx=0; idx<wavelet_->size(); idx++ )
    {
	int arrpos = idx - wavelet_->centerSample();
	if ( arrpos < 0 )
	    arrpos += convolvesize_;

	freqwavelet_[arrpos] = wavelet_->samples()[idx];
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

    outtrc_.info().sampling.indexOnOrAfter( mutelevel );

    SampledFunctionImpl<float,ValueSeries<float> > samplfunc( input,
	    insz, outputsampling_.start,
	    outputsampling_.step );

    for ( int idx=0; idx<outsz; idx++ )
    {
	const float corrtime = outtrc_.info().sampling.atIndex( idx );
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
			 spike.reflectivity_.real(), outtrc_.info().sampling );
	nrspikes++;
    }

    Array1DImpl<float> output( outsz );
    Array1DStacker<float, Array1D<float> > stktrcs( wavelettrcs, output );
    if ( !stktrcs.execute() )
	mErrRet( mToUiStringTodo(stktrcs.errMsg()), false )

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
    const float step = outtrc_.info().sampling.step;
    float start = mCast( float, mCast( int, outtrc_.startPos()/step ) ) * step;
    if ( start < outtrc_.startPos() - 1e-4f )
	start += step;

    const float width = step * convolvesize_;
    const int nperiods = mCast( int, Math::Floor( start/width ) ) + 1;
    const SamplingData<float> fftsampling( start, step );
    SeisTrc fftout( convolvesize_ );
    fftout.info().sampling = fftsampling;
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




MultiTraceSynthGenerator::MultiTraceSynthGenerator()
    : totalnr_( -1 )
{
}


MultiTraceSynthGenerator::~MultiTraceSynthGenerator()
{
    deepErase( synthgens_ );
    deepErase( trcs_ );
    deepErase( sampledrefmodels_ );
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
			const ObjectSet<const ReflectivityModel>& refmodels )
{
    totalnr_ = 0;
    models_ = &refmodels;
    for ( int idx=0; idx<refmodels.size(); idx++ )
	totalnr_ += refmodels[idx]->size();
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
	    synthgen.setWavelet( wavelet_, OD::UsePtr );
	synthgen.setOutSampling( outputsampling_ );
	if ( !synthgen.doWork() )
	    mErrRet( synthgen.errMsg(), false );

	Threads::Locker lckr( lock_ );
	trcs_ += new SeisTrc( synthgen.result() );
	ReflectivityModel* sampledrefmodel = new ReflectivityModel();
	synthgen.getSampledRM( *sampledrefmodel );
	sampledrefmodels_ += sampledrefmodel;
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
	ObjectSet<const ReflectivityModel>& sampledrms )
{
    TypeSet<int> sortidxs;
    for ( int idtrc=0; idtrc<sampledrefmodels_.size(); idtrc++ )
	sortidxs += idtrc;
    sort_coupled( trcidxs_.arr(), sortidxs.arr(), trcidxs_.size() );
    for ( int irm=0; irm<sampledrefmodels_.size(); irm++ )
	sampledrms += sampledrefmodels_[ sortidxs[irm] ];
    sampledrefmodels_.erase();
}



RaySynthGenerator::RaySynthGenerator( const TypeSet<ElasticModel>* ems,
				      bool ownrms )
    : raytracingdone_( false )
    , ownraymodels_( ownrms )
    , aimodels_( ems )
    , raymodels_( nullptr )
{}


RaySynthGenerator::RaySynthGenerator( ObjectSet<RayModel>* rms )
    : raytracingdone_( true )
    , ownraymodels_( false )
    , aimodels_( nullptr )
    , raymodels_( rms )
{}


void RaySynthGenerator::reset()
{
    resetNrDone();
    message_ = uiString::emptyString();
}


RaySynthGenerator::~RaySynthGenerator()
{
    delete rtr_;
    if ( ownraymodels_ && raymodels_ )
	deepErase( *raymodels_ );
}


od_int64 RaySynthGenerator::nrIterations() const
{
    return aimodels_ ? aimodels_->size() : raymodels_->size();
}


const ObjectSet<RayTracer1D>& RaySynthGenerator::rayTracers() const
{ return  rtr_->rayTracers(); }


bool RaySynthGenerator::doPrepare( int )
{
    if ( ownraymodels_ && raymodels_ )
	deepErase( *raymodels_ );

    if ( aimodels_ && aimodels_->isEmpty() )
	mErrRet(tr("No AI model found"), false);

    if ( offsets_.isEmpty() )
	offsets_ += 0;

    if ( !raymodels_ )
    {
	rtr_ = new RayTracerRunner( *aimodels_, raysetup_ );
	message_ = tr("Raytracing");
	if ( !rtr_->execute() )
	    mErrRet( rtr_->errMsg(), false );

	raytracingdone_ = true;

	message_ = tr("Preparing Reflectivity Model");
	raymodels_ = new ObjectSet<RayModel>();
	const ObjectSet<RayTracer1D>& rt1ds = rtr_->rayTracers();
	resetNrDone();
	for ( int idx=rt1ds.size()-1; idx>=0; idx-- )
	{
	    const RayTracer1D* rt1d = rt1ds[idx];
	    RayModel* rm = new RayModel( *rt1d, offsets_.size() );
	    raymodels_->insertAt( rm, 0 );

	    if ( forcerefltimes_ )
		rm->forceReflTimes( forcedrefltimes_ );
	    addToNrDone( 1 );
	}
    }

    resetNrDone();
    ObjectSet<const ReflectivityModel> models;
    getAllRefls( models );
    const bool zerooffset = offsets_.size() == 1 && mIsZero(offsets_[0],1e-1f);
    StepInterval<float> cursampling( outputsampling_ );
    if ( models.isEmpty() )
	mErrRet( tr("No models given to make synthetics"), false );

    if ( !SynthGenBase::isInputOK() )
	return mErrOccRet;

    if ( !SynthGenBase::getOutSamplingFromModel(models,cursampling,
						applynmo_ || zerooffset) &&
	  aimodels_ )
    {
	Interval<float> modelsampling;
	ElasticModel::getTimeSampling( *aimodels_, modelsampling );
	cursampling.include( modelsampling, false );
    }

    outputsampling_.include( cursampling, false );
    outputsampling_.step = cursampling.step;

    message_ = tr("Generating synthetics");

    return true;
}


bool RaySynthGenerator::doWork( od_int64 start, od_int64 stop, int )
{
    if ( !raymodels_ ) return false;
    IOPar par; fillPar( par );
    for ( int idx=mCast(int,start); idx<=stop; idx++, addToNrDone(1) )
    {
	if ( !shouldContinue() )
	    return false;

	RayModel& rm = *(*raymodels_)[idx];
	deepErase( rm.outtrcs_ );
	deepErase( rm.sampledrefmodels_ );

	MultiTraceSynthGenerator multitracegen;
	multitracegen.setModels( rm.refmodels_ );
	multitracegen.setOutSampling( outputsampling_ );
	multitracegen.usePar( par );
	if ( wavelet_ )
	    multitracegen.setWavelet( wavelet_, OD::UsePtr );

	if ( !multitracegen.execute() )
	    mErrRet( multitracegen.errMsg(), false )

	multitracegen.getResult( rm.outtrcs_ );
	multitracegen.getSampledRMs( rm.sampledrefmodels_ );
	for ( int idoff=0; idoff<offsets_.size(); idoff++ )
	{
	    if ( !rm.outtrcs_.validIdx( idoff ) )
	    {
		rm.outtrcs_ += new SeisTrc( outputsampling_.nrSteps() + 1 );
		rm.outtrcs_[idoff]->info().sampling = outputsampling_;
		for ( int idz=0; idz<rm.outtrcs_[idoff]->size(); idz++ )
		    rm.outtrcs_[idoff]->set( idz, mUdf(float), 0 );
	    }
	    rm.outtrcs_[idoff]->info().offset = offsets_[idoff];
	    rm.outtrcs_[idoff]->info().nr = idx+1;
	}
    }

    return true;
}


od_int64 RaySynthGenerator::totalNr() const
{
    return !raytracingdone_ && rtr_ ? rtr_->totalNr() : nrIterations();
}


od_int64 RaySynthGenerator::nrDone() const
{
    return !raytracingdone_ && rtr_ ? rtr_->nrDone() : ParallelTask::nrDone();
}



uiString RaySynthGenerator::uiNrDoneText() const
{
    return !raytracingdone_ && rtr_ ? tr("Layers done") : tr("Models done");
}


RaySynthGenerator::RayModel::RayModel( const RayTracer1D& rt1d, int nroffsets )
    : zerooffset2dmodel_(0)
{
    for ( int idx=0; idx<nroffsets; idx++ )
    {
	ReflectivityModel* refmodel = new ReflectivityModel();
	rt1d.getReflectivity( idx, *refmodel );

	TimeDepthModel* t2dm = new TimeDepthModel();
	rt1d.getTDModel( idx, *t2dm );

	refmodels_ += refmodel;
	t2dmodels_ += t2dm;
	if ( !idx )
	{
	    zerooffset2dmodel_ = new TimeDepthModel();
	    rt1d.getZeroOffsTDModel( *zerooffset2dmodel_ );
	}
    }
}


RaySynthGenerator::RayModel::~RayModel()
{
    deepErase( outtrcs_ );
    deepErase( t2dmodels_ );
    deepErase( refmodels_ );
    deepErase( sampledrefmodels_ );
    delete zerooffset2dmodel_;
}


bool RaySynthGenerator::usePar( const IOPar& par )
{
    SynthGenBase::usePar( par );
    raysetup_.merge( par );
    raysetup_.get( RayTracer1D::sKeyOffset(), offsets_ );
    if ( offsets_.isEmpty() )
	offsets_ += 0;

    return true;
}


void RaySynthGenerator::fillPar( IOPar& par ) const
{
    SynthGenBase::fillPar( par );
    par.merge( raysetup_ );
}


void RaySynthGenerator::forceReflTimes( const StepInterval<float>& si)
{
    forcedrefltimes_ = si; forcerefltimes_ = true;
}


void RaySynthGenerator::getAllRefls( ObjectSet<const ReflectivityModel>& refs )
{
    if ( !raymodels_ || raymodels_->isEmpty() ) return;

    refs.setEmpty();
    for ( int imod=0; imod<raymodels_->size(); imod++ )
    {
	if ( !(*raymodels_)[imod] )
	    continue;

	ObjectSet<const ReflectivityModel> curraymodel;
	(*raymodels_)[imod]->getRefs( curraymodel, false );
	refs.append( curraymodel  );
    }
}

#define mGet( inpset, outpset, steal )\
{\
    outpset.copy( inpset );\
    if ( steal )\
	inpset.erase();\
}

void RaySynthGenerator::RayModel::getTraces(
		    ObjectSet<SeisTrc>& trcs, bool steal )
{
    mGet( outtrcs_, trcs, steal );
}


void RaySynthGenerator::RayModel::getRefs(
			ObjectSet<const ReflectivityModel>& refmodels,
			bool steal, bool sampled )
{
    ObjectSet<const ReflectivityModel>& rms =
	sampled ? sampledrefmodels_ : refmodels_;
    mGet( rms, refmodels, steal );
}


void RaySynthGenerator::RayModel::getZeroOffsetD2T( TimeDepthModel& tdms )
{
    tdms = *zerooffset2dmodel_;
}


void RaySynthGenerator::RayModel::getD2T(
			ObjectSet<TimeDepthModel>& tdmodels, bool steal )
{
    mGet( t2dmodels_, tdmodels, steal );
}


void RaySynthGenerator::RayModel::forceReflTimes( const StepInterval<float>& si)
{
    for ( int idx=0; idx<refmodels_.size(); idx++ )
    {
	ReflectivityModel& refmodel
			= const_cast<ReflectivityModel&>(*refmodels_[idx]);
	for ( int iref=0; iref<refmodel.size(); iref++ )
	{
	    refmodel[iref].time_ = si.atIndex(iref);
	    refmodel[iref].correctedtime_ = si.atIndex(iref);
	}
    }
}


const SeisTrc* RaySynthGenerator::RayModel::stackedTrc() const
{
    if ( outtrcs_.isEmpty() )
	return 0;

    SeisTrc* trc = new SeisTrc( *outtrcs_[0] );
    SeisTrcPropChg stckr( *trc );
    for ( int idx=1; idx<outtrcs_.size(); idx++ )
    {
	const SeisTrc* outtrc = outtrcs_[idx];
	if ( !outtrc || outtrc->isNull() )
	    continue;

	stckr.stack( *outtrcs_[idx], false, mCast(float,idx) );
    }

    return trc;
}


void RaySynthGenerator::getTraces( ObjectSet<SeisTrcBuf>& seisbufs )
{
    if ( !raymodels_ || raymodels_->isEmpty() ) return;

    for ( int imdl=0; imdl<raymodels_->size(); imdl++ )
    {
	const int crlstep = SI().crlStep();
	const BinID bid0( SI().inlRange(false).stop + SI().inlStep(),
			  SI().crlRange(false).stop + crlstep );
	SeisTrcBuf* tbuf = new SeisTrcBuf( true );
	ObjectSet<SeisTrc> trcs; (*raymodels_)[imdl]->getTraces( trcs, true );
	for ( int idx=0; idx<trcs.size(); idx++ )
	{
	    SeisTrc* trc = trcs[idx];
	    trc->info().binid = BinID( bid0.inl(), bid0.crl() + imdl*crlstep );
	    trc->info().nr = imdl+1;
	    trc->info().coord = SI().transform( trc->info().binid );
	    tbuf->add( trc );
	}
	seisbufs += tbuf;
    }
}


void RaySynthGenerator::getStackedTraces( SeisTrcBuf& seisbuf )
{
    if ( !raymodels_ || raymodels_->isEmpty() ) return;

    seisbuf.erase();
    const int crlstep = SI().crlStep();
    const BinID bid0( SI().inlRange(false).stop + SI().inlStep(),
		      SI().crlRange(false).stop + crlstep );
    for ( int imdl=0; imdl<raymodels_->size(); imdl++ )
    {
	SeisTrc* trc = const_cast<SeisTrc*> ((*raymodels_)[imdl]->stackedTrc());
	trc->info().binid = BinID( bid0.inl(), bid0.crl() + imdl*crlstep );
	trc->info().nr = imdl+1;
	trc->info().coord = SI().transform( trc->info().binid );
	seisbuf.add( trc );
    }
}
} // namespace Seis
