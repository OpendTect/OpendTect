/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : Wavelet
-*/

static const char* rcsID = "$Id: synthseis.cc,v 1.33 2011-08-10 15:03:51 cvsbruno Exp $";

#include "arrayndimpl.h"
#include "fourier.h"
#include "genericnumer.h"
#include "raytrace1d.h"
#include "raytracerrunner.h"
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
    , outputsampling_(mUdf(float),mUdf(float),mUdf(float))
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
    , doresample_(true)			
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


void SynthGenerator::setConvDomain( bool fourier )
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
    if ( !needprepare_ || !fft_ )
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
    setConvDomain( isfourier_ );

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

    return computeTrace( (float*)outtrc_.data().getComponent(0)->data() );
}


bool SynthGenerator::computeTrace( float* res ) 
{
    outtrc_.zero();
    cresamprefl_.erase();

    if ( doresample_ )
    {
	ReflectivitySampler sampler( *refmodel_, outputsampling_, 
				    cresamprefl_, usenmotimes_ );
	sampler.setTargetDomain( (bool)fft_ );
	sampler.execute();
    }
    else
    {
	for ( int idx=0; idx<refmodel_->size(); idx++ )
	    cresamprefl_ += (*refmodel_)[idx].reflectivity_;
    }

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
    const int wvltcs = wvltsz%2 == 0 ? wavelet_->centerSample() 
				     : wavelet_->centerSample()-1;
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
	
	addToNrDone( 1 );
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
    if ( !synthgens_.isEmpty() )
	synthgens_[0]->getSampledReflectivities( rfs );
}



RaySynthGenerator::RaySynthGenerator()
    : raysampling_(0,0)
{}


RaySynthGenerator::~RaySynthGenerator()
{
    deepErase( raymodels_ );
}


void RaySynthGenerator::addModel( const ElasticModel& aim )
{
    aimodels_ += aim;
}


void RaySynthGenerator::setRayParams( const RayTracer1D::Setup& su, 
				  const TypeSet<float>& offs, bool isnmo )
{
    raysetup_ = su;
    offsets_ = offs;
    usenmotimes_ = isnmo;
}


bool RaySynthGenerator::doWork( TaskRunner* tr )
{
    return doRayTracing( tr ) && doSynthetics( tr );
}


bool RaySynthGenerator::doRayTracing( TaskRunner* tr )
{
    deepErase( raymodels_ );
    if ( offsets_.isEmpty() )
	offsets_ += 0;

    if ( aimodels_.isEmpty() )
	mErrRet( "No AI model found" );

    RayTracerRunner rtr( aimodels_, offsets_, raysetup_ );
    if ( tr && !tr->execute( rtr ) )
	mErrRet( rtr.errMsg(); )
    else if ( !rtr.execute() )
	mErrRet( rtr.errMsg(); )

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
	    raysampling_.include( d2t.getLastTime() );
	}
	raymodels_.insertAt( rm, 0 );
    }
    if ( !raysampling_.width() )
	mErrRet( "no valid time generated from raytracing" );
    return true;
}


bool RaySynthGenerator::doSynthetics( TaskRunner* tr )
{
    if ( !wavelet_ )
	mErrRet( "no wavelet found" )
    if ( mIsUdf( outputsampling_.start ) )
	outputsampling_ = raysampling_; 
    if ( mIsUdf( outputsampling_.step ) )
	outputsampling_.step = wavelet_->sampleRate();
    if ( outputsampling_.width()/(float)outputsampling_.step<wavelet_->size() )
	mErrRet( "Time range can not be smaller than wavelet" )
    if ( outputsampling_.nrSteps() < 1 )
	mErrRet( "Time interval is empty" );

    IOPar par; fillPar( par );
    for ( int idx=0; idx<raymodels_.size(); idx++ )
    {
	RayModel& rm = *raymodels_[idx];
	deepErase( rm.outtrcs_ );

	rm.sampledrefs_.erase();
	MultiTraceSynthGenerator multitracegen;
	multitracegen.setModels( rm.refmodels_ );
	multitracegen.setWavelet( wavelet_, OD::UsePtr );
	multitracegen.setOutSampling( outputsampling_ );
	multitracegen.usePar( par );

	if ( tr && !tr->execute( multitracegen ) )
	    mErrRet( multitracegen.errMsg(); )
	else if ( !multitracegen.execute() )
	    mErrRet( multitracegen.errMsg())

	multitracegen.result( rm.outtrcs_ );
	for ( int idoff=0; idoff<offsets_.size(); idoff++ )
	    multitracegen.getSampledReflectivities( rm.sampledrefs_ );
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
			ObjectSet<const SeisTrc>& trcs, bool steal )
{
    mGet( outtrcs_, trcs, steal );
}


void RaySynthGenerator::RayModel::getRefs( 
			ObjectSet<const ReflectivityModel>& trcs, bool steal )
{
    mGet( refmodels_, trcs, steal );
}


void RaySynthGenerator::RayModel::getD2T( 
			ObjectSet<const TimeDepthModel>& trcs, bool steal )
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

}// namespace

