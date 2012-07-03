/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : SynthSeis
-*/

static const char* rcsID mUnusedVar = "$Id: synthseis.cc,v 1.56 2012-07-03 12:03:22 cvsbruno Exp $";

#include "synthseis.h"

#include "arrayndimpl.h"
#include "factory.h"
#include "fourier.h"
#include "genericnumer.h"
#include "reflectivitysampler.h"
#include "raytrace1d.h"
#include "raytracerrunner.h"
#include "reflectivitymodel.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "sorting.h"
#include "survinfo.h"
#include "velocitycalc.h"
#include "wavelet.h"

namespace Seis
{

#define mErrRet(msg) { errmsg_ = msg; return false; }

mImplFactory( SynthGenerator, SynthGenerator::factory );

SynthGenBase::SynthGenBase()
    : wavelet_(0)
    , isfourier_(true)
    , waveletismine_(false)
    , usenmotimes_(false)
    , outputsampling_(mUdf(float),mUdf(float),mUdf(float))
    , tr_(0)
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
	mErrRet( "No valid wavelet given" );	
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
    par.setYN( sKeyNMO(), usenmotimes_ );
    par.setYN( sKeyInternal(), dointernalmultiples_ );
    par.set( sKeySurfRefl(), surfreflcoeff_ );
}


bool SynthGenBase::usePar( const IOPar& par ) 
{
    return par.getYN( sKeyNMO(), usenmotimes_ )
	&& par.getYN( sKeyFourier(), isfourier_ )
	&& par.getYN( sKeyInternal(), dointernalmultiples_ )
        && par.get( sKeySurfRefl(), surfreflcoeff_ );
}


bool SynthGenBase::setOutSampling( const StepInterval<float>& si )
{
    outputsampling_ = si;
    return true;
}




SynthGenerator::SynthGenerator()
    : fft_(new Fourier::CC)
    , fftsz_(0)			   
    , freqwavelet_(0)
    , outtrc_(*new SeisTrc)
    , refmodel_(0)
    , needprepare_(true)
    , doresample_(true)
    , progress_(-1)
{}


SynthGenerator::~SynthGenerator()
{
    delete fft_;
    delete [] freqwavelet_;
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
    return true;
}


bool SynthGenerator::setWavelet( const Wavelet* wvlt, OD::PtrPolicy pol )
{
    needprepare_ = true;
    return SynthGenBase::setWavelet( wvlt, pol );
}


void SynthGenerator::setConvolDomain( bool fourier )
{
    if ( fourier == ((bool) fft_ ) )
	return;

    if ( fft_ )
    {
	delete fft_; fft_ = 0;
    }
    else
    {
	fft_ = new Fourier::CC;
    }
    isfourier_ = fourier;
}


bool SynthGenerator::setOutSampling( const StepInterval<float>& si )
{
    SynthGenBase::setOutSampling( si );
    outtrc_.reSize( si.nrSteps()+1, false );
    outtrc_.info().sampling = si;
    fftsz_ = si.nrSteps()+1 ; //fft_->getFastSize(  si.nrSteps()+1 );
    needprepare_ = true;
    return true;
}


#define mDoFFT( arr, dir, sz )\
    fft_->setInputInfo( Array1DInfoImpl(sz) );\
    fft_->setDir( dir );\
    fft_->setNormalization( !dir );\
    fft_->setInput( arr );\
    fft_->setOutput( arr );\
    fft_->run( true );

bool SynthGenerator::doPrepare()
{
    if ( !needprepare_ || !fft_ )
	return true;

    if ( freqwavelet_ )
	delete [] freqwavelet_;

    freqwavelet_ = new float_complex[ fftsz_ ];

    for ( int idx=0; idx<fftsz_; idx++ )
	freqwavelet_[idx] = wavelet_ && idx<wavelet_->size() ? 
	    			    wavelet_->samples()[idx] : 0;

    mDoFFT( freqwavelet_, true, fftsz_ )

    needprepare_ = false;
    return true;
}


bool SynthGenerator::doWork()
{
    setConvolDomain( isfourier_ );

    if ( needprepare_ )
	doPrepare();

    if ( !wavelet_ ) 
	mErrRet( "No wavelet found" );	

    if ( !refmodel_ ) 
	mErrRet( "No reflectivity model found" );	

    if ( outputsampling_.nrSteps() < 2 )
	mErrRet( "Output sampling is too small" );	

    const int wvltsz = wavelet_->size(); 
    if ( wvltsz < 2 )
	mErrRet( "Wavelet is too short" );	

    if ( wvltsz > outtrc_.size() )
	mErrRet( "Wavelet is longer than the output trace" );

    return computeTrace( (float*)outtrc_.data().getComponent(0)->data() );
}


bool SynthGenerator::computeTrace( float* res ) 
{
    outtrc_.zero();

    if ( !computeReflectivities() ) 
	return false;

    return fft_ ? doFFTConvolve( res ) : doTimeConvolve( res );
}


bool SynthGenerator::computeReflectivities()
{
    cresamprefl_.erase();

    if ( doresample_ )
    {
	ReflectivitySampler sampler( *refmodel_, outputsampling_, 
				    cresamprefl_, usenmotimes_ );
	sampler.setTargetDomain( (bool)fft_ );
	sampler.execute( true );
	progress_ = sampler.nrDone();
    }
    else
    {
	for ( int idx=0; idx<refmodel_->size(); idx++ )
	    cresamprefl_ += (*refmodel_)[idx].reflectivity_;
    }

    return true;
}


