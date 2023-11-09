/*+
________________________________________________________________________

 Copyright:	(C) 1995-2023 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ioman.h"
#include "ioobj.h"
#include "filepath.h"
#include "moddepmgr.h"
#include "multiid.h"
#include "simpletimedepthmodel.h"
#include "survinfo.h"
#include "testprog.h"
#include "timedepthconv.h"
#include "unitofmeasure.h"
#include "veldesc.h"

/*
   This test program is based on the following two time-depth models:
   SRD = 50ft = 15.24m

   Model 1, inl 426, crl 700:
   TWT =  616.16ms - Vint = 1960 m/s  ~ FS8
   TWT =  677.70ms - Vint = 2175 m/s  ~ FS7
   TWT = 1007.57ms - Vint = 2260 m/s  ~ FS4
   TWT = 1100.00ms - Vint = 2400 m/s  end of Z sampling in time

   Model 2, inl 426, crl 1200:
   TWT =  537.92ms - Vint = 1960 m/s  ~ FS8
   TWT =  559.11ms - Vint = 2175 m/s  ~ FS7
   TWT =  842.07ms - Vint = 2260 m/s  ~ FS4
   TWT = 1100.00ms - Vint = 2400 m/s  end of Z sampling in time

*/

static bool printranges_ = false;

/* The first Z of the surveys in always in the first layer, hence a single
   vavg range at the top: */
static float topvavg()	{ return 1960.f; }
// The bottom Vavg depends on the last Z of the survey, hence:
static Interval<float> botvavgTWT()
{ return Interval<float>( 2098.883f, 2144.485f ); }
static Interval<float> botvavgM()
{ return Interval<float>( 2112.153f, 2151.227f ); }
static Interval<float> botvavgFT()
{ return Interval<float>( 2111.707f, 2150.834f ); }
static float defveleps_ = 1e-1f;

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

static MultiID getVelID( const int* grps, int idx )
{
    MultiID ret;
    ret.setGroupID( 100010 ).setObjectID( grps[idx] );
    return ret;
}

#define mMsg(txt) BufferString( SI().name(), ": ", txt )

BufferString msg_;


#define mCheckVal(val,expval,defeps) \
{ \
    if ( !mIsEqual(val,expval,defeps) ) \
    { \
	msg_.set( mMsg("Invalid scanned velocity: ") ) \
	   .add( "Expected: " ).add( expval ) \
	   .add( "; Computed: " ).add( val ); \
	return false; \
    } \
}

#define mPrintWorkZrg() \
{ \
    if ( printranges_ ) \
    { \
	BufferString msg( "From: [", zrgfrom.start, ", " ); \
	msg.add( zrgfrom.stop ).add( "] step " ).add( zrgfrom.step ) \
	   .add( " - To: [" ).add( zrgto.start ).add( ", " ).add( zrgto.stop ) \
	   .add( "] step " ).add( zrgto.step ); \
	tstStream() << mMsg(msg) << od_newline; \
    } \
}


static bool isVavgOK( const Interval<float>& vavgrg,
		      const Interval<float>& truerg, float eps )
{
    mCheckVal( vavgrg.start, truerg.start, eps )
    mCheckVal( vavgrg.stop, truerg.stop, eps )

    return true;
}


static bool testSimpleT2DTransform( const MultiID& mid )
{
    ConstRefMan<ZAxisTransform> stretcher = new SimpleT2DTransform( mid );
    mRunStandardTest( stretcher->isOK(), mMsg("Time-to-depth simple model") );
    mRunStandardTest( stretcher->fromZDomainInfo().def_.isTime() &&
		      stretcher->toZDomainInfo().def_.isDepth(),
		      mMsg("Time-to-depth simple model from-to domains") );
    const ZSampling zrgfrom = stretcher->getZInterval( true, false );
    const ZSampling zrgto = stretcher->getZInterval( false, false );
    mRunStandardTest( !zrgfrom.isUdf() && !zrgto.isUdf(),
		      mMsg("Time-to-depth simple model ranges") );
    mPrintWorkZrg()

    return true;
}


