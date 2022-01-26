/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/


#include "testprog.h"

#include "ailayer.h"
#include "raytrace1d.h"

#define mDefTimeEps 1e-6f
#define mDefDepthEps 1e-2f

#define mTestVal(computedval,expectedval,eps) \
    if ( !mIsEqual(computedval,expectedval,eps) ) \
    { \
	BufferString err( "got: ", computedval ) ; \
	err.add( ", expected: " ).add( expectedval ); \
	mRunStandardTestWithError( false, "Computed values differ", err ) \
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


static bool testRayTracer( const RayTracer1D::Setup& rtsu )
{
    const ElasticModel emdl = getEModel();

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

    ConstRefMan<OffsetReflectivityModel> retmodels = raytracer.getRefModel();
    mRunStandardTest( retmodels, "Retrieve output model from raytracer" );

    const TimeDepthModelSet& tdmodelset = *retmodels.ptr();
    const TimeDepthModel& tdmodelz0 = tdmodelset.getDefaultModel();
    mTestVal(tdmodelz0.getDepth(1),48.f,mDefDepthEps);
    mTestVal(tdmodelz0.getDepth(2),568.f,mDefDepthEps);
    mTestVal(tdmodelz0.getDepth(3),953.f,mDefDepthEps);
    mTestVal(tdmodelz0.getDepth(4),1303.f,mDefDepthEps);
    mTestVal(tdmodelz0.getTime(1),0.048f,mDefTimeEps);
    mTestVal(tdmodelz0.getTime(2),0.448f,mDefTimeEps);
    mTestVal(tdmodelz0.getTime(3),0.668f,mDefTimeEps);
    mTestVal(tdmodelz0.getTime(4),0.843f,mDefTimeEps);

    const TimeDepthModel* tdmodeloff2 = tdmodelset.get( 1 );
    mRunStandardTest( retmodels, "Retrieve offset trace from raytracer" );

    mTestVal(tdmodeloff2->getDepth(1),48.f,mDefDepthEps);
    mTestVal(tdmodeloff2->getDepth(2),568.f,mDefDepthEps);
    mTestVal(tdmodeloff2->getDepth(3),953.f,mDefDepthEps);
    mTestVal(tdmodeloff2->getDepth(4),1303.f,mDefDepthEps);

    mRunStandardTest( true, "RayTracer values" );

    return true;
}


static bool testBaseRayTracer()
{
    RayTracer1D::Setup rtsu;
    rtsu.doreflectivity( false );
    return testRayTracer( rtsu );
}


static bool testRefRayTracer()
{
    RayTracer1D::Setup rtsu;
    rtsu.doreflectivity( true );
    return testRayTracer( rtsu );
}


static bool testTDModelSet()
{
    const ElasticModel emdl = getEModel();
    const int nrlayers = emdl.size();

    const TimeDepthModelSet::Setup tdmssu;

    ConstRefMan<TimeDepthModelSet> simple = new TimeDepthModelSet( emdl );
    TypeSet<float> offsets;
    ConstRefMan<TimeDepthModelSet> emptyts =
			new TimeDepthModelSet( emdl, tdmssu, &offsets );
    offsets += 0.f;
    ConstRefMan<TimeDepthModelSet> zeroff =
			new TimeDepthModelSet( emdl, tdmssu, &offsets );
    offsets.append( getOffsets() );

    TypeSet<float> velmax( nrlayers, mUdf(float) );
    ConstRefMan<TimeDepthModelSet> psmodel =
		new TimeDepthModelSet( emdl, tdmssu, &offsets, velmax.arr() );
    offsets = getOffsets();
    velmax.setAll( mUdf(float) );
    ConstRefMan<TimeDepthModelSet> psmodel2 =
		new TimeDepthModelSet( emdl, tdmssu, &offsets, velmax.arr() );

    const int modsz = nrlayers+1;
    mRunStandardTest( simple->isOK() && simple->modelSize() == modsz &&
		      simple->size() == 1 && simple->get(0),
		      "Test simple" );
    mRunStandardTest( emptyts->isOK() && emptyts->modelSize() == modsz &&
		      emptyts->size() == 1 && emptyts->get(0),
		      "Test emptyts" );
    mRunStandardTest( zeroff->isOK() && zeroff->modelSize() == modsz &&
		      zeroff->size() == 1 && zeroff->get(0),
		      "Test zeroff" );
    mRunStandardTest( psmodel->isOK() && psmodel->modelSize() == modsz &&
		      psmodel->size() == 3 && psmodel->get(2),
		      "Test psmodel" );
    mRunStandardTest( psmodel2->isOK() && psmodel2->modelSize() == modsz &&
		      psmodel2->size() == 2 && psmodel2->get(1),
		      "Test psmodel2" );

    const TimeDepthModel& tdmodel = psmodel2->getDefaultModel();
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


static bool testOffRefModelSet( bool withangles, bool withrefs )
{
    const ElasticModel emdl = getEModel();
    const int nrlayers = emdl.size();

    const OffsetReflectivityModel::Setup refmssu( withangles, withrefs );

    ConstRefMan<OffsetReflectivityModel> simple =
	new OffsetReflectivityModel( emdl, refmssu );
    TypeSet<float> offsets;
    ConstRefMan<OffsetReflectivityModel> emptyts =
	new OffsetReflectivityModel( emdl, refmssu, &offsets );
    offsets += 0.f;
    ConstRefMan<OffsetReflectivityModel> zeroff =
	new OffsetReflectivityModel( emdl, refmssu, &offsets );
    offsets.append( getOffsets() );

    TypeSet<float> velmax( nrlayers, mUdf( float ) );
    ConstRefMan<OffsetReflectivityModel> psmodel =
	new OffsetReflectivityModel( emdl, refmssu, &offsets, velmax.arr());
    offsets = getOffsets();
    velmax.setAll( mUdf( float ) );
    ConstRefMan<OffsetReflectivityModel> psmodel2 =
	new OffsetReflectivityModel( emdl, refmssu, &offsets, velmax.arr());

    mRunStandardTest( psmodel2->hasAngles() == withangles &&
		      psmodel2->hasReflectivities() == withrefs,
		      "Test content" );

    const int modsz = nrlayers + 1;
    mRunStandardTest( simple->isOK() && simple->modelSize() == modsz &&
	simple->size() == 1 && simple->get(0),
	"Test simple" );
    mRunStandardTest( emptyts->isOK() && emptyts->modelSize() == modsz &&
	emptyts->size() == 1 && emptyts->get(0),
	"Test emptyts" );
    mRunStandardTest( zeroff->isOK() && zeroff->modelSize() == modsz &&
	zeroff->size() == 1 && zeroff->get(0),
	"Test zeroff" );
    mRunStandardTest( psmodel->isOK() && psmodel->modelSize() == modsz &&
	psmodel->size() == 3 && psmodel->get(2),
	"Test psmodel" );
    mRunStandardTest( psmodel2->isOK() && psmodel2->modelSize() == modsz &&
	psmodel2->size() == 2 && psmodel2->get(1),
	"Test psmodel2" );

    const TimeDepthModel& tdmodel = psmodel2->getDefaultModel();
    mTestVal( tdmodel.getDepth( 1 ), 48.f, mDefDepthEps );
    mTestVal( tdmodel.getDepth( 2 ), 568.f, mDefDepthEps );
    mTestVal( tdmodel.getDepth( 3 ), 953.f, mDefDepthEps );
    mTestVal( tdmodel.getDepth( 4 ), 1303.f, mDefDepthEps );
    mTestVal( tdmodel.getTime( 1 ), 0.048f, mDefTimeEps );
    mTestVal( tdmodel.getTime( 2 ), 0.448f, mDefTimeEps );
    mTestVal( tdmodel.getTime( 3 ), 0.668f, mDefTimeEps );
    mTestVal( tdmodel.getTime( 4 ), 0.843f, mDefTimeEps );

    mTestVal( velmax[0], 2000.f, mDefDepthEps );
    mTestVal( velmax[1], 2542.49585f, mDefDepthEps );
    mTestVal( velmax[2], 2893.05396f, mDefDepthEps );
    mTestVal( velmax[3], 3154.95435f, mDefDepthEps );

    mRunStandardTest( true, "Offset-based ReflectivityModelSet values" );

    return true;
}


static bool testAngRefModelSet()
{
    const ElasticModel emdl = getEModel();
    const int nrlayers = emdl.size();

    TypeSet<float> angles;
    ConstRefMan<AngleReflectivityModel> emptyts =
	new AngleReflectivityModel( emdl, angles );
    angles += 0.f;
    ConstRefMan<AngleReflectivityModel> zeroff =
	new AngleReflectivityModel( emdl, angles, 20.f );
    angles += 10.f;
    angles += 20.f;

    ConstRefMan<AngleReflectivityModel> psmodel =
	new AngleReflectivityModel( emdl, angles, 30.f );

    angles -= 0.f;
    ConstRefMan<AngleReflectivityModel> psmodel2 =
	new AngleReflectivityModel( emdl, angles, 40.f );

    const int modsz = nrlayers + 1;
    mRunStandardTest( emptyts->isOK() && emptyts->modelSize() == modsz &&
	emptyts->size() == 1 && emptyts->get(0),
	"Test emptyts" );
    mRunStandardTest( zeroff->isOK() && zeroff->modelSize() == modsz &&
	zeroff->size() == 1 && zeroff->get(0),
	"Test zeroff" );
    mRunStandardTest( psmodel->isOK() && psmodel->modelSize() == modsz &&
	psmodel->size() == 1 && psmodel->get( 0 ) &&
	psmodel->nrRefModels() == 3,
	"Test psmodel" );
    mRunStandardTest( psmodel2->isOK() && psmodel2->modelSize() == modsz &&
	psmodel2->size() == 1 && psmodel2->get( 0 ) &&
	psmodel2->nrRefModels() == 2,
	"Test psmodel2" );

    const TimeDepthModel& tdmodel = psmodel2->getDefaultModel();
    mTestVal( tdmodel.getDepth( 1 ), 48.f, mDefDepthEps );
    mTestVal( tdmodel.getDepth( 2 ), 568.f, mDefDepthEps );
    mTestVal( tdmodel.getDepth( 3 ), 953.f, mDefDepthEps );
    mTestVal( tdmodel.getDepth( 4 ), 1303.f, mDefDepthEps );
    mTestVal( tdmodel.getTime( 1 ), 0.048f, mDefTimeEps );
    mTestVal( tdmodel.getTime( 2 ), 0.448f, mDefTimeEps );
    mTestVal( tdmodel.getTime( 3 ), 0.668f, mDefTimeEps );
    mTestVal( tdmodel.getTime( 4 ), 0.843f, mDefTimeEps );

    mRunStandardTest( true, "Angle-based ReflectivityModelSet values" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testBaseRayTracer() || !testRefRayTracer() ||
	 !testTDModelSet() ||
	 !testOffRefModelSet(false,false) ||
	 !testOffRefModelSet(false,true) ||
	 !testOffRefModelSet(true,false) ||
	 !testOffRefModelSet(true,true) ||
	 !testAngRefModelSet() )
	return 1;

    return 0;
}
