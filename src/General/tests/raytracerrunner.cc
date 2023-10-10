/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "ailayer.h"
#include "moddepmgr.h"
#include "raytracerrunner.h"
#include "reflcalcrunner.h"

#define mDefTimeEps 1e-6f
#define mDefDepthEps 1e-2f
#define mDefEpsVal 1e-3f

#define mTestVal(computedval,expectedval,eps) \
    if ( !mIsEqual(computedval,expectedval,eps) ) \
    { \
	BufferString err( "got: ", computedval ) ; \
	err.add( ", expected: " ).add( expectedval ); \
	mRunStandardTestWithError( false, "Computed values differ", err ) \
    }

#define cNrModels 8

static ElasticModel* getEModel()
{
    auto* emdl = new ElasticModel();
    emdl->add( new AILayer( 48.f, 2000.f, 2500.f ) );
    emdl->add( new AILayer( 520.f, 2600.f, 2300.f ) );
    emdl->add( new AILayer( 385.f, 3500.f, 2200.f ) );
    emdl->add( new AILayer( 350.f, 4000.f, 2800.f ) );
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


static TypeSet<float> getAngles()
{
    TypeSet<float> angles;
    angles += 5.f;
    angles += 10.f;
    angles += 15.f;
    angles += 20.f;
    angles += 25.f;
    angles += 30.f;
    return angles;
}


static bool doRun( bool raytracer, const ElasticModelSet& emodels,
		   bool parallel )
{
    const bool singlemod = emodels.size() == 1;
    const TypeSet<float> xaxisvals = raytracer ? getOffsets() : getAngles();

    IOPar iop;
    iop.set( sKey::Type(), raytracer ? VrmsRayTracer1D::sFactoryKeyword()
				     : AICalc1D::sFactoryKeyword() );
    iop.set( raytracer ? RayTracer1D::sKeyOffset()
		       : ReflCalc1D::sKeyAngle(), xaxisvals );
    if ( !raytracer )
	iop.setYN( ReflCalc1D::sKeyAngleInDegrees(), true );
    iop.setYN( RayTracerRunner::sKeyParallel(), parallel );

    uiString msg;
    ConstRefMan<ReflectivityModelSet> refmodels = raytracer
			? RayTracerRunner::getRefModels( emodels, iop, msg )
			: ReflCalcRunner::getRefModels( emodels, iop, msg );
    mRunStandardTestWithError( refmodels.ptr(), "Execute runner",
			       toString(msg) );

    const int modidx = singlemod ? 0 : cNrModels-1;
    const ReflectivityModelBase* refmodel = refmodels->get( modidx );
    mRunStandardTest( refmodel, "Has reflectivity model" );

    const ElasticModel& emdl = singlemod ? *emodels.first()
					 : *emodels.get(cNrModels-1);
    const int nrlayers = emdl.size();
    const int nrd2t = raytracer ? 6 : 1;
    const int modsz = nrlayers + 1;
    mRunStandardTest( refmodel->isOK() && refmodel->modelSize() == modsz &&
		      refmodel->size() == nrd2t &&
		      ((raytracer && refmodel->get(5)) || (!raytracer) ),
		      "Test runner output" );

    const TimeDepthModel& tdmodel = refmodel->getDefaultModel();
    mTestVal( tdmodel.getDepth( 1 ), 48.f, mDefDepthEps );
    mTestVal( tdmodel.getDepth( 2 ), 568.f, mDefDepthEps );
    mTestVal( tdmodel.getDepth( 3 ), 953.f, mDefDepthEps );
    mTestVal( tdmodel.getDepth( 4 ), 1303.f, mDefDepthEps );
    mTestVal( tdmodel.getTime( 1 ), 0.048f, mDefTimeEps );
    mTestVal( tdmodel.getTime( 2 ), 0.448f, mDefTimeEps );
    mTestVal( tdmodel.getTime( 3 ), 0.668f, mDefTimeEps );
    mTestVal( tdmodel.getTime( 4 ), 0.843f, mDefTimeEps );

    mRunStandardTest( refmodel->nrRefModels() == 6 &&
		      refmodel->isSpikeDefined(5,2),
		      "Has defined reflectivities" );
    if ( raytracer )
    {
	mTestVal( refmodel->getReflectivities(5)->arr()[2].real(),
		  0.152156904f, mDefEpsVal );
    }
    else
    {
	mTestVal( refmodel->getReflectivities(0)->arr()[0].real(),
		  0.0892531872f, mDefEpsVal );
	mTestVal( refmodel->getReflectivities(0)->arr()[1].real(),
		  0.125730994f, mDefEpsVal );
	mTestVal( refmodel->getReflectivities(0)->arr()[2].real(),
		  0.185185185f, mDefEpsVal );
    }

    Interval<float> twtrg;
    refmodels->getTWTrange( twtrg, true );
    const Interval<float> exptwtrg( 0.048f, 0.843f );
    mRunStandardTest( twtrg.isEqual(exptwtrg,mDefTimeEps),
			"TWT range is correct" );

    mRunStandardTest( true, "ReflectivityModel values" );

    return true;
}


static bool setupAndApplyRunner( bool raytracer, int nr, bool parallel )
{
    ElasticModelSet emodels;
    for ( int idx=0; idx<nr; idx++ )
	emodels.add( getEModel() );

    if ( nr > 1 )
    {
	auto* emdl = new ElasticModel;
	// Single layer model
	emdl->add( new AILayer( 48.f, 2000.f, 2500.f ) );
	emodels.add( emdl->clone() );

	// Model with the identical layers: May get merged
	emdl->add( new AILayer( 48.f, 2000.f, 2500.f ) );
	emdl->add( new AILayer( 48.f, 2000.f, 2500.f ) );
	emodels.add( emdl );
    }

    return doRun( raytracer, emodels, parallel );
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    OD::ModDeps().ensureLoaded("General");

    if ( !setupAndApplyRunner(false,1,false) ||
	 !setupAndApplyRunner(false,1,true) ||
	 !setupAndApplyRunner(false,cNrModels,false) ||
	 !setupAndApplyRunner(false,cNrModels,true) ||
	 !setupAndApplyRunner(true,1,false) ||
	 !setupAndApplyRunner(true,1,true) ||
	 !setupAndApplyRunner(true,cNrModels,false) ||
	 !setupAndApplyRunner(true,cNrModels,true) )
	return 1;

    return 0;
}
