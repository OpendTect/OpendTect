/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/


#include "timedepthmodel.h"
#include "raytrace1d.h"
#include "velocitycalc.h"
#include "testprog.h"

#define mDefTimeEps 1e-6f
#define mDefDepthEps 1e-2f

#define mErrMsg( fn, baseentity ) \
od_cout() << "Computation failed for " #fn " with:\n v0: " << v0 << "\ndv: " \
	  << dv << "\n v0depth: " << v0depth << "\n at: " << baseentity << "\n"
#define mTestVal(computedval,expectedval,eps) \
    if ( !mIsEqual(computedval,expectedval,eps) ) \
    { \
	BufferString err( "got: ", computedval ) ; \
	err.add( ", expected: " ).add( expectedval ); \
	mRunStandardTestWithError( false, "Computed values differ", err ) \
    }

bool testLinearT2D( double v0, double dv, double v0depth, float time,
		    float expecteddepth )
{
    float res;
    if ( !computeLinearT2D( v0, dv, v0depth, SamplingData<float>(time,0),
			    1, &res ))
    {
	mErrMsg( testLinearT2D, time );
	return false;
    }

    if ( !mIsEqual( res, expecteddepth,  1e-3 ))
    {
	mErrMsg( testLinearT2D, time );
	od_cout() << "Expected depth: " << expecteddepth << "\n";
	od_cout() << "Computed depth: " << res << "\n";
	return false;
    }

    return true;
}


bool testLinearD2T( double v0, double dv, double v0depth, float depth,
		    float expectedtime )
{
    float res;
    if ( !computeLinearD2T( v0, dv, v0depth, SamplingData<float>(depth,0),
			   1, &res ))
    {
	mErrMsg( testLinearD2T, depth );
	return false;
    }

    if ( !mIsEqual( res, expectedtime,  1e-3 ))
    {
	mErrMsg( testLinearD2T, depth );
	od_cout() << "Expected time: " << expectedtime << "\n";
	od_cout() << "Computed time: " << res << "\n";
	return false;
    }

    return true;
}


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
    return offsets;
}


static bool testRayTracer()
{
    const ElasticModel emdl = getEModel();

    RayTracer1D::Setup rtsu;
    rtsu.doreflectivity( false );

    VrmsRayTracer1D raytracer;
    raytracer.setup() = rtsu;
    raytracer.setOffsets( getOffsets() );
    raytracer.setModel( emdl );
    mRunStandardTest( raytracer.execute(), "VRMSRayTracer execution" );
    mTestVal(raytracer.getDepth(0),48.f,mDefDepthEps);
    mTestVal(raytracer.getDepth(1),568.f,mDefDepthEps);
    mTestVal(raytracer.getDepth(2),953.f,mDefDepthEps);
    mTestVal(raytracer.getDepth(3),1303.f,mDefDepthEps);

    // second offset, at 200m:
    mTestVal(raytracer.getTime(0,1),0.110923395f,mDefTimeEps);
    mTestVal(raytracer.getTime(1,1),0.454853654f,mDefTimeEps);
    mTestVal(raytracer.getTime(2,1),0.671567619f,mDefTimeEps);
    mTestVal(raytracer.getTime(3,1),0.845380187f,mDefTimeEps);

    TimeDepthModel tdmodel;
    mRunStandardTest( raytracer.getZeroOffsTDModel(tdmodel),
		      "Retrieve zero-offset TD model from raytracer" );
    mTestVal(tdmodel.getDepth(1),48.f,mDefDepthEps);
    mTestVal(tdmodel.getDepth(2),568.f,mDefDepthEps);
    mTestVal(tdmodel.getDepth(3),953.f,mDefDepthEps);
    mTestVal(tdmodel.getDepth(4),1303.f,mDefDepthEps);
    mTestVal(tdmodel.getTime(1),0.048f,mDefTimeEps);
    mTestVal(tdmodel.getTime(2),0.448f,mDefTimeEps);
    mTestVal(tdmodel.getTime(3),0.668f,mDefTimeEps);
    mTestVal(tdmodel.getTime(4),0.843f,mDefTimeEps);


    mRunStandardTest( true, "RayTracer values" );

    return true;
}


