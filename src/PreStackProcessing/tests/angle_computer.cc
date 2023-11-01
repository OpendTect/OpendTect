/*+
________________________________________________________________________

 Copyright:	(C) 1995-2023 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"
#include "testprog.h"

#include "ioman.h"
#include "ioobj.h"
#include "moddepmgr.h"
#include "multiid.h"
#include "prestackanglecomputer.h"
#include "prestackanglemutecomputer.h"
#include "prestackgather.h"
#include "survinfo.h"
#include "unitofmeasure.h"

/*
   The test program is based on the following synthetic model:
   Layer 1: H=561.5498m, Vp = 1960m/s
   Layer 2: H=54.81m,	 Vp = 2175m/s
   Layer 3: H=379.228m,  Vp = 2260m/s
   Layer 4: H=169.188m,  Vp = 2400m/s
   The interfaces thus cross the interpretations FS8, FS7, FS4 at respectively:
   TWT=573.0111, 623.405933 and 959.0103626ms for BinID 426/800
   That model is digitized in the various test surveys exactly for that BinID,
   for a seismic reference datum of 50ft.

*/

static double srd_ = 15.24; // 50ft

static BufferStringSet& survDirNames()
{
    static BufferStringSet ret;
    if ( ret.isEmpty() )
    {
	ret.add( "F3_Test_Survey" )
	 .add( "F3_Test_Survey_DisplayFT" ).add( "F3_Test_Survey_XYinft" )
	 .add( "F3_Test_Survey_DepthM" ).add( "F3_Test_Survey_DepthM_XYinft" )
	 .add( "F3_Test_Survey_DepthFT").add("F3_Test_Survey_DepthFT__XYinft_");
    }

    return ret;
}

static const int velids_direct[] = { 8, 9, 9, 14, 14, 16, 16 };
static const int velids_samedomain_otherunit[] = { 9, 8, 8, 15, 15, 17, 17 };
static const int velids_otherdomain_sameunit[] = { 14, 15, 15, 8, 8, 9, 9 };
static const int velids_otherdomain_otherunit[] = { 15, 14, 14, 9, 9, 8, 8 };

static const int vavgids_direct[] = { 18, 19, 19, 26, 26, 28, 28 };
static const int vavgids_samedomain_otherunit[] = { 19, 18, 18, 27, 27, 29, 29};
static const int vavgids_otherdomain_sameunit[] = { 26, 27, 27, 18, 18, 19, 19};
static const int vavgids_otherdomain_otherunit[] ={ 27, 26, 26, 19, 19, 18, 18};

static const int vrmsids[] = { 20, 21, 21, 20, 20, 21, 21 };

static MultiID getVelID( const int* grps, int idx )
{
    MultiID ret;
    ret.setGroupID( 100010 ).setObjectID( grps[idx] );
    return ret;
}

#define mMsg(txt) BufferString( SI().name(), ": ", txt )

BufferString msg_;


// 1e-3f radians < 0.058 degree
#define mCheckVal(ofsidx,zidx,val) \
{ \
    if ( !mIsEqual(vals.get(ofsidx,zidx),val,1e-3f) ) \
    { \
	msg_.set( mMsg("Invalid computed value: ") ) \
	    .add( "Expected: " ).add( val ).add( "; Computed: " ) \
	    .add( vals.get(ofsidx,zidx) ).add( " at zidx " ) \
	    .add( zidx ).add( " and offset idx: " ).add( ofsidx ); \
	return false; \
    } \
}


bool isRawAngleOK( const PreStack::Gather& angles )
{
    const Array2D<float>& vals = angles.data();
    mCheckVal( 0, 0,   0.f )
    mCheckVal( 5, 0,   M_PI_2f )
    mCheckVal( 1, 50,  0.905887909f )
    mCheckVal( 4, 50,  1.377249863f )
    mCheckVal( 2, 100, 0.905887909f )
    mCheckVal( 3, 100, 1.08918025f )
    mCheckVal( 2, 200, 0.549786276f )
    mCheckVal( 3, 200, 0.743334588f )
    mCheckVal( 1, 250, 0.234869558f )
    mCheckVal( 4, 250, 0.763504006f )
    mCheckVal( 0, 275, 0.f )
    mCheckVal( 5, 275, 0.820676323f )

    return true;
}


bool isMovingAverageAngleOK( const PreStack::Gather& angles )
{
    const Array2D<float>& vals = angles.data();
    mCheckVal( 0, 0,   0.f );
    mCheckVal( 5, 0,   1.55975366f );
    mCheckVal( 1, 50,  0.90728283f );
    mCheckVal( 4, 50,  1.37730265f );
    mCheckVal( 2, 100, 0.90623587f );
    mCheckVal( 3, 100, 1.08934581f );
    mCheckVal( 2, 200, 0.54997629f );
    mCheckVal( 3, 200, 0.7434904f );
    mCheckVal( 1, 250, 0.2349564f );
    mCheckVal( 4, 250, 0.7636112f );
    mCheckVal( 0, 275, 0.f );
    mCheckVal( 5, 275, 0.82726491f );

    return true;
}