bool SynthGenerator::doFFTConvolve( float* res )
{
    if ( !fft_ ) return false; 

    const int wvltsz = wavelet_->size();
    const int nrsamp = outtrc_.size();
    float_complex* cres = new float_complex[ fftsz_ ];

    for ( int idx=0; idx<cresamprefl_.size(); idx++ )
	cres[idx] = cresamprefl_[idx] * freqwavelet_[idx]; 

    mDoFFT( cres, false, fftsz_ )

    const int midwvltsz = wvltsz%2 == 0 ? wvltsz/2-1 : wvltsz/2;

    for ( int idx=0; idx<midwvltsz; idx++ )
	res[nrsamp-idx-1] = cres[idx].real();
    for ( int idx=0; idx<nrsamp-midwvltsz; idx++ )
	res[idx] = cres[idx+midwvltsz].real();

    delete [] cres;
    
    return true;
}


bool SynthGenerator::doTimeConvolve( float* res )
{
    const int ns = outtrc_.size();
    TypeSet<float> refs; getSampledReflectivities( refs );
    const int wvltsz = wavelet_->size();
    const int wvltcs = wavelet_->centerSample();
    GenericConvolve( wvltsz, -wvltcs, wavelet_->samples(),
		     refs.size(), 0, refs.arr(),
		     ns, 0, res );
    return true;
}


void SynthGenerator::getSampledReflectivities( TypeSet<float>& refs ) const
{
    int sz = cresamprefl_.size(); 

    if ( fft_ )
    {
	float_complex* outrefs = new float_complex[sz]; 
	memcpy( outrefs, cresamprefl_.arr(), sizeof(float)*sz );

	mDoFFT( outrefs, false, sz )
	for ( int idx=0; idx<sz; idx++ )
	    refs += outrefs[idx].real();
	delete [] outrefs;
    }
    else
    {
	for ( int idx=0; idx<sz; idx++ )
	    refs += cresamprefl_[idx].real();
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
    for ( int idx=start; idx<=stop; idx++ )
    {
	synthgen.setModel( *(*models_)[idx] );

	if ( !synthgen.doPrepare() )
	    mErrRet( synthgen.errMsg() );
	synthgen.setWavelet( wavelet_, OD::UsePtr );
	IOPar par; fillPar( par ); synthgen.usePar( par ); 
	synthgen.setOutSampling( outputsampling_ );
	if ( !synthgen.doWork() )
	    mErrRet( synthgen.errMsg() );	
	
	lock_.lock();

	trcs_ += new SeisTrc( synthgen.result() );
	trcidxs_ += idx;

	lock_.unLock();

	addToNrDone( synthgen.currentProgress() );
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
	synthgens_[0]->getSampledReflectivities( rfs );
}



RaySynthGenerator::RaySynthGenerator()
    : raysampling_(0,0)
    , forcerefltimes_(false)
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
	mErrRet( "no wavelet found" )

    if ( aimodels_.isEmpty() )
	mErrRet( "No AI model found" );

    if ( offsets_.isEmpty() )
	offsets_ += 0;

    //TODO Put this in the doWork this by looking for the 0 offset longest time,
    //run the corresponding RayTracer, get raysamling and put the rest in doWork
    RayTracerRunner rtr( aimodels_, raysetup_ );
    if ( ( tr_ && !tr_->execute( rtr ) ) || !rtr.execute() ) 
	mErrRet( rtr.errMsg() )

    ObjectSet<RayTracer1D>& rt1ds = rtr.rayTracers();
    for ( int idx=rt1ds.size()-1; idx>=0; idx-- )
    {
	const RayTracer1D* rt1d = rt1ds.remove(idx);
	RayModel* rm = new RayModel( *rt1d, offsets_.size() );
	delete rt1d;

	for ( int idoff=0; idoff<offsets_.size(); idoff++ )
	{
	    const TimeDepthModel& d2t = usenmotimes_ ? *rm->t2dmodels_[0]
						     : *rm->t2dmodels_[idoff];
	    if ( !mIsUdf( d2t.getLastTime() ) )
		raysampling_.include( d2t.getLastTime() );
	}
	raymodels_.insertAt( rm, 0 );

	if ( forcerefltimes_ )
	    rm->forceReflTimes( forcedrefltimes_ );
    }
    if ( !raysampling_.width() )
	mErrRet( "no valid time generated from raytracing" );

    if ( mIsUdf( outputsampling_.start ) )
    {
	raysampling_.stop += wavelet_->sampleRate()*wavelet_->size()/2; 
	outputsampling_ = raysampling_; 
    }
    if ( mIsUdf( outputsampling_.step ) )
	outputsampling_.step = wavelet_->sampleRate();
    if ( outputsampling_.width()/(float)outputsampling_.step<wavelet_->size() )
	mErrRet( "Time range can not be smaller than wavelet" )
    if ( outputsampling_.nrSteps() < 1 )
	mErrRet( "Time interval is empty" );

    return true;
}


bool RaySynthGenerator::doWork( od_int64 start, od_int64 stop, int )
{
    IOPar par; fillPar( par );
    for ( int idx=start; idx<=stop; idx++, addToNrDone(1) )
    {
	if ( !shouldContinue() )
	    return false;

	RayModel& rm = *raymodels_[idx];
	deepErase( rm.outtrcs_ );

	rm.sampledrefs_.erase();
	MultiTraceSynthGenerator multitracegen;
	multitracegen.setTaskRunner( tr_ );
	multitracegen.setModels( rm.refmodels_ );
	multitracegen.setWavelet( wavelet_, OD::UsePtr );
	multitracegen.setOutSampling( outputsampling_ );
	multitracegen.usePar( par );

	if ( (tr_ && !tr_->execute(multitracegen)) || !multitracegen.execute() )
	    mErrRet( multitracegen.errMsg() )

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
	stckr.stack( *outtrcs_[idx], false, idx );

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

}// namespace

