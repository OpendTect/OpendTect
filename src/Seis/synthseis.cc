/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : Wavelet
-*/

static const char* rcsID = "$Id: synthseis.cc,v 1.24 2011-05-03 15:12:39 cvsbruno Exp $";

#include "arrayndimpl.h"
#include "fourier.h"
#include "genericnumer.h"
#include "raytrace1d.h"
#include "reflectivitymodel.h"
#include "reflectivitysampler.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "synthseis.h"
#include "survinfo.h"
#include "velocitycalc.h"
#include "wavelet.h"

namespace Seis
{

#define mErrRet(msg) { errmsg_ = msg; return false; }


SynthGenBase::SynthGenBase()
    : wavelet_(0)
    , isfourier_(true)
    , usenmotimes_(false)
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
}


bool SynthGenBase::usePar( const IOPar& par ) 
{
    return par.getYN( sKeyFourier(), isfourier_ ) && 
	par.getYN( sKeyNMO(), usenmotimes_ );
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
{}


SynthGenerator::~SynthGenerator()
{
    delete fft_;
    delete [] freqwavelet_;
    delete &outtrc_;
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
    if ( !needprepare_ )
	return true;

    if ( freqwavelet_ )
	delete [] freqwavelet_;

    freqwavelet_ = new float_complex[ fftsz_];

    for ( int idx=0; idx<fftsz_; idx++ )
	freqwavelet_[idx] = idx<wavelet_->size() ? wavelet_->samples()[idx] : 0;

    mDoFFT( freqwavelet_, true, fftsz_ )

    needprepare_ = false;
    return true;
}


bool SynthGenerator::doWork()
{
    if ( needprepare_ )
	doPrepare();

    if ( !refmodel_ || refmodel_->isEmpty() ) 
	mErrRet( "No reflectivity model found" );	

    if ( !wavelet_ ) 
	mErrRet( "No wavelet found" );	

    if ( outputsampling_.nrSteps() < 2 )
	mErrRet( "Output sampling is too small" );	

    const int wvltsz = wavelet_->size(); 
    if ( wvltsz < 2 )
	mErrRet( "Wavelet is too short" );	

    if ( wvltsz > outtrc_.size() )
	mErrRet( "Wavelet is longer than the output trace" );

    setConvolDomain( isfourier_ );

    return computeTrace( (float*)outtrc_.data().getComponent(0)->data() );
}


bool SynthGenerator::computeTrace( float* res ) 
{
    outtrc_.zero();
    cresamprefl_.erase();
    ReflectivitySampler sampler( *refmodel_, outputsampling_, 
	    			cresamprefl_, usenmotimes_ );
    sampler.setTargetDomain( (bool)fft_ );
    sampler.execute();

    return fft_ ? doFFTConvolve( res) : doTimeConvolve( res );
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

    for ( int idx=0; idx<wvltsz/2; idx++ )
	res[nrsamp-idx-1] = cres[idx].real();
    for ( int idx=0; idx<nrsamp-wvltsz/2; idx++ )
	res[idx] = cres[idx+wvltsz/2].real();

    delete [] cres;
    
    return true;
}


bool SynthGenerator::doTimeConvolve( float* res )
{
    const int ns = outtrc_.size();
    TypeSet<float> refs; getSampledReflectivities( refs );
    const int wvltcs = wavelet_->centerSample();
    const int wvltsz = wavelet_->size();
    GenericConvolve( wvltsz, -wvltcs-1, wavelet_->samples(),
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



MultiTraceSynthGenerator::~MultiTraceSynthGenerator()
{
    deepErase( synthgens_ );
}


void MultiTraceSynthGenerator::setModels( 
			const ObjectSet<const ReflectivityModel>& refmodels ) 
{
    for ( int idx=0; idx<refmodels.size(); idx++ )
    {
	SynthGenerator* synthgen = new SynthGenerator();
	synthgen->setModel( *refmodels[idx] );
	synthgens_ += synthgen;
    }
}


od_int64 MultiTraceSynthGenerator::nrIterations() const
{ return synthgens_.size(); }


bool MultiTraceSynthGenerator::doWork(od_int64 start, od_int64 stop, int thread)
{
    for ( int idx=start; idx<=stop; idx++ )
    {
	SynthGenerator& synthgen = *synthgens_[idx];
	if ( !synthgen.doPrepare() )
	    mErrRet( synthgen.errMsg() );	

	synthgen.setWavelet( wavelet_, OD::UsePtr );
	IOPar par; fillPar( par ); synthgen.usePar( par ); 
	synthgen.setOutSampling( outputsampling_ );
	if ( !synthgen.doWork() )
	    mErrRet( synthgen.errMsg() );	
    }
    return true;
}


void MultiTraceSynthGenerator::result( ObjectSet<const SeisTrc>& trcs ) const 
{
    for ( int idx=0; idx<synthgens_.size(); idx++ )
	trcs += new SeisTrc( synthgens_[idx]->result() );
}


void MultiTraceSynthGenerator::getSampledReflectivities( 
						TypeSet<float>& rfs) const
{
    synthgens_[0]->getSampledReflectivities( rfs );
}




mImplFactory( RaySynthGenerator, RaySynthGenerator::factory );


RaySynthGenerator::RaySynthGenerator()
    : outputdataismine_(true) 
{
    clean();
}



RaySynthGenerator::~RaySynthGenerator()
{
    if ( outputdataismine_ )
	deepErase( raymodels_ );
    else
	raymodels_.erase();
}


bool RaySynthGenerator::addModel( const AIModel& aim )
{
    aimodels_ += aim;
    return true;
}


bool RaySynthGenerator::setRayParams( const TypeSet<float>& offs,
				    const RayTracer1D::Setup& su, bool isnmo)
{
    raysetup_ = su;
    usenmotimes_ = isnmo;
    offsets_ = offs;

    return true;
}


void RaySynthGenerator::clean()
{
    aimodels_.erase();
    outputsampling_.set(mUdf(float),mUdf(float),mUdf(float));
    raysampling_.set(0,0,0);

    if ( outputdataismine_ )
	deepErase( raymodels_ );
    else
	raymodels_.erase();

    outputdataismine_ = true;
}


bool RaySynthGenerator::doWork()
{
    return doRayTracing() && doSynthetics();
}


bool RaySynthGenerator::doRayTracing()
{
    if ( aimodels_.isEmpty() ) 
	mErrRet( "No AIModel set" );

    if ( offsets_.isEmpty() ) 
	offsets_ += 0;

    for ( int idx=0; idx<aimodels_.size(); idx++ )
    {
	const AIModel& aim = aimodels_[idx];
	if ( aim.isEmpty() )
	    continue;

	RayTracer1D* rt1d = new RayTracer1D( raysetup_ );
	rt1d->setModel( true, aim );
	rt1d->setOffsets( offsets_ );
	if ( !rt1d->execute() )
	    mErrRet( rt1d->errMsg() )

	RayModel* rm = new RayModel( *rt1d, offsets_.size() );
	for ( int idoff=0; idoff<offsets_.size(); idoff++ )
	{
	    const TimeDepthModel& d2t = usenmotimes_ ? *rm->t2dmodels_[0]
						     : *rm->t2dmodels_[idoff];
	    raysampling_.include( d2t.getLastTime() );
	}
	delete rt1d;
	raymodels_ += rm;
    }
    return true;
}


bool RaySynthGenerator::doSynthetics()
{
    if ( !mIsUdf( outputsampling_.start ) )
	raysampling_ = outputsampling_;
    raysampling_.step = wavelet_ ? wavelet_->sampleRate() : 0;
    if ( raysampling_.nrSteps() < 1 )
	mErrRet( "no valid times generated" )

    ObjectSet<MultiTraceSynthGenerator> mtsgs;
    IOPar par; fillPar( par );
    for ( int idx=0; idx<raymodels_.size(); idx++ )
    {
	const RayModel& rm = *raymodels_[idx];
	MultiTraceSynthGenerator* mtsg = new MultiTraceSynthGenerator();
	mtsg->setModels( rm.refmodels_ );
	mtsg->setOutSampling( raysampling_ );
	mtsg->usePar( par );
	mtsg->setWavelet( wavelet_, OD::UsePtr );
	mtsgs += mtsg;
	if ( !mtsg->execute() )
	    { errmsg_ = mtsg->errMsg(); }
    }
    for ( int idx=0; idx<raymodels_.size(); idx++ )
    {
	mtsgs[idx]->result( raymodels_[idx]->outtrcs_ );
	for ( int idoff=0; idoff<offsets_.size(); idoff++ )
	    mtsgs[idx]->getSampledReflectivities(raymodels_[idx]->sampledrefs_);
	delete mtsgs[idx];
    }
    return true;
}


RaySynthGenerator::RayModel* RaySynthGenerator::result( int imdl ) 
{
    outputdataismine_ = false;
    return raymodels_[imdl];
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


}// namespace

