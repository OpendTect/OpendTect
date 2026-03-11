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


//Velocity Object IDs
static const int velids_direct[] =   {	8,  9,	9, 14, 14, 16, 16 };
static const int velids_reverse1[] = { 14, 16, 16,  8,	8,  9,	9 };
static const int velids_reverse2[] = { 16, 14, 14,  9,	9,  8,	8 };

//Simple Time-Depth Parameters
static const MultiID tablet2did_m = MultiID( 100090, 6 );
static const MultiID tablet2did_ft = MultiID( 100090, 7 );

//Linear Vel Paramenters
static double v0()
{
    return LinearVelTransform::velUnit()->getUserValueFromSI( 3000. );
}

static const double k = 0.1;

//SRD Paramter
static double srd_ = 15.24; // 50ft


//Model Info : [Nr. Surveys][Nr. ZAxistransforms][Nr. test Vals]

#define mHorizon3DBinID_1 BinID(426,800)
#define mHorizon3DBinID_2 BinID(426,900)
//Horizon3D : Output Values (SI units)
static const float zvalues_hor3d[12][2] =
{
    {  958.38483f,  937.085945f },  //T2D : tablet2did_m
    {  958.38483f,  937.085945f },  //T2D : tablet2did_ft
    { 1458.3209f,  1425.32379f	},  //T2D : Linear
    {  980.3473f,   959.003687f },  //T2D : Velocity (Model: Time)
    {  980.3473f,   959.003687f },  //T2D : Velocity (Model: Depth)
    {  980.3473f,   959.003687f },  //T2D : Velocity (Model: Depth)
    { 0.981122051f, 0.960130379f }, //D2T : tablet2did_m
    { 0.981122051f, 0.960130379f }, //D2T : tablet2did_ft
    { 0.653263919f, 0.639507739f }, //D2T : Linear
    { 0.959009528f, 0.938030362f }, //D2T : Velocity (Model: Depth)
    { 0.959009528f, 0.938030362f }, //D2T : Velocity (Model: Time)
    { 0.959009528f, 0.938030362f }  //D2T : Velocity (Model: Time)
};

//Fault : Output Values
static const float zvalues_flt[12][2] =
{
    { 475.230133f, 1043.864161f }, //T2D : tablet2did_m
    { 475.230133f, 1043.864161f }, //T2D : tablet2did_ft
    { 718.244495f, 1557.496705f }, //T2D : Linear
    { 458.2054225f, 1055.745435f }, //T2D : Velocity
    { 458.2054225f, 1055.745435f }, //T2D : Velocity
    { 458.2054225f, 1055.745435f }, //T2D : Velocity
    { 0.466338386f, 1.027888367f },  //D2T : tablet2did_m
    { 0.466338386f, 1.027888367f },  //D2T : tablet2did_ft
    { 0.313165591f, 0.701561303f },  //D2T : Linear
    { 0.483107534f, 1.021958163f },  //D2T : Velocity
    { 0.483107534f, 1.021958163f },  //D2T : Velocity
    { 0.483107534f, 1.021958163f }   //D2T : Velocity
};


static MultiID getVelID( const int* grps, int idx )
{
    MultiID ret;
    ret.setGroupID( 100010 ).setObjectID( grps[idx] );
    return ret;
}

bool hasSurfaceIOData( EM::ObjectType objtype )
{
    return objtype == EM::ObjectType::Hor3D ||
	   objtype == EM::ObjectType::Hor2D ||
	   objtype == EM::ObjectType::Flt3D ||
	   objtype == EM::ObjectType::FltSS2D ||
	   objtype == EM::ObjectType::FltSS3D;
}


static const BufferStringSet& survDirNames()
{
    static BufferStringSet ret;
    if ( ret.isEmpty() )
    {
	ret.add( "F3_Test_Survey" )
	   .add( "F3_Test_Survey_DisplayFT" )
	   .add( "F3_Test_Survey_XYinft" )
	   .add( "F3_Test_Survey_DepthM" )
	   .add( "F3_Test_Survey_DepthM_XYinft" )
	   .add( "F3_Test_Survey_DepthFT" )
	   .add( "F3_Test_Survey_DepthFT__XYinft_" );
    }

    return ret;
}


static const TypeSet<OD::Pair<MultiID,EM::ObjectType> >& getNativeInputData()
{
    static TypeSet<OD::Pair<MultiID,EM::ObjectType>> inpdataset;
    inpdataset.setEmpty();
    const int grpid = IOObjContext::getStdDirData(
					    IOObjContext::Surf )->groupID();
    if ( SI().zIsTime() )
	inpdataset.add( OD::Pair(MultiID(grpid,3),EM::ObjectType::Hor3D) );
    else
    {
	// Depth data for SRD=50ft
	inpdataset.add( OD::Pair(MultiID(grpid,11),EM::ObjectType::Hor3D) );
    }

    inpdataset.add( OD::Pair(MultiID(grpid,9),EM::ObjectType::Flt3D) );

    return inpdataset;
}


