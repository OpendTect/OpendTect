/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"
#include "moddepmgr.h"

#include "ailayer.h"
#include "fourier.h"
#include "raytrace1d.h"
#include "reflectivitysampler.h"

#define mDefTimeEps 1e-6f
#define mDefDepthEps 1e-2f
#define mDefRefEps 1e-6f

#define mTestVal(computedval,expectedval,eps) \
    if ( !mIsEqual(computedval,expectedval,eps) ) \
    { \
	BufferString err( "got: ", computedval ) ; \
	err.add( ", expected: " ).add( expectedval ); \
	mRunStandardTestWithError( false, "Computed values differ", err ) \
    }

#define cNrModels 8

static ConstRefMan<ReflectivityModelBase> getRefModel()
{
    ElasticModel emdl;
    emdl.add( new AILayer( 48.f, 2000.f, 2500.f ) );
    emdl.add( new AILayer( 385.f, 3500.f, 2200.f ) );
    emdl.add( new AILayer( 350.f, 4000.f, 2800.f ) );

    uiString msg;
    IOPar raypars;
    raypars.set( sKey::Type(), VrmsRayTracer1D::sFactoryKeyword() );
    PtrMan<RayTracer1D> raytracer = RayTracer1D::createInstance( raypars,
								 &emdl, msg );
    if ( !raytracer || !raytracer->execute() )
	return nullptr;

    ConstRefMan<OffsetReflectivityModel> refmodel = raytracer->getRefModel();
    raytracer = nullptr;
    return ConstRefMan<ReflectivityModelBase>( refmodel.ptr() );
}


static bool testInputModel( const ReflectivityModelBase& refmodel )
{
    mRunStandardTest( refmodel.nrRefModels() == 1, "Number of offsets" )
    mRunStandardTest( refmodel.nrLayers() == 3, "Number of layers" )
    mRunStandardTest( refmodel.nrSpikes() == 2, "Number of spikes" )
    mRunStandardTest( refmodel.isSpikeDefined(0,0) &&
		      refmodel.isSpikeDefined(0,1), "Spikes are defined" )

    const ReflectivityModelTrace* inpreftrc = refmodel.getReflectivities(0);
    const float_complex* refarr = inpreftrc ? inpreftrc->arr() : nullptr;
    const float* reftimes = refmodel.getReflTimes();
    mRunStandardTest( refarr && reftimes, "Valid refmodel" )
    mTestVal( refarr[0].real(), 0.212598428f, mDefRefEps);
    mTestVal( refarr[1].real(), 0.185185179f, mDefRefEps);
    mTestVal( reftimes[0], 0.048f, mDefTimeEps);
    mTestVal( reftimes[1], 0.268f, mDefTimeEps);

    return true;
}


static int getFFTSz( const ZSampling& zrg )
{
    const int sz = zrg.nrSteps() + 1;
    PtrMan<Fourier::CC> fft = new Fourier::CC;
    return fft->getFastSize( sz );
}


static bool testSampledFreqs( const float_complex* refs, int bufsz )
{
    mTestVal( refs[0].real(), 0.397783607f, mDefRefEps );
    mTestVal( refs[7].real(), -0.164669171f, mDefRefEps );
    mTestVal( refs[7].imag(), 0.178875163f, mDefRefEps );
    mTestVal( refs[8].real(), -0.123888724f, mDefRefEps );
    mTestVal( refs[8].imag(), -0.0801410004f, mDefRefEps );
    mTestVal( refs[160].real(), -0.123887263f, mDefRefEps );
    mTestVal( refs[160].imag(), 0.0801407322f, mDefRefEps );
    mTestVal( refs[167].real(), 0.0425443649f, mDefRefEps );
    mTestVal( refs[167].imag(), 0.202209145f, mDefRefEps );

    return true;
}


static bool testSampledTimes( const float_complex* refs, int bufsz )
{
    mTestVal( refs[0].real(), -1.27393758e-08f, mDefRefEps );
    mTestVal( refs[0].imag(), 7.22123659e-08f, mDefRefEps );
    mTestVal( refs[5].real(), 0.212598279f, mDefRefEps );
    mTestVal( refs[5].imag(), 0.f, mDefRefEps );
    mTestVal( refs[7].real(), -6.18999465e-08f, mDefRefEps );
    mTestVal( refs[7].imag(), 2.74857506e-08f, mDefRefEps );
    mTestVal( refs[60].real(), 0.185185179f, mDefRefEps );
    mTestVal( refs[60].imag(), 0.f, mDefRefEps );
    mTestVal( refs[160].real(), -5.13662446e-09f, mDefRefEps );
    mTestVal( refs[160].imag(), 1.15412551e-08f, mDefRefEps );
    mTestVal( refs[161].real(), 1.33755663e-07f, mDefRefEps );
    mTestVal( refs[161].imag(), -9.69852039e-08f, mDefRefEps );
    mTestVal( refs[167].real(), -2.0395662e-08f, mDefRefEps );
    mTestVal( refs[167].imag(), -4.59706371e-08f, mDefRefEps );

    return true;
}


static bool testRunSampler( const ReflectivityModelBase& refmodel,
			    const ZSampling& twtrg, bool dofreq, bool dotime,
			    bool usebuf )
{
    ReflectivitySampler sampler;
    sampler.setInput( *refmodel.getReflectivities(0),
		      refmodel.getReflTimes(), twtrg );

    const int fftsz = getFFTSz( twtrg );
    RefMan<ReflectivityModelTrace> sampledfreqreflectivities,
				   sampledtimereflectivities,
				   creflectitivies;
    if ( dofreq )
    {
	sampledfreqreflectivities = new ReflectivityModelTrace( fftsz );
	sampler.setFreqOutput( *sampledfreqreflectivities.ptr() );
    }

    if ( dotime )
    {
	sampledtimereflectivities = new ReflectivityModelTrace( fftsz );
	if ( usebuf )
	{
	    creflectitivies = new ReflectivityModelTrace( fftsz );
	    sampler.setTimeOutput( *sampledtimereflectivities.ptr(),
				   creflectitivies->arr(),
				   creflectitivies->size() );
	}
	else
	    sampler.setTimeOutput( *sampledtimereflectivities.ptr() );
    }

    mRunStandardTest( sampler.execute(), "Run reflectivity sampler" );
    if ( dofreq )
	mRunStandardTest(
		testSampledFreqs(sampledfreqreflectivities->arr(),fftsz),
			  "Computed frequency reflectivities are OK" )

    if ( dotime )
	mRunStandardTest(
		testSampledTimes(sampledtimereflectivities->arr(),fftsz),
			  "Computed time reflectivities are OK" )

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    OD::ModDeps().ensureLoaded("General");

    ConstRefMan<ReflectivityModelBase> refmodel = getRefModel();
    if ( !refmodel )
    {
	tstStream(true) << "Cannot compute input reflectivity model" << od_endl;
	return 1;
    }

    const ZSampling outputsampling( 0.028f, 0.696f, 0.004f );

    if ( !testInputModel(*refmodel.ptr()) ||
	 !testRunSampler(*refmodel.ptr(),outputsampling,true,false,false) ||
	 !testRunSampler(*refmodel.ptr(),outputsampling,true,false,true) ||
	 !testRunSampler(*refmodel.ptr(),outputsampling,false,true,false) ||
	 !testRunSampler(*refmodel.ptr(),outputsampling,false,true,true) ||
	 !testRunSampler(*refmodel.ptr(),outputsampling,true,true,false) ||
	 !testRunSampler(*refmodel.ptr(),outputsampling,true,true,true) )
	return 1;

    return 0;
}