static bool testTDModelSet()
{
    const ElasticModel emdl = getEModel();
    const int nrlayers = emdl.size();

    const RayTracer1D::Setup rtsu;

    const TimeDepthModelSet simple( emdl );
    TypeSet<float> offsets;
    const TimeDepthModelSet emptyts( emdl, &offsets );
    offsets += 0.f;
    const TimeDepthModelSet zeroff( emdl, &offsets );
    offsets.append( getOffsets() );

    TypeSet<float> velmax( nrlayers, mUdf(float) );
    const TimeDepthModelSet psmodel( emdl, &offsets, rtsu.pup_, rtsu.pdown_,
				     velmax.arr() );
    offsets = getOffsets();
    velmax.setAll( mUdf(float) );
    const TimeDepthModelSet psmodel2( emdl, &offsets, rtsu.pup_, rtsu.pdown_,
				      velmax.arr() );

    const int modsz = nrlayers+1;
    mRunStandardTest( simple.isOK() && simple.modelSize() == modsz &&
		      simple.size() == 1 && simple.get( 0 ) &&
		      !simple.axisVals(), "Test simple" );
    mRunStandardTest( emptyts.isOK() && emptyts.modelSize() == modsz &&
		      emptyts.size() == 1 && emptyts.get( 0 ) &&
		      !emptyts.axisVals(), "Test emptyts" );
    mRunStandardTest( zeroff.isOK() && zeroff.modelSize() == modsz &&
		      zeroff.size() == 1 && zeroff.get( 0 ) &&
		      !zeroff.axisVals(), "Test zeroff" );
    mRunStandardTest( psmodel.isOK() && psmodel.modelSize() == modsz &&
		      psmodel.size() == 3 && psmodel.get( 2 ) &&
		      psmodel.axisVals(), "Test psmodel" );
    mRunStandardTest( psmodel2.isOK() && psmodel2.modelSize() == modsz &&
		      psmodel2.size() == 2 && psmodel2.get(1) &&
		      psmodel2.axisVals(), "Test psmodel2" );

    const TimeDepthModel& tdmodel = psmodel2.getDefaultModel();
    mTestVal(tdmodel.getDepth(1),48.f,mDefDepthEps);
    mTestVal(tdmodel.getDepth(2),568.f,mDefDepthEps);
    mTestVal(tdmodel.getDepth(3),953.f,mDefDepthEps);
    mTestVal(tdmodel.getDepth(4),1303.f,mDefDepthEps);
    mTestVal(tdmodel.getTime(1),0.048f,mDefTimeEps);
    mTestVal(tdmodel.getTime(2),0.448f,mDefTimeEps);
    mTestVal(tdmodel.getTime(3),0.668f,mDefTimeEps);
    mTestVal(tdmodel.getTime(4),0.843f,mDefTimeEps);

    mTestVal(velmax[0],2000.f,mDefDepthEps);
    mTestVal(velmax[1],2542.49585f,mDefDepthEps);
    mTestVal(velmax[2],2893.05396f,mDefDepthEps);
    mTestVal(velmax[3],3154.95435f,mDefDepthEps);

    mRunStandardTest( true, "TimeDepthModelSet values" );

    return true;
}



int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testLinearT2D( 2000, 0, 0, 0, 0) ||
	    !testLinearT2D( 2000, 0, 0, 6, 6000) ||
	    !testLinearT2D( 2000, 0, -200, 0, -200) ||
	    !testLinearT2D( 2000, 0, -200, 6, 5800) ||
	    !testLinearT2D( 2000, 0.1, 0, 0, 0) ||
	    !testLinearT2D( 2000, 0.1, 0, 6, 6997.176) ||
	    !testLinearT2D( 2000, 0.1, -200, 0, -200) ||
	    !testLinearT2D( 2000, 0.1, -200, 6, 6727.204) )
	return 1;

    if ( !testLinearD2T( 2000, 0, 0, 0, 0) ||
	    !testLinearD2T( 2000, 0, 0, 6000, 6) ||
	    !testLinearD2T( 2000, 0, -200, -200, 0) ||
	    !testLinearD2T( 2000, 0, -200, 5800, 6) ||
	    !testLinearD2T( 2000, 0.1, 0, 0, 0) ||
	    !testLinearD2T( 2000, 0.1, 0, 6997.176, 6) ||
	    !testLinearD2T( 2000, 0.1, -200, -200, 0) ||
	    !testLinearD2T( 2000, 0.1, -200, 6727.204, 6) )
	return 1;

    if ( !testRayTracer() ||
	 !testTDModelSet() )
	return 1;

    return 0;
}