static bool testSimpleD2TTransform( const MultiID& mid )
{
    ConstRefMan<ZAxisTransform> stretcher = new SimpleD2TTransform( mid );
    mRunStandardTest( stretcher->isOK(), mMsg("Depth-to-time simple model") );
    mRunStandardTest( stretcher->fromZDomainInfo().def_.isDepth() &&
		      stretcher->toZDomainInfo().def_.isTime(),
		      mMsg("Depth-to-time simple model from-to domains") );

    const ZSampling zrgfrom = stretcher->getZInterval( true, false );
    const ZSampling zrgto = stretcher->getZInterval( false, false );
    mRunStandardTest( !zrgfrom.isUdf() && !zrgto.isUdf(),
		      mMsg("Depth-to-time simple model ranges") );
    mPrintWorkZrg()

    return true;
}


static bool testSimpleTimeDepthModel( const MultiID& mid )
{
    return testSimpleT2DTransform( mid ) && testSimpleD2TTransform( mid );
}


static bool testLinearT2DTransform()
{
    const double v0 =
		LinearVelTransform::velUnit()->getUserValueFromSI( 3000. );
    const double k = 0.1;
    ConstRefMan<ZAxisTransform> stretcher = new LinearT2DTransform( v0, k );
    mRunStandardTest( stretcher->isOK(), mMsg("Time-to-depth linear model") );
    mRunStandardTest( stretcher->fromZDomainInfo().def_.isTime() &&
		      stretcher->toZDomainInfo().def_.isDepth(),
		      mMsg("Time-to-depth linear model from-to domains") );
    const ZSampling zrgfrom = stretcher->getZInterval( true, false );
    const ZSampling zrgto = stretcher->getZInterval( false, false );
    mRunStandardTest( !zrgfrom.isUdf() && !zrgto.isUdf(),
		      mMsg("Time-to-depth linear model ranges") );
    mPrintWorkZrg()

    return true;
}


static bool testLinearD2TTransform()
{
    const double v0 =
		LinearVelTransform::velUnit()->getUserValueFromSI( 3000. );
    const double k = 0.1;
    ConstRefMan<ZAxisTransform> stretcher = new LinearD2TTransform( v0, k );
    mRunStandardTest( stretcher->isOK(), mMsg("Depth-to-time linear model") );
    mRunStandardTest( stretcher->fromZDomainInfo().def_.isDepth() &&
		      stretcher->toZDomainInfo().def_.isTime(),
		      mMsg("Depth-to-time linear model from-to domains") );
    const ZSampling zrgfrom = stretcher->getZInterval( true, false );
    const ZSampling zrgto = stretcher->getZInterval( false, false );
    mRunStandardTest( !zrgfrom.isUdf() && !zrgto.isUdf(),
		      mMsg("Depth-to-time linear model ranges") );
    mPrintWorkZrg()

    return true;
}


static bool testLinearVelTransform()
{
    return testLinearT2DTransform() && testLinearD2TTransform();
}


bool testTime2DepthStretcher( const MultiID& mid )
{
    ConstRefMan<ZAxisTransform> stretcher = new Time2DepthStretcher( mid );
    mRunStandardTest( stretcher->isOK(), mMsg("Time-to-depth stretcher") );
    mRunStandardTest( stretcher->fromZDomainInfo().def_.isTime() &&
		      stretcher->toZDomainInfo().def_.isDepth(),
		      mMsg("Time-to-depth stretcher from-to domains") );
    const ZSampling zrgfrom = stretcher->getZInterval( true, false );
    const ZSampling zrgto = stretcher->getZInterval( false, false );
    mRunStandardTest( !zrgfrom.isUdf() && !zrgto.isUdf(),
		      mMsg("Time-to-depth stretcher ranges") );
    mPrintWorkZrg()

    return true;
}


