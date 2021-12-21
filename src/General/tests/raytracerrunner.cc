/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/


#include "testprog.h"

#include "ailayer.h"
#include "moddepmgr.h"
#include "raytracerrunner.h"

#define mDefTimeEps 1e-6f
#define mDefDepthEps 1e-2f

#define mTestVal(computedval,expectedval,eps) \
    if ( !mIsEqual(computedval,expectedval,eps) ) \
    { \
	BufferString err( "got: ", computedval ) ; \
	err.add( ", expected: " ).add( expectedval ); \
	mRunStandardTestWithError( false, "Computed values differ", err ) \
    }

#define cNrModels 8

static ElasticModel getEModel()
{
    ElasticModel emdl;
    emdl += AILayer( 48.f, 2000.f, 2500.f );
    emdl += AILayer( 520.f, 2600.f, 2300.f );
    emdl += AILayer( 385.f, 3500.f, 2200.f );
    emdl += AILayer( 350.f, 4000.f, 2800.f );
    return emdl;
}


static TypeSet<float> getOffsets()
{
    TypeSet<float> offsets;
    offsets += 100.f;
    offsets += 200.f;
    offsets += 300.f;
    offsets += 400.f;
    offsets += 500.f;
    offsets += 600.f;
    return offsets;
}


static bool doRun( const TypeSet<ElasticModel>& emodels, bool parallel )
{
    const bool singlemod = emodels.size() == 1;
    const TypeSet<float> offsets = getOffsets();

    RayTracerRunner rtrunner;
    rtrunner.setModel( emodels );
    rtrunner.setOffsets( offsets );

    mRunStandardTestWithError( rtrunner.executeParallel(parallel),
			       "Execute raytracer runner",
			       toString(rtrunner.uiMessage()) );

    const RayTracer1D& rt = singlemod ? *rtrunner.rayTracers().first()
				      : *rtrunner.rayTracers().get(cNrModels-1);

    TypeSet<float> retoffsets;
    rt.getOffsets( retoffsets );
    mRunStandardTest( retoffsets == offsets, "Offset distribution" );

    const ElasticModel& emdl = singlemod ? emodels.first()
					 : emodels[cNrModels-1];
    const int nrlayers = emdl.size();

    ConstRefMan<TimeDepthModelSet> retmodel = rt.getTDModels();

    const int modsz = nrlayers + 1;
    mRunStandardTest( retmodel->isOK() && retmodel->modelSize() == modsz &&
	retmodel->size() == 6 && retmodel->get( 5 ),
	"Test raytracerrunner output" );

    const TimeDepthModel& tdmodel = retmodel->getDefaultModel();
    mTestVal( tdmodel.getDepth( 1 ), 48.f, mDefDepthEps );
    mTestVal( tdmodel.getDepth( 2 ), 568.f, mDefDepthEps );
    mTestVal( tdmodel.getDepth( 3 ), 953.f, mDefDepthEps );
    mTestVal( tdmodel.getDepth( 4 ), 1303.f, mDefDepthEps );
    mTestVal( tdmodel.getTime( 1 ), 0.048f, mDefTimeEps );
    mTestVal( tdmodel.getTime( 2 ), 0.448f, mDefTimeEps );
    mTestVal( tdmodel.getTime( 3 ), 0.668f, mDefTimeEps );
    mTestVal( tdmodel.getTime( 4 ), 0.843f, mDefTimeEps );

    mDynamicCastGet(const ReflectivityModelSet*,refmodel,retmodel.ptr())
    mRunStandardTest( refmodel && refmodel->nrRefModels() == 6 &&
		      refmodel->isDefined(5,3),
		      "Has defined reflectivities" );

    mRunStandardTest( true, "Offset-based ReflectivityModelSet values" );

    return true;
}


static bool runRayTracerRunner( int nr, bool parallel )
{
    TypeSet<ElasticModel> emodels;
    for ( int idx=0; idx<nr; idx++ )
	emodels += getEModel();

    return doRun( emodels, parallel );
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    OD::ModDeps().ensureLoaded("General");

    if ( !runRayTracerRunner(1,false) ||
	 !runRayTracerRunner(1,true) ||
	 !runRayTracerRunner(cNrModels,false) ||
	 !runRayTracerRunner(cNrModels,true) )
	return 1;

    return 0;
}