bool isFFTAngleOK( const PreStack::Gather& angles )
{
    const Array2D<float>& vals = angles.data();
    mCheckVal( 0, 0,   0.f )
    mCheckVal( 5, 0,   1.5606885f )
    mCheckVal( 1, 50,  0.9083171f )
    mCheckVal( 4, 50,  1.37746108f )
    mCheckVal( 2, 100, 0.90595824f )
    mCheckVal( 3, 100, 1.08921146f )
    mCheckVal( 2, 200, 0.54981774f )
    mCheckVal( 3, 200, 0.74334842f )
    mCheckVal( 1, 250, 0.23362018f )
    mCheckVal( 4, 250, 0.76326013f )
    mCheckVal( 0, 275, 0.f )
    mCheckVal( 5, 275, 0.80776274f )

    return true;
}


bool isMutedGatherOK( const PreStack::Gather& output, bool toponly )
{
    const int nroffs = output.size( PreStack::Gather::offsetDim() == 0 );
    const int nrz = output.size( PreStack::Gather::zDim() == 0 );

    const bool zistime = output.zIsTime();
    const bool zinfeet = output.zInFeet();
    const int ioff = 3; // 500m
    const int topmuteidx  = zistime ? 110 : (zinfeet ? 91 : 83);
    const int tailmuteidx = zistime ? 172 : (zinfeet ? 147 : 135);

    const Array2D<float>& vals = output.data();
    mCheckVal( nroffs-1, 0, 0.f );	// top right corner
    mCheckVal( ioff, topmuteidx, 0.f ); // 30deg outer mute
    mCheckVal( ioff, topmuteidx+1, 1.f );
    if ( !toponly )
    {
	mCheckVal( ioff, tailmuteidx-1, 1.f );
	mCheckVal( ioff, tailmuteidx,	0.f ); // 20deg inner mute
	mCheckVal( 0, nrz-1, 0.f ); // bottom left corner
    }

    return true;
}

// Analytically computed values, not an approximation
static const float anglezeroidx[] =
	{ 0.f, M_PI_2f, M_PI_2f, M_PI_2f, M_PI_2f, M_PI_2f };
// Angles in time are independent of SRD
static const float anglestimesurvmididx[] =
	{ 0.f, 0.4356808f, 0.7496963f, 0.9493774f, 1.0779576f, 1.1650089f };
static const float anglestimesurvzidx[] =
	{ 0.f, 0.2534775f, 0.4780217f, 0.6606509f, 0.8031735f, 0.9133299f };
static const float anglestimesurvlastidx[] =
	{ 0.f, 0.2114259f, 0.4054794f, 0.5720755f, 0.7094278f, 0.8206763f };

// Angles in depth DO dependent on SRD
static const float anglesdepthmsurvmididx[] =
	{ 0.f, 0.3859646f, 0.6824316f, 0.8837874f, 1.0192462f, 1.1134146f };
static const float anglesdepthmsurvzidx[] =
	{ 0.f, 0.2497161f, 0.4716784f, 0.6531071f, 0.7953764f, 0.9057690f };
static const float anglesdepthmsurvlastidx[] =
	{ 0.f, 0.2028901f, 0.3903305f, 0.5529432f, 0.6885389f, 0.7994973f };

// Angles in depth DO dependent on SRD
static const float anglesdepthftsurvmididx[] =
	{ 0.f, 0.3865709f, 0.6832821f, 0.8846392f, 1.0200214f, 1.1141026f };
static const float anglesdepthftsurvzidx[] =
	{ 0.f, 0.2497914f, 0.4718056f, 0.6532588f, 0.7955335f, 0.9059216f };
static const float anglesdepthftsurvlastidx[] =
	{ 0.f, 0.2032376f, 0.3909500f, 0.5537297f, 0.6894022f, 0.8003765f };


static bool compareAngles( const PreStack::Gather& angles, int zidx,
			   bool depth, bool feet )
{
    const int nroffset = angles.data().info().getSize( 0 );
    const int nrz = angles.data().info().getSize( 1 );
    TypeSet<int> zidxs;
    zidxs += 0;
    zidxs += (nrz-1)/2;
    zidxs += zidx;
    zidxs += nrz - 1;
    const int nrztests = zidxs.size();

    TypeSet<const float*> targetvals;
    targetvals += anglezeroidx;
    targetvals += depth
		  ? (feet ? anglesdepthftsurvmididx : anglesdepthmsurvmididx)
		  : anglestimesurvmididx;
    targetvals += depth
		  ? (feet ? anglesdepthftsurvzidx : anglesdepthmsurvzidx )
		  : anglestimesurvzidx;
    targetvals += depth
		  ? (feet ? anglesdepthftsurvlastidx : anglesdepthmsurvlastidx)
		  : anglestimesurvlastidx;
    const Array2D<float>& vals = angles.data();
    for ( int idx=0; idx<nrztests; idx++ )
	for ( int ioff=0; ioff<nroffset; ioff++ )
	    { mCheckVal( ioff, zidxs[idx], targetvals[idx][ioff] ) }

    return true;
}