bool testDepth2TimeStretcher( const MultiID& mid )
{
    ConstRefMan<ZAxisTransform> stretcher = new Depth2TimeStretcher( mid );
    mRunStandardTest( stretcher->isOK(), mMsg("Depth-to-time stretcher") );
    mRunStandardTest( stretcher->fromZDomainInfo().def_.isDepth() &&
		      stretcher->toZDomainInfo().def_.isTime(),
		      mMsg("Depth-to-time stretcher from-to domains") );

    const ZSampling zrgfrom = stretcher->getZInterval( true, false );
    const ZSampling zrgto = stretcher->getZInterval( false, false );
    mRunStandardTest( !zrgfrom.isUdf() && !zrgto.isUdf(),
		      mMsg("Depth-to-time stretcher ranges") );
    mPrintWorkZrg()

    return true;
}


bool testVelocityStretcher( const MultiID& mid )
{
    return testTime2DepthStretcher( mid ) && testDepth2TimeStretcher( mid );
}


static bool testVelocityModelScanner( const MultiID& mid )
{
    PtrMan<IOObj> velobj = IOM().get( mid );
    mRunStandardTestWithError( velobj.ptr(),
			       mMsg("Velocity Model database object"),
			       IOM().message() );

    VelocityDesc desc;
    mRunStandardTest( desc.usePar( velobj->pars() ),
		      mMsg("Reading velocity tag") );
    mRunStandardTest( desc.isVelocity(),
		      mMsg("Dataset is a velocity model") );

    VelocityModelScanner scanner( *velobj, desc );
    mRunStandardTestWithError( scanner.execute(),
			       mMsg("Velocity Model Scanner execution"),
			       toString(scanner.uiMessage()) );

    const bool zistime = SI().zIsTime();
    const bool zinfeet = SI().zInFeet();
    Interval<float> truergtop( topvavg(), topvavg() );
    Interval<float> truergbot = zistime
			? botvavgTWT() : (zinfeet ? botvavgFT() : botvavgM() );
    const UnitOfMeasure* veluom = UnitOfMeasure::surveyDefVelUnit();
    truergtop.start = veluom->getUserValueFromSI( truergtop.start );
    truergtop.stop = veluom->getUserValueFromSI( truergtop.stop );
    truergbot.start = veluom->getUserValueFromSI( truergbot.start );
    truergbot.stop = veluom->getUserValueFromSI( truergbot.stop );

    mRunStandardTest( isVavgOK( scanner.getTopVAvg(), truergtop, defveleps_ ),
		      mMsg("Velocity average range at Top") );
    mRunStandardTest( isVavgOK( scanner.getBotVAvg(), truergbot, defveleps_ ),
		      mMsg("Velocity average range at Bottom") );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProgDR();

    OD::ModDeps().ensureLoaded("Seis");

    const BufferStringSet& survdirnms = survDirNames();

    clParser().setKeyHasValue( "datadir" );
    BufferString basedir;
    clParser().getVal( "datadir", basedir );
    const uiRetVal uirv = IOMan::setDataSource( basedir.buf(),
						survdirnms.first()->str() );
    mRunStandardTestWithError( uirv.isOK(), "Initialize the first project",
			       toString(uirv) );

    const int* velids = velids_direct;

    const MultiID tablet2did_m( "100090.6" );
    const MultiID tablet2did_ft( "100090.7" );
    SurveyDiskLocation sdl( IOM().rootDir() );
    int isurv = 0;
    for ( const auto* survdirnm : survdirnms )
    {
	sdl.setDirName( survdirnm->buf() );
	SurveyChanger changer( sdl );

	const float srd =
	    UnitOfMeasure::surveyDefSRDStorageUnit()
					->getUserValueFromSI( srd_ );
	eSI().setSeismicReferenceDatum( srd );

	const MultiID velid = getVelID( velids, isurv );
	if ( !testSimpleTimeDepthModel(tablet2did_m) ||
	     !testSimpleTimeDepthModel(tablet2did_ft) ||
	     !testLinearVelTransform() ||
	     !testVelocityStretcher(velid) ||
	     !testVelocityModelScanner(velid) )
	    return 1;

	isurv++;
    }

    return 0;
}
