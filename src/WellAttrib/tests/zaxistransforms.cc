/*+
________________________________________________________________________

Copyright:	(C) 1995-2022 dGB Beheer B.V.
License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emfault3d.h"
#include "emfaultset3d.h"
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


//3D Velocity Object IDs
static const int velids_direct[] =   {	8,  9,	9, 14, 14, 16, 16 };
static const int velids_reverse1[] = { 14, 16, 16,  8,	8,  9,	9 };
static const int velids_reverse2[] = { 16, 14, 14,  9,	9,  8,	8 };

//2D Velocity Object IDs
static const int velids2d_direct[] = { 121, 122, 122, 123, 123, 126, 126 };
static const int velids2d_reverse1[] = { 123, 126, 126, 121, 121, 122, 122 };
static const int velids2d_reverse2[] = { 126, 123, 123, 122, 122, 121, 121 };

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

#define mHorizon2DIdxPair1 Pos::IdxPair( 8, 622 ), true
//Horizon2D : Output Values (SI units)
static const float zvalues_hor2d[12][1] =
{
    { 2071.964f },  //T2D : tablet2did_m
    { 2071.964f },  //T2D : tablet2did_ft
    { 2379.512f },  //T2D : Linear
    { 2377.433f },  //T2D : Velocity (Model: Time)
    { 2377.433f },  //T2D : Velocity (Model: Depth)
    { 2377.433f },  //T2D : Velocity (Model: Depth)
    { 1.688717f },  //D2T : tablet2did_m
    { 1.688717f },  //D2T : tablet2did_ft
    { 1.534697f },  //D2T : Linear
    { 1.535982f },  //D2T : Velocity (Model: Depth)
    { 1.535982f },  //D2T : Velocity (Model: Time)
    { 1.535982f }  //D2T : Velocity (Model: Time)
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


static const TypeSet<OD::Pair<MultiID,EM::ObjectType> >&
				getNativeInputData( bool is2d )
{
    static TypeSet<OD::Pair<MultiID, EM::ObjectType>> inpdataset;
    inpdataset.setEmpty();
    const int grpid = IOObjContext::getStdDirData(
		      IOObjContext::Surf )->groupID();

    if ( !is2d )
    {
	if ( SI().zIsTime() )
	    inpdataset.add( OD::Pair(MultiID(grpid, 3),
			    EM::ObjectType::Hor3D) );
	else
	{
	    // Depth data for SRD=50ft
	    inpdataset.add( OD::Pair(MultiID(grpid, 11),
			    EM::ObjectType::Hor3D) );
	}

	inpdataset.add( OD::Pair(MultiID(grpid, 9), EM::ObjectType::Flt3D) );
	inpdataset.add( OD::Pair(MultiID(grpid, 16), EM::ObjectType::FltSet) );
    }
    else
	inpdataset.add( OD::Pair(MultiID(grpid, 82), EM::ObjectType::Hor2D) );

    return inpdataset;
}


static const TypeSet<OD::Pair<MultiID,EM::ObjectType>>&
				getTranformedInputData( bool is2d )
{
    static TypeSet<OD::Pair<MultiID,EM::ObjectType>> inpdataset;
    inpdataset.setEmpty();
    const int grpid = IOObjContext::getStdDirData(
		      IOObjContext::Surf )->groupID();
    if ( !is2d )
    {
	inpdataset.add( OD::Pair(MultiID(grpid, 13), EM::ObjectType::Hor3D) );
	inpdataset.add( OD::Pair(MultiID(grpid, 14), EM::ObjectType::Flt3D) );
	inpdataset.add( OD::Pair(MultiID(grpid, 17), EM::ObjectType::FltSet) );
    }
    else
	inpdataset.add( OD::Pair( MultiID(grpid, 219), EM::ObjectType::Hor2D) );

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


static RefObjectSet<ZAxisTransform>& get2DTransforms( int survidx )
{
    static RefObjectSet<ZAxisTransform> zatfs;
    zatfs.setEmpty();
    //Time to Depth
    zatfs.add( new SimpleT2DTransform(tablet2did_m) );
    zatfs.add( new SimpleT2DTransform(tablet2did_ft) );
    zatfs.add( new LinearT2DTransform(v0(), k));
    zatfs.add( new Time2DepthStretcher( getVelID(velids2d_direct,survidx)) );
    zatfs.add( new Time2DepthStretcher( getVelID(velids2d_reverse1,survidx)) );
    zatfs.add( new Time2DepthStretcher( getVelID(velids2d_reverse2,survidx)) );

    //Depth to Time
    zatfs.add( new SimpleD2TTransform(tablet2did_m) );
    zatfs.add( new SimpleD2TTransform(tablet2did_ft) );
    zatfs.add( new LinearD2TTransform(v0(), k));
    zatfs.add( new Depth2TimeStretcher( getVelID(velids2d_direct,survidx)) );
    zatfs.add( new Depth2TimeStretcher( getVelID(velids2d_reverse1,survidx)) );
    zatfs.add( new Depth2TimeStretcher( getVelID(velids2d_reverse2,survidx)) );

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


bool testHorizon2DOutput( const EM::EMObject* emobj, int survidx, int zatfidx )
{
    mDynamicCastGet( const EM::Horizon2D*, hor, emobj );
    mRunStandardTest( hor, mMsg("Horizon 2D has been processed") );


    const TrcKey trckey( mHorizon2DIdxPair1 );
    float z = hor->getZ( trckey );
    const float* zvals = zvalues_hor2d[zatfidx];
    float zval = zvals[0];
    if ( emobj->zDomain().isDepthFeet() )
	zval *= mToFeetFactorF;

    BufferString readvalstr( "Z-Values read: [" );
    readvalstr.add(toStringPrecise(z)).add(']');
    BufferString expvalstr( "Expected Z-Values: [" );
    expvalstr.add(toStringPrecise(zval)).add(']');
    const BufferString errmsg( expvalstr.buf(), readvalstr.buf() );

    const float eps = hor->zDomain().isTime() ? 1e-3f : 1e1f;
    mRunStandardTestWithError( (mIsEqual(z,zval,eps)),
			       mMsg("Horizon 2D position testing"), errmsg  );

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
    mDynamicCastGet( const EM::FaultSet3D* , fltset, emobj );
    mRunStandardTest( fltset, mMsg("FaultSet3D has been processed") )

    const int nrflts = fltset->nrFaults();
    mRunStandardTest( nrflts==1, mMsg("FaultSet3D has single member Fault3D") )

    const EM::FaultID fid = fltset->getFaultID( 0 );
    const EM::Fault3D* flt = fltset->getFault3D( fid ).ptr();
    mRunStandardTest( flt, mMsg("Member Fault3D has been retreived") )

    const EM::Fault3DGeometry& geometry = flt->geometry();
    const int nrsticks = geometry.nrSticks();
    const BufferString expnrstick( "Expected: ", 2 );
    const BufferString nrstickstr( "Actual: ", nrsticks );
    BufferString expnrstickerrmsg( expnrstick, " ; ", nrstickstr );
    mRunStandardTestWithError( nrsticks == 2,
			       mMsg("Number of member's sticks"),
			       expnrstickerrmsg );

    const Geometry::FaultStickSurface* fltsurf = geometry.geometryElement();
    mRunStandardTest( fltsurf, mMsg("Member Fault surface created"));

    const Geometry::FaultStick* stick = fltsurf->getStick( 0 );
    mRunStandardTest( !stick->locs_.isEmpty(),
		      mMsg("Member's Stick has data") );

    const Coord3 firstcrd = stick->getCoordAtIndex( 0 );
    mRunStandardTest( !firstcrd.isUdf(),
		      mMsg("Member Fault 3D position is defined") );
    const Coord3 lastcrd = stick->getCoordAtIndex( stick->size()-1 );
    mRunStandardTest( !lastcrd.isUdf(),
		      mMsg("Member Fault 3D position is defined") );

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

    const float eps = emobj->zDomain().isTime() ? 1e-4f : 1e-0f;
    mRunStandardTestWithError( (mIsEqual(firstz,zval1,eps) &&
				mIsEqual(secondz,zval2,eps)),
				mMsg("Member Fault 3D position testing"),
				errmsg );

    return true;
}


static bool handleEarthModelObjects( ZAxisTransform& zatf,
				     int survidx, int zatfidx, bool is2d=false )
{
    const TypeSet<OD::Pair<MultiID,EM::ObjectType> >& inpdataset =
			    (zatf.fromZDomainInfo().def_ == SI().zDomain() ) ?
			    getNativeInputData( is2d ) :
			    getTranformedInputData( is2d );

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

	ObjectSet<EM::SurfaceT2DTransfData> datas;
	datas.add( data );

	PtrMan<Task> exec =
	EM::SurfaceT2DTransformer::createExecutor( datas, zatf, objtype );
	const BufferString msg( "Run ", objtypestr, " transformation" );
	mRunStandardTestWithError( exec->execute(),
				mMsg(msg.str()), exec->uiMessage().getString() )

	mDynamicCastGet(EM::SurfaceT2DTransformer*,surftrans,exec.ptr());
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
		if ( !testHorizon2DOutput(emobj.ptr(),survidx,zatfidx) )
		    return false;
		break;

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

	//3D EMObjects
	{
	    ObjectSet<ZAxisTransform>& zatfs = get3DTransforms( survidx );
	    for ( int zatfidx = 0; zatfidx < zatfs.size(); zatfidx++ )
	    {
		tstStream() << od_endl;
		RefMan<ZAxisTransform> zatf = zatfs.get( zatfidx );
		logStream() << "Testing " << survdirnm << " for " <<
					zatf->factoryDisplayName() <<
					" transformation on 3D emobjs" <<
					od_endl;
		if ( !handleEarthModelObjects(*zatf, survidx, zatfidx) )
		{
		    zatfs.erase();
		    return 1;
		}
	    }
	}

	//2D EMObjects
	{
	    ObjectSet<ZAxisTransform>& zatfs = get2DTransforms( survidx );
	    for ( int zatfidx = 0; zatfidx < zatfs.size(); zatfidx++ )
	    {
		tstStream() << od_endl;
		RefMan<ZAxisTransform> zatf = zatfs.get( zatfidx );
		logStream() << "Testing " << survdirnm << " for " <<
					zatf->factoryDisplayName() <<
					" transformation on 2D emobjs" <<
					od_endl;
		if ( !handleEarthModelObjects(*zatf, survidx, zatfidx, true) )
		{
		    zatfs.erase();
		    return 1;
		}
	    }

	}
    }

    return 0;
}