static bool testSmoothing( PreStack::VelocityBasedAngleComputer& computer,
			   ConstRefMan<PreStack::Gather>& angles )
{
    mRunStandardTestWithError( isRawAngleOK(*angles),
			       mMsg("Test raw angle values"), msg_ );

    computer.setMovingAverageSmoother( 0.1f, HanningWindow::sName() );
    angles = computer.computeAngles();
    mRunStandardTestWithError( angles && isMovingAverageAngleOK(*angles),
			       mMsg("Angle values after AVG filter"), msg_ );

    computer.setFFTSmoother( 10.f, 15.f );
    angles = computer.computeAngles();
    mRunStandardTestWithError( angles && isFFTAngleOK(*angles),
			       mMsg("Angle values after FFT Filter"), msg_ );
    return true;
}


static bool testAngleGatherComputer( const MultiID& velmid, const TrcKey& tk,
			const FlatPosData& fp, Seis::OffsetType offstyp,
			const ZDomain::Info& zinfo, int isurv, int zid )
{
    PtrMan<IOObj> velobj = IOM().get( velmid );
    mRunStandardTestWithError( velobj.ptr(),
			       mMsg("Velocity Model database object"),
			       IOM().message() );

    RefMan<PreStack::VelocityBasedAngleComputer> computer =
			    new PreStack::VelocityBasedAngleComputer();
    computer->setMultiID( velobj->key() );
    computer->setOutputSampling( fp, offstyp, zinfo );
    computer->setTrcKey( tk );
    mRunStandardTest( computer->isOK(), mMsg("Angle gather computer") );

    ConstRefMan<PreStack::Gather> angles = computer->computeAngles();
    mRunStandardTest( angles, mMsg("Compute angle gather") );
    mRunStandardTestWithError( compareAngles(*angles,zid,zinfo.isDepth(),
				zinfo.isDepthFeet()),
			       mMsg("Angles gather values"), msg_ );

    return isurv == 0 ? testSmoothing( *computer.ptr(), angles ) : true;
}


static bool testAngleMuteApplier( const MultiID& velid, PreStack::Gather& input)
{
    input.data().setAll( 1.f );
    TypeSet<float> offsets;
    const int nroffs = input.size( PreStack::Gather::offsetDim() == 0 );
    for ( int ioff=0; ioff<nroffs; ioff++ )
	offsets += input.getOffset( ioff );

    const bool offsetinfeet = input.isOffsetInFeet();

    PreStack::AngleMute muter;
    PreStack::AngleMute::AngleMutePars& pars = muter.params();
    pars.mutecutoff_ = 30.f;
    pars.velvolmid_ = velid;
    pars.tail_ = false;
    pars.taperlen_ = 0.f;
    pars.raypar_.set( RayTracer1D::sKeyOffset(), offsets );
    pars.raypar_.setYN( RayTracer1D::sKeyOffsetInFeet(), offsetinfeet );
    pars.smoothingpar_.set( PreStack::AngleComputer::sKeySmoothType(),
			    PreStack::AngleComputer::None );

    DataPackMgr& mgr = DPM( DataPackMgr::FlatID() );
    mgr.add( input );

    const BinID relbid( 0, 0 );
    DataPackID dpid = input.id();
    muter.setInput( relbid, dpid );
    muter.setOutputInterest( relbid, true );
    mRunStandardTestWithError( muter.prepareWork() && muter.execute(),
			       mMsg("Apply angle-based outer mute to gather"),
			       toString(muter.errMsg()) );

    DataPackID outid = muter.getOutput( relbid );
    ConstRefMan<PreStack::Gather> output = mgr.get<PreStack::Gather>( outid );
    mRunStandardTestWithError( output && isMutedGatherOK( *output, true ),
			       mMsg("Gather with outer mute applied"), msg_ );

    pars.mutecutoff_ = 20.f;
    pars.tail_ = true;
    muter.setInput( relbid, outid );
    output = nullptr;
    mRunStandardTestWithError( muter.prepareWork() && muter.execute(),
			       mMsg("Apply angle-based inner mute to gather"),
			       toString(muter.errMsg()) );

    outid = muter.getOutput( relbid );
    output = mgr.get<PreStack::Gather>( outid );
    mRunStandardTestWithError( output && isMutedGatherOK( *output, false ),
			       mMsg("Gather with outer and inner mute applied"),
			       msg_ );

    return true;
}