static const TypeSet<OD::Pair<MultiID,EM::ObjectType>>& getTranformedInputData()
{
    static TypeSet<OD::Pair<MultiID,EM::ObjectType>> inpdataset;
    if ( inpdataset.isEmpty() )
    {
	const int grpid = IOObjContext::getStdDirData(
					    IOObjContext::Surf )->groupID();
	inpdataset.add( OD::Pair(MultiID(grpid,13),EM::ObjectType::Hor3D) );
	inpdataset.add( OD::Pair(MultiID(grpid,14),EM::ObjectType::Flt3D) );
    }

    return inpdataset;
}


#define mMsg(txt) BufferString( SI().name(), ": ", txt )

static RefObjectSet<ZAxisTransform>& get3DTransforms( int survidx )
{
    static RefObjectSet<ZAxisTransform> zatfs;
    zatfs.setEmpty();
    //Time to Depth
    zatfs.add( new SimpleT2DTransform(tablet2did_m) );
    zatfs.add( new SimpleT2DTransform(tablet2did_ft) );
    zatfs.add( new LinearT2DTransform(v0(), k));
    zatfs.add( new Time2DepthStretcher(getVelID(velids_direct,survidx)) );
    zatfs.add( new Time2DepthStretcher(getVelID(velids_reverse1,survidx)) );
    zatfs.add( new Time2DepthStretcher(getVelID(velids_reverse2,survidx)) );

    //Depth to Time
    zatfs.add( new SimpleD2TTransform(tablet2did_m) );
    zatfs.add( new SimpleD2TTransform(tablet2did_ft) );
    zatfs.add( new LinearD2TTransform(v0(), k));
    zatfs.add( new Depth2TimeStretcher(getVelID(velids_direct,survidx)) );
    zatfs.add( new Depth2TimeStretcher(getVelID(velids_reverse1,survidx)) );
    zatfs.add( new Depth2TimeStretcher(getVelID(velids_reverse2,survidx)) );

    return zatfs;
}


bool testHorizon3DOutput( const EM::EMObject* emobj, int survidx, int zatfidx )
{
    mDynamicCastGet(const EM::Horizon3D*,hor,emobj);
    mRunStandardTest( hor, mMsg("Horizon 3D has been processed") )

    const float z1 = hor->getZ( mHorizon3DBinID_1 );
    mRunStandardTest( !mIsUdf(z1),mMsg("Horizon 3D position 1 is defined") );
    const float z2 = hor->getZ( mHorizon3DBinID_2 );
    mRunStandardTest( !mIsUdf(z2),mMsg("Horizon 3D position 2 is defined") );
    const float* zvals = zvalues_hor3d[zatfidx];
    float zval1 = zvals[0];
    float zval2 = zvals[1];
    if ( emobj->zDomain().isDepthFeet() )
    {
	zval1 *= mToFeetFactorF;
	zval2 *= mToFeetFactorF;
    }

    BufferString readvalstr( "Z-Values read: [" );
    readvalstr.add(toStringPrecise(z1)).add(',')
	      .add(toStringPrecise(z2)).add("]");
    BufferString expvalstr( "Expected Z-Values: [" );
    expvalstr.add(toStringPrecise(zval1)).add(',')
	     .add(toStringPrecise(zval2)).add("]; ");
    const BufferString errmsg( expvalstr.buf(), readvalstr.buf() );

    const float eps = hor->zDomain().isTime() ? 1e-3f : 1e1f;
    mRunStandardTestWithError( (mIsEqual(z1,zval1,eps) &&
				mIsEqual(z2,zval2,eps)),
			       mMsg("Horizon 3D position testing"), errmsg  );

    return true;
}


