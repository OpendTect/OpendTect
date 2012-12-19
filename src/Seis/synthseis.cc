/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : SynthSeis
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "synthseis.h"

#include "arrayndimpl.h"
#include "factory.h"
#include "fourier.h"
#include "genericnumer.h"
#include "muter.h"
#include "reflectivitysampler.h"
#include "raytrace1d.h"
#include "raytracerrunner.h"
#include "reflectivitymodel.h"
#include "samplfunc.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "sorting.h"
#include "survinfo.h"
#include "velocitycalc.h"
#include "wavelet.h"


using namespace Seis;


#define mErrRet(msg, retval) { errmsg_ = msg; return retval; }

mImplFactory( SynthGenerator, SynthGenerator::factory );

SynthGenBase::SynthGenBase()
    : wavelet_(0)
    , isfourier_(true)
    , stretchlimit_( cStdStretchLimit() ) //20%
    , mutelength_( cStdMuteLength() )  //20ms
    , waveletismine_(false)
    , applynmo_(false)
    , outputsampling_(mUdf(float),mUdf(float),mUdf(float))
    , dointernalmultiples_(false)
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
	mErrRet( "No valid wavelet given", false );
    if ( pol != OD::CopyPtr )
	wavelet_ = wvlt;
    else 
	wavelet_ = new Wavelet( *wvlt );

    waveletismine_ = pol != OD::UsePtr;
    return true;
}


void SynthGenBase::fillPar( IOPar& par ) const
{
    par.setYN( sKeyFourier(), isfourier_ );
    par.setYN( sKeyNMO(), applynmo_ );
    par.setYN( sKeyInternal(), dointernalmultiples_ );
    par.set( sKeySurfRefl(), surfreflcoeff_ );
    par.set( sKeyMuteLength(), mutelength_ );
    par.set( sKeyStretchLimit(), stretchlimit_ );
}


bool SynthGenBase::usePar( const IOPar& par ) 
{
    return par.getYN( sKeyNMO(), applynmo_ )
	&& par.getYN( sKeyFourier(), isfourier_ )
	&& par.getYN( sKeyInternal(), dointernalmultiples_ )
        && par.get( sKeySurfRefl(), surfreflcoeff_ )
    	&& par.get( sKeyStretchLimit(), stretchlimit_)
        && par.get( sKeyMuteLength(), mutelength_ );
}


bool SynthGenBase::setOutSampling( const StepInterval<float>& si )
{
    outputsampling_ = si;
    return true;
}



SynthGenerator::SynthGenerator()
    : outtrc_(*new SeisTrc)
    , refmodel_(0)
    , doresample_(true)
    , progress_(-1)
    , convolvesize_( 0 )
{}


SynthGenerator::~SynthGenerator()
{
    freqwavelet_.erase();
    creflectivities_.erase();
    freqreflectivities_.erase();
    
    delete &outtrc_;
}


SynthGenerator* SynthGenerator::create( bool advanced )
{
    SynthGenerator* sg = 0;
    const BufferStringSet& fnms = SynthGenerator::factory().getNames(false);

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
    reflectivities_.erase();
    creflectivities_.erase();
    
    convolvesize_ = 0;
    
    return true;
}