static bool testAngleMuteComputer( const MultiID& velid, const MultiID& muteid,
				   const TrcKey& tk,
				   const TypeSet<double>& offsets,
				   Seis::OffsetType offstyp )
{
    PreStack::AngleMuteComputer computer;
    PreStack::AngleMuteComputer::AngleMuteCompPars& pars = computer.params();
    pars.mutecutoff_ = 30.f;
    pars.velvolmid_ = velid;
    pars.outputmutemid_ = muteid;
    pars.raypar_.set( RayTracer1D::sKeyOffset(), offsets );
    pars.raypar_.setYN( RayTracer1D::sKeyOffsetInFeet(),
			offstyp == Seis::OffsetType::OffsetFeet );
    pars.smoothingpar_.set( PreStack::AngleComputer::sKeySmoothType(),
			    PreStack::AngleComputer::None );
    pars.tks_.set( tk );

    mRunStandardTestWithError( computer.execute(),
			       mMsg("Compute angle-based mute function"),
			       toString(computer.errMsg()) );

    return true;
}


mLoad1Module("PreStackProcessing")

bool BatchProgram::doWork( od_ostream& strm )
{
    mInitBatchTestProg();

    const double zstart = 0.;
    TypeSet<StepInterval<double> > zrgs;
    zrgs.add( StepInterval<double> (zstart,1.1,0.004) );
    zrgs.add( StepInterval<double> (zstart,1200.,5.) );
    zrgs.add( StepInterval<double> (zstart,3930.,15.) );

    TypeSet<const int*> velids;
    velids += velids_direct;
    velids += velids_samedomain_otherunit;
    velids += velids_otherdomain_sameunit;
    velids += velids_otherdomain_otherunit;
    velids += vavgids_direct;
    velids += vavgids_samedomain_otherunit;
    velids += vavgids_otherdomain_sameunit;
    velids += vavgids_otherdomain_otherunit;
    velids += vrmsids;

    //TWT=933ms, Z~=965m, Z~=3165ft
    const int zids [] = { 233, 193, 211 };

    const TrcKey tk( BinID(426,800) );
    const MultiID muteid( "100070.5" );

    SurveyDiskLocation sdl( IOM().rootDir() );
    int isurv = 0;
    for ( const auto* survdirnm : survDirNames() )
    {
	sdl.setDirName( survdirnm->str() );
	SurveyChanger changer( sdl );

	const ZDomain::Info& zinfo = SI().zDomainInfo();
	const bool zistime = zinfo.isTime();
	const Seis::OffsetType offstyp = SI().xyInFeet()
				       ? Seis::OffsetType::OffsetFeet
				       : Seis::OffsetType::OffsetMeter;
	const bool offsetsinfeet = offstyp == Seis::OffsetType::OffsetFeet;
	const bool zinfeet = SI().zInFeet();
	const float srd =
	    UnitOfMeasure::surveyDefSRDStorageUnit()
					->getUserValueFromSI( srd_ );
	eSI().setSeismicReferenceDatum( srd );
	const int residx = zistime ? 0 : (zinfeet ? 2 : 1);

	const StepInterval<double>& zrange = zrgs[residx];
	StepInterval<double> offsetrange( 0., 2500., 500. );
	if ( offsetsinfeet )
	    offsetrange.scale( mToFeetFactorF );

	FlatPosData fp;
	fp.setRange( true, offsetrange );
	fp.setRange( false, zrange );

	offsetrange = StepInterval<double>( 200., 2000., 100. );
	if ( offsetsinfeet )
	    offsetrange.scale( mToFeetFactorF );
	TypeSet<double> offsets;
	for ( int ioff=0; ioff<offsetrange.nrSteps()+1; ioff++ )
	    offsets += offsetrange.atIndex( ioff );

	FlatPosData fpmute;
	fpmute.setRange( true, offsetrange );
	fpmute.setRange( false, zrange );
	RefMan<PreStack::Gather> input = new PreStack::Gather( fpmute, offstyp,
							       zinfo );
	input->setTrcKey( tk );
	input->setCorrected( true );

	const int& zid = zids[residx];

	int idx = 0;
	for ( const auto& velidset : velids )
	{
	    const MultiID velmid = getVelID( velidset, isurv );
	    if ( !testAngleGatherComputer(velmid,tk,fp,offstyp,zinfo,
					  isurv,zid) )
		return false;

	    if ( idx==0 )
	    {
		if ( !testAngleMuteApplier(velmid,*input) ||
		     !testAngleMuteComputer(velmid,muteid,tk,
					    offsets,offstyp) )
		    return false;
	    }

	    idx++;
	}

	isurv++;
    }

    return true;
}