bool testFltOutput( const EM::EMObject* emobj, int survidx, int zatfidx )
{
    mDynamicCastGet(const EM::Fault3D*,flt,emobj);
    mRunStandardTest( flt, mMsg("Fault 3D has been processed") )

    const EM::Fault3DGeometry& geometry = flt->geometry();
    const int nrsticks = geometry.nrSticks();
    const BufferString expnrstick( "Expected: ", 2 );
    const BufferString nrstickstr( "Actual: ", nrsticks );
    BufferString expnrstickerrmsg( expnrstick, " ; ", nrstickstr );
    mRunStandardTestWithError( nrsticks == 2,
			       mMsg("Number of sticks"), expnrstickerrmsg );

    const Geometry::FaultStickSurface* fltsurf = geometry.geometryElement();
    mRunStandardTest( fltsurf, mMsg("Fault surface created"));

    const Geometry::FaultStick* stick = fltsurf->getStick( 0 );
    mRunStandardTest( !stick->locs_.isEmpty(), mMsg("Stick has data") );
    const Coord3 firstcrd = stick->getCoordAtIndex( 0 );
    mRunStandardTest( !firstcrd.isUdf(),mMsg("Fault 3D position is defined") );
    const Coord3 lastcrd = stick->getCoordAtIndex( stick->size()-1 );
    mRunStandardTest( !lastcrd.isUdf(),mMsg("Fault 3D position is defined") );

    const float* zvals = zvalues_flt[zatfidx];
    float zval1 = zvals[0];
    float zval2 = zvals[1];
    if ( emobj->zDomain().isDepthFeet() )
    {
	zval1 *= mToFeetFactorF;
	zval2 *= mToFeetFactorF;
    }

    const float firstz = mCast(float,firstcrd.z_);
    const float secondz = mCast(float,lastcrd.z_);
    BufferString readvalstr( "Z-Values read: [" );
    readvalstr.add(toStringPrecise(firstz)).add(',')
	      .add(toStringPrecise(secondz)).add("]");
    BufferString expvalstr( "Expected Z-Values: [" );
    expvalstr.add(toStringPrecise(zval1)).add(',')
	     .add(toStringPrecise(zval2)).add("]; ");
    const BufferString errmsg( expvalstr.buf(), readvalstr.buf() );

    const float eps = flt->zDomain().isTime() ? 1e-4f : 1e-0f;
    mRunStandardTestWithError( (mIsEqual(firstz,zval1,eps) &&
				mIsEqual(secondz,zval2,eps)),
			       mMsg("Fault 3D position testing"), errmsg );

    return true;
}


bool testFltSetOutput( const EM::EMObject* emobj, int survidx, int zatfidx )
{
    //TODO
    return true;
}


static bool handleEarthModelObjects( ZAxisTransform& zatf,
				     int survidx, int zatfidx )
{
    const TypeSet<OD::Pair<MultiID,EM::ObjectType> >& inpdataset =
	(zatf.fromZDomainInfo().def_ == SI().zDomain() ) ?
			    getNativeInputData() : getTranformedInputData();

    const int grpid = IOObjContext::getStdDirData(
						IOObjContext::Surf )->groupID();
    const MultiID outmid( grpid, MultiID::cMemoryObjID() );
    for ( const auto& inpdata : inpdataset )
    {
	const EM::ObjectType objtype = inpdata.second();
	const BufferString objtypestr = EM::toString( objtype );
	EM::EMManager& em = EM::EMM();
	EM::SurfaceIOData sd;
	uiString errmsg;
	const MultiID inpmid = inpdata.first();
	if( hasSurfaceIOData(objtype) )
	{
	    const BufferString msg( "Read ", objtypestr, " data" );
	    mRunStandardTestWithError( em.getSurfaceData(inpmid,sd,errmsg),
				mMsg(msg.str()), errmsg.getString() )
	}

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

	ManagedObjectSet<EM::SurfaceT2DTransfData> datas;
	datas.add( data );

	PtrMan<Executor> exec =
	EM::SurfaceT2DTransformer::createExecutor( datas, zatf, objtype );
	mDynamicCastGet(EM::SurfaceT2DTransformer*,surftrans,exec.ptr());
	const BufferString msg( "Run ", objtypestr, " transformation" );
	mRunStandardTestWithError( surftrans->execute(),
				mMsg(msg.str()), surftrans->errMsg().getText() )

	ConstRefMan<EM::EMObject> emobj =
			surftrans->getTransformedSurface( data->outmid_ );
	switch( objtype )
	{
	    case EM::ObjectType::Hor3D:
	    case EM::ObjectType::AnyHor:
		if ( !testHorizon3DOutput(emobj.ptr(),survidx,zatfidx) )
		    return false;

		break;
	    case EM::ObjectType::Flt3D:
		if ( !testFltOutput(emobj.ptr(),survidx,zatfidx) )
		    return false;

		break;
	    case EM::ObjectType::FltSet:
		if ( !testFltSetOutput(emobj.ptr(),survidx,zatfidx) )
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
	ObjectSet<ZAxisTransform>& zatfs = get3DTransforms( survidx );
	for ( int zatfidx=0; zatfidx<zatfs.size(); zatfidx++ )
	{
	    tstStream() << od_endl;
	    RefMan<ZAxisTransform> zatf = zatfs.get( zatfidx );
	    logStream() << "Testing " << survdirnm << " for " <<
		zatf->factoryDisplayName() << " transformation" << od_endl;
	    if ( !handleEarthModelObjects(*zatf,survidx,zatfidx) )
	    {
		zatfs.erase();
		return 1;
	    }
	}
    }

    return 0;
}
