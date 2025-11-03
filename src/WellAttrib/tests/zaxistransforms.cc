/*+
________________________________________________________________________

Copyright:	(C) 1995-2022 dGB Beheer B.V.
License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emfault3d.h"
#include "emstoredobjaccess.h"
#include "emsurfaceiodata.h"
#include "emsurft2dtransformer.h"
#include "executor.h"
#include "ioman.h"
#include "moddepmgr.h"
#include "multiid.h"
#include "odpair.h"
#include "simpletimedepthmodel.h"
#include "surveydisklocation.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "unitofmeasure.h"

#define mErrRet(s) { errStream() << s << od_endl; return false; }

//Velocity Object IDs
static const int velids_direct[] = { 8, 16, 9, 9, 14, 14, 16 };

//Simple Time-Depth Parameters
static const MultiID tablet2did_m = MultiID( 100090, 6 );
static const MultiID tablet2did_ft = MultiID( 100090, 7 );

//Linear Vel Paramenters
static double v0()
{
    return LinearVelTransform::velUnit()->getUserValueFromSI( 3000.f );
}

static const double k = 0.1;

//SRD Paramter
static double srd_ = 15.24; // 50ft


#define mEps 1e-6f
//Model Info : [Nr. Surveys][Nr. ZAxistransforms][Nr. test Vals]
//Fault : Output Values
static const float zvalues_flt[7][8][2] =
{
    { //F3_Test_Survey
	{ 475.2301330f, 1043.8642578f }, //T2D : tablet2did_m
	{ 475.2301330f, 1043.8642578f }, //T2D : tablet2did_ft
	{ 718.2444970f, 1557.4968164f }, //T2D : Linear
	{ 458.2054040f, 1055.7767236f }, //T2D : Velocity
	{ 0.4831075f, 1.0219321f }, //D2T : tablet2did_m
	{ 0.4831075f, 1.0219321f }, //D2T : tablet2did_ft
	{ 0.3243359f, 0.6938919f }, //D2T : Linear
	{ 0.5004789f, 1.0120050f }  //D2T : Velocity
    },
    { //F3_Test_Survey_DepthFT
	{ 1559.1539796f, 3424.7515019f }, //T2D : tablet2did_m
	{ 1559.1539796f, 3424.7515019f }, //T2D : tablet2did_ft
	{ 2356.4450683f, 5109.8974609f }, //T2D : Linear
	{ 1503.2985839f, 3463.8337402f }, //T2D : Velocity
	{ 0.4831075f, 1.0219321f }, //D2T : tablet2did_m
	{ 0.4831075f, 1.0219321f }, //D2T : tablet2did_ft
	{ 0.3243359f, 0.6938919f }, //D2T : Linear
	{ 0.5004791f, 1.0120050f }  //D2T : Velocity
    },
    { //F3_Test_Survey_DisplayFT
	{ 1559.1539306f, 3424.7514648f }, //T2D : tablet2did_m
	{ 1559.1539306f, 3424.7514648f }, //T2D : tablet2did_ft
	{ 2356.4450683f, 5109.8974609f }, //T2D : Linear
	{ 1503.2985839f, 3463.8339843f }, //T2D : Velocity
	{ 0.4831075f, 1.0219321f }, //D2T : tablet2did_m
	{ 0.4831075f, 1.0219321f }, //D2T : tablet2did_ft
	{ 0.3243359f, 0.6938919f }, //D2T : Linear
	{ 0.5004789f, 1.0120050f }  //D2T : Velocity
    },
    { //F3_Test_Survey_XYinft
	{ 1559.1540527f, 3424.7514648f }, //T2D : tablet2did_m
	{ 1559.1540527f, 3424.7514648f }, //T2D : tablet2did_ft
	{ 2356.4453125f, 5109.8974609f }, //T2D : Linear
	{ 1503.2987060f, 3463.8339843f }, //T2D : Velocity
	{ 0.4831075f, 1.0219321f }, //D2T : tablet2did_m
	{ 0.4831075f, 1.0219321f }, //D2T : tablet2did_ft
	{ 0.3243359f, 0.6938920f }, //D2T : Linear
	{ 0.5004789f, 1.0120050f }  //D2T : Velocity
    },
    { //F3_Test_Survey_DepthM
	{ 475.2301330f, 1043.8642578f }, //T2D : tablet2did_m
	{ 475.2301330f, 1043.8642578f }, //T2D : tablet2did_ft
	{ 718.2444970f, 1557.4968164f }, //T2D : Linear
	{ 458.2054138f, 1055.7766113f }, //T2D : Velocity
	{ 0.4831075f, 1.0219321f }, //D2T : tablet2did_m
	{ 0.4831075f, 1.0219321f }, //D2T : tablet2did_ft
	{ 0.3243359f, 0.6938919f }, //D2T : Linear
	{ 0.5004795f, 1.0120050f }  //D2T : Velocity
    },
    { //F3_Test_Survey_DepthM_XYinft
	{ 475.2301330f, 1043.8642578f }, //T2D : tablet2did_m
	{ 475.2301330f, 1043.8642578f }, //T2D : tablet2did_ft
	{ 718.2444970f, 1557.4968164f }, //T2D : Linear
	{ 458.2054040f, 1055.7766113f }, //T2D : Velocity
	{ 0.4831075f, 1.0219321f }, //D2T : tablet2did_m
	{ 0.4831075f, 1.0219321f }, //D2T : tablet2did_ft
	{ 0.3243359f, 0.6938919f }, //D2T : Linear
	{ 0.5004789f, 1.0120050f }  //D2T : Velocity
    },
    { //F3_Test_Survey_DepthFT__XYinft_
	{ 1559.1540527f, 3424.7514648f }, //T2D : tablet2did_m
	{ 1559.1540527f, 3424.7514648f }, //T2D : tablet2did_ft
	{ 2356.4453125f, 5109.8974609f }, //T2D : Linear
	{ 1503.2987060f, 3463.8337402f }, //T2D : Velocity
	{ 0.4831075f, 1.0219321f }, //D2T : tablet2did_m
	{ 0.4831075f, 1.0219321f }, //D2T : tablet2did_ft
	{ 0.3243359f, 0.6938919f }, //D2T : Linear
	{ 0.5004789f, 1.0120050f }  //D2T : Velocity
    }
};


#define mHorizon3DBinID_1 BinID(426,800)
#define mHorizon3DBinID_2 BinID(426,900)
//Horizon3D : Output Values
static const float zvalues_hor3d[7][8][2] =
{
    { //F3_Test_Survey
	{ 566.503967f, 551.361450f }, //T2D : tablet2did_m
	{ 566.503967f, 551.361450f }, //T2D : tablet2did_ft
	{ 856.708191f, 833.693664f }, //T2D : Linear
	{ 562.842895f, 549.798157f }, //T2D : Velocity
	{ 0.5730112f, 0.5580959f }, //D2T : tablet2did_m
	{ 0.5730112f, 0.5580959f }, //D2T : tablet2did_ft
	{ 0.3841169f, 0.3742115f }, //D2T : Linear
	{ 0.5924090f, 0.5785800f }  //D2T : Velocity
    },
    { //F3_Test_Survey_DepthFT
	{ 1858.6088156f, 1808.9286417f }, //T2D : tablet2did_m
	{ 1858.6088156f, 1808.9286417f }, //T2D : tablet2did_ft
	{ 2810.7224114f, 2735.2154330f }, //T2D : Linear
	{ 1848.4530029f, 1805.2114258f }, //T2D : Velocity
	{ 0.5680090f, 0.5543861f }, //D2T : tablet2did_m
	{ 0.5680090f, 0.5543861f }, //D2T : tablet2did_ft
	{ 0.3807954f, 0.3717469f }, //D2T : Linear
	{ 0.5815666f, 0.5700924f }  //D2T : Velocity
    },
    { //F3_Test_Survey_DisplayFT
	{ 1858.6087646f, 1808.9285889f }, //T2D : tablet2did_m
	{ 1858.6087646f, 1808.9285889f }, //T2D : tablet2did_ft
	{ 2810.7224121f, 2735.2153320f }, //T2D : Linear
	{ 1846.1667480f, 1803.4663086f }, //T2D : Velocity
	{ 0.5680090f, 0.5543861f }, //D2T : tablet2did_m
	{ 0.5680090f, 0.5543861f }, //D2T : tablet2did_ft
	{ 0.3807954f, 0.3717469f }, //D2T : Linear
	{ 0.5852728f, 0.5729352f }  //D2T : Velocity
    },
    { //F3_Test_Survey_XYinft
	{ 1858.6087646f, 1808.9285889f }, //T2D : tablet2did_m
	{ 1858.6087646f, 1808.9285889f }, //T2D : tablet2did_ft
	{ 2810.7224121f, 2735.2153320f }, //T2D : Linear
	{ 1846.1667480f, 1803.4663086f }, //T2D : Velocity
	{ 0.1850900f, 0.1805435f }, //D2T : tablet2did_m
	{ 0.1850900f, 0.1805435f }, //D2T : tablet2did_ft
	{ 0.1248829f, 0.1218248f }, //D2T : Linear
	{ 0.1901860f, 0.1860567f }  //D2T : Velocity
    },
    { //F3_Test_Survey_DepthM
	{ 566.503967f, 551.36145f }, //T2D : tablet2did_m
	{ 566.503967f, 551.36145f }, //T2D : tablet2did_ft
	{ 856.708191f, 833.693664f }, //T2D : Linear
	{ 563.5928955f, 550.3659058f }, //T2D : Velocity
	{ 0.5680090f, 0.5543861f }, //D2T : tablet2did_m
	{ 0.5680090f, 0.5543861f }, //D2T : tablet2did_ft
	{ 0.3807954f, 0.3717469f }, //D2T : Linear
	{ 0.5815666f, 0.5700924f }  //D2T : Velocity
    },
    { //F3_Test_Survey_DepthM_XYinft
	{ 566.503967f, 551.36145f }, //T2D : tablet2did_m
	{ 566.503967f, 551.36145f }, //T2D : tablet2did_ft
	{ 856.708191f, 833.693664f }, //T2D : Linear
	{ 563.5928955f, 550.3659058f }, //T2D : Velocity
	{ 0.5680090f, 0.5543861f }, //D2T : tablet2did_m
	{ 0.5680090f, 0.5543861f }, //D2T : tablet2did_ft
	{ 0.3807954f, 0.3717469f }, //D2T : Linear
	{ 0.5815666f, 0.5700924f }  //D2T : Velocity
    },
    { //F3_Test_Survey_DepthFT__XYinft_
	{ 1858.6088156f, 1808.9286417f }, //T2D : tablet2did_m
	{ 1858.6088156f, 1808.9286417f }, //T2D : tablet2did_ft
	{ 2810.7224114f, 2735.2154330f }, //T2D : Linear
	{ 1848.4530029f, 1805.2114258f }, //T2D : Velocity
	{ 0.5680090f, 0.5543861f }, //D2T : tablet2did_m
	{ 0.5680090f, 0.5543861f }, //D2T : tablet2did_ft
	{ 0.3807954f, 0.3717469f }, //D2T : Linear
	{ 0.5815666f, 0.5700924f }  //D2T : Velocity
    }
};


static MultiID getVelID( const int* grps, int idx )
{
    MultiID ret;
    ret.setGroupID( 100010 ).setObjectID( grps[idx] );
    return ret;
}

bool hasSurfaceIOData( EM::ObjectType objtype )
{
    return objtype == EM::ObjectType::Hor3D || objtype == EM::ObjectType::Hor2D
	|| objtype == EM::ObjectType::Flt3D
	|| objtype == EM::ObjectType::FltSS2D
	|| objtype == EM::ObjectType::FltSS3D;
}


static BufferStringSet& survDirNames()
{
    static BufferStringSet ret;
    if ( ret.isEmpty() )
    {
	ret.add( "F3_Test_Survey" )
	    .add( "F3_Test_Survey_DepthFT")
	    .add("F3_Test_Survey_DisplayFT")
	    .add("F3_Test_Survey_XYinft")
	    .add("F3_Test_Survey_DepthM")
	    .add("F3_Test_Survey_DepthM_XYinft")
	    .add("F3_Test_Survey_DepthFT__XYinft_");
    }

    return ret;
}


TypeSet<OD::Pair<MultiID,EM::ObjectType>> getNativeInputData()
{
    TypeSet<OD::Pair<MultiID,EM::ObjectType>> inpdataset;
    const int grpid = IOObjContext::getStdDirData(
	IOObjContext::Surf )->groupID();
    inpdataset.add( OD::Pair(MultiID(grpid,2),EM::ObjectType::Hor3D) );
    inpdataset.add( OD::Pair(MultiID(grpid,9),EM::ObjectType::Flt3D) );
    return inpdataset;
}


TypeSet<OD::Pair<MultiID,EM::ObjectType>> getTranformedInputData()
{
    TypeSet<OD::Pair<MultiID,EM::ObjectType>> inpdataset;
    const int grpid = IOObjContext::getStdDirData(
	IOObjContext::Surf )->groupID();
    inpdataset.add( OD::Pair(MultiID(grpid,10),EM::ObjectType::Hor3D) );
    inpdataset.add( OD::Pair(MultiID(grpid,11),EM::ObjectType::Flt3D) );
    return inpdataset;
}


#define mMsg(txt) BufferString( SI().name(), ": ", txt )

static RefObjectSet<ZAxisTransform> get3DTransforms( int survidx )
{
    RefObjectSet<ZAxisTransform> zatfs;
    //Time to Depth
    zatfs.add( new SimpleT2DTransform(tablet2did_m) );
    zatfs.add( new SimpleT2DTransform(tablet2did_ft) );
    zatfs.add( new LinearT2DTransform(v0(), k));
    zatfs.add( new Time2DepthStretcher(getVelID(velids_direct,survidx)) );

    //Depth to Time
    zatfs.add( new SimpleD2TTransform(tablet2did_m) );
    zatfs.add( new SimpleD2TTransform(tablet2did_ft) );
    zatfs.add( new LinearD2TTransform(v0(), k));
    zatfs.add( new Depth2TimeStretcher(getVelID(velids_direct,survidx)) );

    return zatfs;
}


bool testHorizon3DOutput( RefMan<EM::EMObject> emobj, int survidx,
								int zatfidx )
{
    mDynamicCastGet(EM::Horizon3D*,hor,emobj.ptr());
    if ( !hor )
	return false;

    const float z1 = hor->getPos(mHorizon3DBinID_1.toInt64()).z_;
    const float z2 = hor->getPos(mHorizon3DBinID_2.toInt64()).z_;
    const float* zvals = zvalues_hor3d[survidx][zatfidx];

    mRunStandardTest( (mIsEqual(z1,zvals[0],mEps) &&
	    mIsEqual(z2,zvals[1],mEps)),mMsg("Horizon 3D position testing") );

    return true;
}


bool testFltOutput( RefMan<EM::EMObject> emobj, int survidx, int zatfidx )
{
    mDynamicCastGet(EM::Fault3D*,flt,emobj.ptr());
    if ( !flt )
	return false;

    const EM::Fault3DGeometry& geometry = flt->geometry();
    const int nrsticks = geometry.nrSticks();
    mRunStandardTest( nrsticks == 2, mMsg("Number of sticks") );

    const Geometry::FaultStickSurface* fltsurf = geometry.geometryElement();
    mRunStandardTest( fltsurf, mMsg("Fault surface created"));

    const Geometry::FaultStick* stick = fltsurf->getStick( 0 );
    mRunStandardTest( !stick->locs_.isEmpty(), mMsg("Stick has data") );
    const Coord3 firstcrd = stick->getCoordAtIndex( 0 );
    const Coord3 lastcrd = stick->getCoordAtIndex( stick->size()-1 );

    const float* zvals = zvalues_flt[survidx][zatfidx];
    mRunStandardTest( (mIsEqual(firstcrd.z_,zvals[0],mEps) &&
		       mIsEqual(lastcrd.z_,zvals[1],mEps)),
		      mMsg("Fault 3D position testing") );

    return true;
}


bool testFltSetOutput( RefMan<EM::EMObject> emobj, int zatfidx )
{
    //TODO
    return true;
}


bool handleEarthModelObjects( ZAxisTransform& zatf, int survidx, int zatfidx )
{
    TypeSet<OD::Pair<MultiID,EM::ObjectType>> inpdataset =
	(zatf.fromZDomainInfo().def_ == SI().zDomain() ) ?
			    getNativeInputData() : getTranformedInputData();

    const int grpid = IOObjContext::getStdDirData(
	IOObjContext::Surf )->groupID();
    const MultiID outmid( grpid, MultiID::cMemoryObjID() );
    for ( const auto& inpdata : inpdataset )
    {
	const EM::ObjectType objtype = inpdata.second();
	EM::EMManager& em = EM::EMM();
	EM::SurfaceIOData sd;
	uiString errmsg;
	const MultiID inpmid = inpdata.first();
	if ( hasSurfaceIOData(objtype) &&
				    !em.getSurfaceData(inpmid,sd,errmsg) )
	    mErrRet(errmsg)

	auto* data = new EM::SurfaceT2DTransfData( sd );
	data->inpmid_ = inpmid;
	data->outmid_ = outmid;
	data->surfsel_.rg = sd.rg;
	if ( SI().sampling(true) != SI().sampling(false) )
	{
	    if ( data->surfsel_.rg.isEmpty() )
		data->surfsel_.rg.init( true );

	    data->surfsel_.rg.limitTo( SI().sampling(true).hsamp_ );
	}

	ObjectSet<EM::SurfaceT2DTransfData> datas;
	datas.add( data );

	PtrMan<Executor> exec =
	EM::SurfaceT2DTransformer::createExecutor( datas, zatf, objtype );
	mDynamicCastGet(EM::SurfaceT2DTransformer*,surftrans,exec.ptr());
	if ( surftrans->execute() )
	{
	    switch(objtype)
	    {
		case EM::ObjectType::Hor3D:
		case EM::ObjectType::AnyHor:
		    if ( !testHorizon3DOutput(
			    surftrans->getTransformedSurface(data->outmid_),
			    survidx,zatfidx) )
			return false;

		    break;
		case EM::ObjectType::Flt3D:
		    if ( !testFltOutput(
			    surftrans->getTransformedSurface(data->outmid_),
			    survidx,zatfidx) )
			return false;

		    break;
		case EM::ObjectType::FltSet:
		    if ( !testFltSetOutput(
			    surftrans->getTransformedSurface(data->outmid_),
			    zatfidx) )
			return false;

		    break;
		case EM::ObjectType::Hor2D:
		case EM::ObjectType::FltSS2D:
		case EM::ObjectType::FltSS2D3D:
		case EM::ObjectType::FltSS3D:
		case EM::ObjectType::Body:
		case EM::ObjectType::Unknown:
		    break;
	    }
	}
	else
	{
	    tstStream() << zatf.name() << " for " <<
		    EM::ObjectTypeDef().getKey( objtype ) << "\n";
	}
    }

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProgDR();

    OD::ModDeps().ensureLoaded( "WellAttrib" );
    const BufferStringSet& survdirnms = survDirNames();

    clParser().setKeyHasValue( "datadir" );
    BufferString basedir;
    clParser().getVal( "datadir", basedir );
    const uiRetVal uirv = IOMan::setDataSource( basedir.buf(),
						survdirnms.first()->str() );
    mRunStandardTestWithError( uirv.isOK(), "Initialize the first project",
			       toString(uirv) );

    SurveyDiskLocation sdl( IOM().rootDir() );
    for ( int survidx=0; survidx<survdirnms.size(); survidx++ )
    {
	const BufferString& survdirnm = survdirnms.get( survidx );
	sdl.setDirName( survdirnm.buf() );
	SurveyChanger changer( sdl );

	const float srd =
	  UnitOfMeasure::surveyDefSRDStorageUnit()->getUserValueFromSI( srd_ );
	eSI().setSeismicReferenceDatum( srd );
	RefObjectSet<ZAxisTransform> zatfs = get3DTransforms( survidx );
	for ( int zatfidx=0; zatfidx<zatfs.size(); zatfidx++ )
	{
	    ZAxisTransform* zatf = zatfs.get( zatfidx );
	    logStream() << "Testing " << survdirnm << " for " <<
		zatf->factoryDisplayName() << " transformation" << od_endl;
	    if ( !handleEarthModelObjects(*zatf,survidx,zatfidx) )
		return 1;

	    tstStream() << od_endl;
	}
    }

    return 0;
}