bool SynthGenerator::setWavelet( const Wavelet* wvlt, OD::PtrPolicy pol )
{
    freqwavelet_.erase();
    convolvesize_ = 0;
        
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
ft->setDir( dir );\
ft->setNormalization( !dir );\
ft->setInput( arr );\
ft->setOutput( arr );\

#define mDoFFT( ft, arr, dir, sz ) \
mPrepFFT( ft, arr, dir, sz ) \
ft->run( true );


int SynthGenerator::nextStep()
{
    if ( !wavelet_ )
	mErrRet( "No wavelet found", SequentialTask::ErrorOccurred() );
    
    if ( !refmodel_ )
	mErrRet( "No reflectivity model found",
		SequentialTask::ErrorOccurred() );
    
    if ( outputsampling_.nrSteps() < 2 )
	mErrRet( "Output sampling is too small",
		SequentialTask::ErrorOccurred() );
    
    const int wvltsz = wavelet_->size();
    if ( wvltsz < 2 )
	mErrRet( "Wavelet is too short",
		SequentialTask::ErrorOccurred() );
    
    if ( wvltsz > outtrc_.size() )
	mErrRet( "Wavelet is longer than the output trace",
		SequentialTask::ErrorOccurred() );
    
    if ( convolvesize_==0 )
    {
	if ( applynmo_ )
	{
	    float maxtime = 0;
	    for ( int idx=0; idx<refmodel_->size(); idx++)
	    {
		float time = (*refmodel_)[idx].time_;
		maxtime = mMAX( time, maxtime );
	    }
	    
	    maxtime += wavelet_->samplePositions().stop;
	    
	    const StepInterval<float> convrg( outputsampling_.start,
					maxtime, outputsampling_.step );
	    convolvesize_ = convrg.nrSteps()+1;
	    if ( convolvesize_<1 )
	    {
		mErrRet("Not sound", SequentialTask::ErrorOccurred() );
	    }
	}
	else
	{
	    convolvesize_ = outputsampling_.nrSteps()+1;
	}
	
	if ( convolvesize_<1 )
	    mErrRet( "Cannot determine convolution size.",
		      SequentialTask::ErrorOccurred());
    }
    
    if ( isfourier_ && !freqwavelet_.size() )
    {
	PtrMan<Fourier::CC> fft = Fourier::CC::createDefault();

	const int oldconvsize = convolvesize_;
	convolvesize_ = fft->getFastSize( oldconvsize );
	if ( convolvesize_<oldconvsize || convolvesize_<wavelet_->size() )
	    return SequentialTask::ErrorOccurred();
	
	freqwavelet_.setSize( convolvesize_, float_complex(0,0) );
	for ( int idx=0; idx<wavelet_->size(); idx++ )
	{
	    int arrpos = idx-wavelet_->centerSample();
	    if ( arrpos<0 )
		arrpos += convolvesize_;
	    
	    freqwavelet_[arrpos] = wavelet_->samples()[idx];
	}
	
	mPrepFFT( fft, freqwavelet_.arr(), true, convolvesize_ );
	if ( !fft->run( true ) )
	{
	    freqwavelet_.erase();
	    	    
	    mErrRet( "Error running FFT", SequentialTask::ErrorOccurred());
	}
	
	return SequentialTask::MoreToDo();
    }
    
    if ( reflectivities_.isEmpty() && freqreflectivities_.isEmpty() )
    {
	return computeReflectivities()
	    ? SequentialTask::MoreToDo()
	    : SequentialTask::ErrorOccurred();
    }
    
    return computeTrace( outtrc_ )
	? SequentialTask::Finished()
	: SequentialTask::ErrorOccurred();
}


bool SynthGenerator::doWork()
{
    int res = SequentialTask::MoreToDo();
    while ( res==SequentialTask::MoreToDo() )
    {
    	res = nextStep();
    }
    
    return res==SequentialTask::Finished();
}


bool SynthGenerator::computeTrace( SeisTrc& res ) const
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
    float mutelevel = outputsampling_.start;
    
    PointBasedMathFunction stretchfunc( PointBasedMathFunction::Linear,
				       PointBasedMathFunction::ExtraPolGradient);
    
    for ( int idx=1; idx<refmodel_->size(); idx++ )
    {
	const ReflectivitySpike& spike = (*refmodel_)[idx];
	
	//check for crossing events
	const ReflectivitySpike& spikeabove = (*refmodel_)[idx-1];
	if ( spike.time_<spikeabove.time_ )
	{
	    mutelevel = mMAX(spike.correctedtime_, mutelevel );
	}

	stretchfunc.add( spike.correctedtime_, spike.time_ );
    }
    
    out.setAll( 0 );
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

bool SynthGenerator::computeReflectivities()
{
    reflectivities_.erase();
    freqreflectivities_.erase();
    creflectivities_.erase();
    
    const StepInterval<float> sampling( outputsampling_.start,
	      outputsampling_.atIndex( convolvesize_ ), outputsampling_.step );

    if ( isfourier_ )
    {
	ReflectivitySampler sampler( *refmodel_, sampling,
				     freqreflectivities_, false );
	sampler.setTargetDomain( true );
	return sampler.execute( true );
    }
        
    computeSampledReflectivities( reflectivities_, &creflectivities_ );
    return true;
}


bool SynthGenerator::doFFTConvolve( ValueSeries<float>& res, int outsz ) const
{
    PtrMan<Fourier::CC> fft = Fourier::CC::createDefault();
    if ( !fft )
	return false;
    
    mDeclareAndTryAlloc( ArrPtrMan<float_complex>, cres,
			 float_complex[convolvesize_]);
    if ( !cres )
	return false;
    
    for ( int idx=0; idx<convolvesize_; idx++ )
	cres[idx] = freqreflectivities_[idx] * freqwavelet_[idx];
    
    mPrepFFT( fft, cres, false, convolvesize_ );
    if ( !fft->run( true ) )
    {
	return false;
    }

    for ( int idx=0; idx<outsz; idx++ )
	res.setValue( idx, cres[idx].real() );
    
    return true;
}


bool SynthGenerator::doTimeConvolve( ValueSeries<float>& res, int outsz ) const
{
    const int wvltsz = wavelet_->size();
    const int wvltcs = wavelet_->centerSample();
    
    float* resarr = outsz==convolvesize_ ? res.arr() : 0;
    ArrPtrMan<float> cres = resarr ? 0 : new float[convolvesize_];
    float* usedarr = resarr ? resarr : cres.ptr();

    GenericConvolve( wvltsz, -wvltcs, wavelet_->samples(),
		     convolvesize_, 0, reflectivities_.arr(),
		     convolvesize_, 0, usedarr );
    
    if ( !resarr )
    {
	for ( int idx=0; idx<outsz; idx++ )
	    res.setValue( idx, cres[idx] );
    }
	
    return true;
}


void SynthGenerator::getSampledReflectivities( TypeSet<float>& refs ) const
{
    computeSampledReflectivities( refs );
}


void SynthGenerator::computeSampledReflectivities(TypeSet<float>& realres,
				      TypeSet<float_complex>* cres ) const
{
    const StepInterval<float> sampling( outputsampling_.start,
	outputsampling_.atIndex( convolvesize_ ), outputsampling_.step );
    
    realres.erase();
    realres.setSize( convolvesize_, 0 );
    
    if ( cres )
    {
	cres->erase();
	cres->setSize( convolvesize_, float_complex(0,0) );
    }
    
    for ( int idx=0; idx<refmodel_->size(); idx++ )
    {
	const ReflectivitySpike& spike = (*refmodel_)[idx];
	const int sample = sampling.nearestIndex( spike.time_ );
	realres[sample] += spike.reflectivity_.real();
	if ( cres) (*cres)[sample] += spike.reflectivity_;
    }
}



MultiTraceSynthGenerator::MultiTraceSynthGenerator()
    : totalnr_( -1 )
{}


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

	synthgen.setWavelet( wavelet_, OD::UsePtr );
	IOPar par; fillPar( par ); synthgen.usePar( par ); 
	synthgen.setOutSampling( outputsampling_ );
	if ( !synthgen.doWork() )
	    mErrRet( synthgen.errMsg(), false );
	
	lock_.lock();

	trcs_ += new SeisTrc( synthgen.result() );
	trcidxs_ += idx;

	lock_.unLock();

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


void MultiTraceSynthGenerator::getSampledReflectivities( 
						TypeSet<float>& rfs) const
{
    if ( !synthgens_.isEmpty() )
    {
	synthgens_[0]->getSampledReflectivities( rfs );
    }
}



RaySynthGenerator::RaySynthGenerator()
    : raysampling_(0,0)
    , forcerefltimes_(false)
    , rtr_( 0 )
{}


RaySynthGenerator::~RaySynthGenerator()
{
    deepErase( raymodels_ );
}


void RaySynthGenerator::addModel( const ElasticModel& aim )
{
    aimodels_ += aim;
}


od_int64 RaySynthGenerator::nrIterations() const
{
    return aimodels_.size();
}


bool RaySynthGenerator::doPrepare( int )
{
    deepErase( raymodels_ );

    if ( !wavelet_ )
	mErrRet( "no wavelet found", false )

    if ( aimodels_.isEmpty() )
	mErrRet( "No AI model found", false );

    if ( offsets_.isEmpty() )
	offsets_ += 0;

    //TODO Put this in the doWork this by looking for the 0 offset longest time,
    //run the corresponding RayTracer, get raysamling and put the rest in doWork
    RayTracerRunner rtr( aimodels_, raysetup_ );
    rtr_ = &rtr;
    message_ = "Raytracing";
    if ( !rtr.execute() ) 
	mErrRet( rtr.errMsg(), false );
    rtr_ = 0;

    ObjectSet<RayTracer1D>& rt1ds = rtr.rayTracers();
    for ( int idx=rt1ds.size()-1; idx>=0; idx-- )
    {
	const RayTracer1D* rt1d = rt1ds.removeSingle(idx);
	RayModel* rm = new RayModel( *rt1d, offsets_.size() );
	delete rt1d;

	for ( int idoff=0; idoff<offsets_.size(); idoff++ )
	{
	    const TimeDepthModel& d2t = applynmo_ ? *rm->t2dmodels_[0]
						  : *rm->t2dmodels_[idoff];
	    if ( !mIsUdf( d2t.getLastTime() ) )
		raysampling_.include( d2t.getLastTime() );
	}
	raymodels_.insertAt( rm, 0 );

	if ( forcerefltimes_ )
	    rm->forceReflTimes( forcedrefltimes_ );
    }
    
    if ( !raysampling_.width() )
	mErrRet( "no valid time generated from raytracing", false );

    if ( mIsUdf( outputsampling_.start ) )
    {
	raysampling_.stop += wavelet_->sampleRate()*wavelet_->size()/2; 
	outputsampling_ = raysampling_; 
    }
    if ( mIsUdf( outputsampling_.step ) )
	outputsampling_.step = wavelet_->sampleRate();
    if ( outputsampling_.width()/(float)outputsampling_.step<wavelet_->size() )
	mErrRet( "Time range can not be smaller than wavelet", false )
    if ( outputsampling_.nrSteps() < 1 )
	mErrRet( "Time interval is empty", false );
    
    message_ = "Generating synthethics";

    return true;
}


bool RaySynthGenerator::doWork( od_int64 start, od_int64 stop, int )
{
    IOPar par; fillPar( par );
    for ( int idx=mCast(int,start); idx<=stop; idx++, addToNrDone(1) )
    {
	if ( !shouldContinue() )
	    return false;

	RayModel& rm = *raymodels_[idx];
	deepErase( rm.outtrcs_ );

	rm.sampledrefs_.erase();
	MultiTraceSynthGenerator multitracegen;
	multitracegen.setModels( rm.refmodels_ );
	multitracegen.setWavelet( wavelet_, OD::UsePtr );
	multitracegen.setOutSampling( outputsampling_ );
	multitracegen.usePar( par );

	if ( !multitracegen.execute() )
	    mErrRet( multitracegen.errMsg(), false )

	multitracegen.getResult( rm.outtrcs_ );
	for ( int idoff=0; idoff<offsets_.size(); idoff++ )
	{
	    if ( !rm.outtrcs_.validIdx( idoff ) )
	    {
		rm.outtrcs_ += new SeisTrc( outputsampling_.nrSteps() );
		rm.outtrcs_[idoff]->info().sampling = outputsampling_;
		for ( int idz=0; idz<rm.outtrcs_[idoff]->size(); idz++ )
		    rm.outtrcs_[idoff]->set( idz, mUdf(float), 0 );
	    }
	    rm.outtrcs_[idoff]->info().offset = offsets_[idoff];
	    rm.outtrcs_[idoff]->info().nr = idx+1;
	    multitracegen.getSampledReflectivities( rm.sampledrefs_ );
	}
    }
    
    return true;
}


od_int64 RaySynthGenerator::totalNr() const
{
    return rtr_ ? rtr_->totalNr() : nrIterations();
}


od_int64 RaySynthGenerator::nrDone() const
{
    return rtr_ ? rtr_->nrDone() : ParallelTask::nrDone();
}



RaySynthGenerator::RayModel::RayModel( const RayTracer1D& rt1d, int nroffsets )
{
    for ( int idx=0; idx<nroffsets; idx++ )
    {
	ReflectivityModel* refmodel = new ReflectivityModel(); 
	refmodels_ += refmodel;
	rt1d.getReflectivity( idx, *refmodel );
	TimeDepthModel* t2dm = new TimeDepthModel();
	rt1d.getTWT( idx, *t2dm );
	t2dmodels_ += t2dm;
    }
}


RaySynthGenerator::RayModel::~RayModel()
{
    deepErase( outtrcs_ );
    deepErase( t2dmodels_ );
    deepErase( refmodels_ );
}


const SeisTrc* RaySynthGenerator::RayModel::stackedTrc() const
{
    if ( outtrcs_.isEmpty() )
	return 0;

    SeisTrc* trc = new SeisTrc( *outtrcs_[0] );
    SeisTrcPropChg stckr( *trc );
    for ( int idx=1; idx<outtrcs_.size(); idx++ )
	stckr.stack( *outtrcs_[idx], false, mCast(float,idx) );

    return trc;
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
			ObjectSet<const ReflectivityModel>& trcs, bool steal )
{
    mGet( refmodels_, trcs, steal );
}


void RaySynthGenerator::RayModel::getD2T( 
			ObjectSet<TimeDepthModel>& trcs, bool steal )
{
    mGet( t2dmodels_, trcs, steal );
}


void RaySynthGenerator::RayModel::getSampledRefs( TypeSet<float>& refs ) const
{
    refs.erase(); refs.append( sampledrefs_ );
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

