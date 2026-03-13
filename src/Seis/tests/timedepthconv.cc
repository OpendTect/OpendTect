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
   This test program is based on the following time-depth models:
   SRD = 50ft = 15.24m

   Model 1, inl 426, crl 700:
   TWT =  616.60ms - Vint = 1960 m/s  ~ FS8
   TWT =  677.70ms - Vint = 2175 m/s  ~ FS7
   TWT = 1007.54ms - Vint = 2260 m/s  ~ FS4
   TWT = 1100.00ms - Vint = 2400 m/s  end of Z sampling in time

   Model 2, inl 426, crl  800:
   TWT =  573.01ms - Vint = 1960 m/s  ~ FS8
   TWT =  623.41ms - Vint = 2175 m/s  ~ FS7
   TWT =  959.01ms - Vint = 2260 m/s  ~ FS4
   TWT = 1100.00ms - Vint = 2400 m/s  end of Z sampling in time

   Model 3, inl 426, crl  900:
   TWT =  558.10ms - Vint = 1960 m/s  ~ FS8
   TWT =  605.54ms - Vint = 2175 m/s  ~ FS7
   TWT =  938.03ms - Vint = 2260 m/s  ~ FS4
   TWT = 1100.00ms - Vint = 2400 m/s  end of Z sampling in time

   Model 4, inl 426, crl 1200:
   TWT =  537.92ms - Vint = 1960 m/s  ~ FS8
   TWT =  559.11ms - Vint = 2175 m/s  ~ FS7
   TWT =  842.07ms - Vint = 2260 m/s  ~ FS4
   TWT = 1100.00ms - Vint = 2400 m/s  end of Z sampling in time

*/

// True values for the velocity model case @ inl=426, crl=800
static const float timeval_ = 0.95901f;
static const float depthmval_ = 980.3473f;

static const TrcKey& getTk()
{
    static TrcKey tk( BinID(426,800) );
    return tk;
}

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
	   .add( "F3_Test_Survey_DepthFT")
	   .add("F3_Test_Survey_DepthFT__XYinft_");
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
	BufferString msg( "From: [", zrgfrom.start_, ", " ); \
	msg.add( zrgfrom.stop_ ).add( "] step " ).add( zrgfrom.step_ ) \
	   .add( " - To: [" ).add( zrgto.start_ ).add( ", " ) \
	   .add( zrgto.stop_ ) \
	   .add( "] step " ).add( zrgto.step_ ); \
	tstStream() << mMsg(msg) << od_newline; \
    } \
}


static bool isVavgOK( const Interval<float>& vavgrg,
		      const Interval<float>& truerg, float eps )
{
    mCheckVal( vavgrg.start_, truerg.start_, eps )
    mCheckVal( vavgrg.stop_, truerg.stop_, eps )

    return true;
}


static bool testSimpleT2DTransform( const MultiID& mid )
{
    ConstRefMan<ZAxisTransform> stretcher = new SimpleT2DTransform( mid );
    mRunStandardTest( stretcher->isOK(), mMsg("Time-to-depth simple model") )
    mRunStandardTest( stretcher->fromZDomainInfo().def_.isTime() &&
		      stretcher->toZDomainInfo().def_.isDepth(),
		      mMsg("Time-to-depth simple model from-to domains") )
    const ZSampling zrgfrom = stretcher->getZInterval( true, false );
    const ZSampling zrgto = stretcher->getZInterval( false, false );
    mRunStandardTest( !zrgfrom.isUdf() && !zrgto.isUdf(),
		      mMsg("Time-to-depth simple model ranges") )
    mPrintWorkZrg()

    const TrcKey& tk = TrcKey::udf();
    const float depthval = stretcher->transformTrc( tk, timeval_ );
    const float sidepthval = SI().depthsInFeet() ? depthval * mFromFeetFactorF
						 : depthval;
    mRunStandardTest( mIsEqual(sidepthval,958.38f,1e-2f),
		      mMsg("Transformed Z value") )

    const float timeval = stretcher->transformTrcBack( tk, depthval );
    mRunStandardTest( mIsEqual(timeval,timeval_,1e-5f),
		      mMsg("Transformed back Z value") )

    return true;
}


static bool testSimpleD2TTransform( const MultiID& mid )
{
    ConstRefMan<ZAxisTransform> stretcher = new SimpleD2TTransform( mid );
    mRunStandardTest( stretcher->isOK(), mMsg("Depth-to-time simple model") )
    mRunStandardTest( stretcher->fromZDomainInfo().def_.isDepth() &&
		      stretcher->toZDomainInfo().def_.isTime(),
		      mMsg("Depth-to-time simple model from-to domains") )

    const ZSampling zrgfrom = stretcher->getZInterval( true, false );
    const ZSampling zrgto = stretcher->getZInterval( false, false );
    mRunStandardTest( !zrgfrom.isUdf() && !zrgto.isUdf(),
		      mMsg("Depth-to-time simple model ranges") )
    mPrintWorkZrg()

    const TrcKey& tk = TrcKey::udf();
    float sidepthval = 958.38f;
    float depthval = SI().depthsInFeet() ? sidepthval * mToFeetFactorF
					       : sidepthval;
    const float timeval = stretcher->transformTrc( tk, depthval );
    mRunStandardTest( mIsEqual(timeval,timeval_,1e-5f),
		      mMsg("Transformed Z value") )

    depthval = stretcher->transformTrcBack( tk, timeval_ );
    sidepthval = SI().depthsInFeet() ? depthval * mFromFeetFactorF
				     : depthval;
    mRunStandardTest( mIsEqual(sidepthval,958.38f,1e-2f),
		      mMsg("Transformed back Z value") )

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
    mRunStandardTest( stretcher->isOK(), mMsg("Time-to-depth linear model") )
    mRunStandardTest( stretcher->fromZDomainInfo().def_.isTime() &&
		      stretcher->toZDomainInfo().def_.isDepth(),
		      mMsg("Time-to-depth linear model from-to domains") )
    const ZSampling zrgfrom = stretcher->getZInterval( true, false );
    const ZSampling zrgto = stretcher->getZInterval( false, false );
    mRunStandardTest( !zrgfrom.isUdf() && !zrgto.isUdf(),
		      mMsg("Time-to-depth linear model ranges") )
    mPrintWorkZrg()

    const TrcKey& tk = TrcKey::udf();
    const float depthval = stretcher->transformTrc( tk, timeval_ );
    const float sidepthval = SI().depthsInFeet() ? depthval * mFromFeetFactorF
						 : depthval;
    mRunStandardTest( mIsEqual(sidepthval,1458.32f,1e-2f),
		      mMsg("Transformed Z value") )

    const float timeval = stretcher->transformTrcBack( tk, depthval );
    mRunStandardTest( mIsEqual(timeval,timeval_,1e-5f),
		      mMsg("Transformed back Z value") )

    return true;
}


static bool testLinearD2TTransform()
{
    const double v0 =
		LinearVelTransform::velUnit()->getUserValueFromSI( 3000. );
    const double k = 0.1;
    ConstRefMan<ZAxisTransform> stretcher = new LinearD2TTransform( v0, k );
    mRunStandardTest( stretcher->isOK(), mMsg("Depth-to-time linear model") )
    mRunStandardTest( stretcher->fromZDomainInfo().def_.isDepth() &&
		      stretcher->toZDomainInfo().def_.isTime(),
		      mMsg("Depth-to-time linear model from-to domains") )
    const ZSampling zrgfrom = stretcher->getZInterval( true, false );
    const ZSampling zrgto = stretcher->getZInterval( false, false );
    mRunStandardTest( !zrgfrom.isUdf() && !zrgto.isUdf(),
		      mMsg("Depth-to-time linear model ranges") )
    mPrintWorkZrg()

    const TrcKey& tk = TrcKey::udf();
    float sidepthval = 1458.32f;
    float depthval = SI().depthsInFeet() ? sidepthval * mToFeetFactorF
					 : sidepthval;
    const float timeval = stretcher->transformTrc( tk, depthval );
    mRunStandardTest( mIsEqual(timeval,timeval_,1e-5f),
		      mMsg("Transformed Z value") )

    depthval = stretcher->transformTrcBack( tk, timeval_ );
    sidepthval = SI().depthsInFeet() ? depthval * mFromFeetFactorF
				     : depthval;
    mRunStandardTest( mIsEqual(sidepthval,1458.32f,1e-2f),
		      mMsg("Transformed back Z value") )

    return true;
}


static bool testLinearVelTransform()
{
    return testLinearT2DTransform() && testLinearD2TTransform();
}


bool testTime2DepthStretcher( const MultiID& mid )
{
    RefMan<ZAxisTransform> stretcher = new Time2DepthStretcher( mid );
    mRunStandardTest( stretcher->isOK() && stretcher->needsVolumeOfInterest(),
		      mMsg("Time-to-depth stretcher") )
    mRunStandardTest( stretcher->fromZDomainInfo().def_.isTime() &&
		      stretcher->toZDomainInfo().def_.isDepth(),
		      mMsg("Time-to-depth stretcher from-to domains") )
    const ZSampling zrgfrom = stretcher->getZInterval( true, false );
    const ZSampling zrgto = stretcher->getZInterval( false, false );
    mRunStandardTest( !zrgfrom.isUdf() && !zrgto.isUdf(),
		      mMsg("Time-to-depth stretcher ranges") )
    mPrintWorkZrg()

    const TrcKey& tk = getTk();
    TrcKeyZSampling tkzs;
    tkzs.hsamp_.set( tk );

    bool zistrans = false;
    tkzs.zsamp_ = stretcher->getZInterval( !zistrans, true );
    tkzs.zsamp_.stop_ = tkzs.zsamp_.atIndex( tkzs.nrZ()-2 );
    mRunStandardTestWithError(
	   stretcher->addVolumeOfInterest(tkzs,zistrans) == 0 &&
	   stretcher->loadDataIfMissing(0),
	   "Load input velocity model (from zDomain)",
	   stretcher->errMsg().getString() )

    float depthval = stretcher->transformTrc( tk, timeval_ );
    float sidepthval = SI().depthsInFeet() ? depthval * mFromFeetFactorF
					   : depthval;
    mRunStandardTest( mIsEqual(sidepthval,depthmval_,1e-1f),
		      mMsg("Transformed Z value") )

    depthval = SI().depthsInFeet() ? depthmval_ * mToFeetFactorF : depthmval_;
    float timeval = stretcher->transformTrcBack( tk, depthval );
    mRunStandardTest( mIsEqual(timeval,timeval_,1e-4f),
		      mMsg("Transformed back Z value") )

    stretcher->removeVolumeOfInterest( 0 );
    zistrans = true;

    tkzs.zsamp_ = stretcher->getZInterval( !zistrans, true );
    tkzs.zsamp_.stop_ = tkzs.zsamp_.atIndex( tkzs.nrZ()-2 );
    mRunStandardTestWithError(
	   stretcher->addVolumeOfInterest(tkzs,zistrans) == 0 &&
	   stretcher->loadDataIfMissing(0),
	   "Load input velocity model (to zDomain)",
	   stretcher->errMsg().getString() )

    depthval = stretcher->transformTrc( tk, timeval_ );
    sidepthval = SI().depthsInFeet() ? depthval * mFromFeetFactorF : depthval;
    mRunStandardTest( mIsEqual(sidepthval,depthmval_,1e-1f),
		      mMsg("Transformed Z value") )

    depthval = SI().depthsInFeet() ? depthmval_ * mToFeetFactorF : depthmval_;
    timeval = stretcher->transformTrcBack( tk, depthval );
    mRunStandardTest( mIsEqual(timeval,timeval_,1e-4f),
		      mMsg("Transformed back Z value") )

    return true;
}


bool testDepth2TimeStretcher( const MultiID& mid )
{
    RefMan<ZAxisTransform> stretcher = new Depth2TimeStretcher( mid );
    mRunStandardTest( stretcher->isOK(), mMsg("Depth-to-time stretcher") )
    mRunStandardTest( stretcher->fromZDomainInfo().def_.isDepth() &&
		      stretcher->toZDomainInfo().def_.isTime(),
		      mMsg("Depth-to-time stretcher from-to domains") )

    const ZSampling zrgfrom = stretcher->getZInterval( true, false );
    const ZSampling zrgto = stretcher->getZInterval( false, false );
    mRunStandardTest( !zrgfrom.isUdf() && !zrgto.isUdf(),
		      mMsg("Depth-to-time stretcher ranges") )
    mPrintWorkZrg()

    const TrcKey& tk = getTk();
    TrcKeyZSampling tkzs;
    tkzs.hsamp_.set( tk );

    bool zistrans = false;

    tkzs.zsamp_ = stretcher->getZInterval( !zistrans, true );
    tkzs.zsamp_.stop_ = tkzs.zsamp_.atIndex( tkzs.nrZ()-2 );
    mRunStandardTestWithError(
	    stretcher->addVolumeOfInterest(tkzs,zistrans) == 0 &&
	    stretcher->loadDataIfMissing(0),
	    "Load input velocity model (from zDomain)",
	    stretcher->errMsg().getString() )

    float sidepthval = depthmval_;
    float depthval = SI().depthsInFeet() ? sidepthval * mToFeetFactorF
					 : sidepthval;
    float timeval = stretcher->transformTrc( tk, depthval );
    mRunStandardTest( mIsEqual(timeval,timeval_,1e-4f),
		      mMsg("Transformed Z value") )

    depthval = stretcher->transformTrcBack( tk, timeval_ );
    sidepthval = SI().depthsInFeet() ? depthval * mFromFeetFactorF
				     : depthval;
    mRunStandardTest( mIsEqual(sidepthval,depthmval_,1e-1f),
		      mMsg("Transformed back Z value") )

    stretcher->removeVolumeOfInterest( 0 );
    zistrans = true;

    tkzs.zsamp_ = stretcher->getZInterval( !zistrans, true );
    tkzs.zsamp_.stop_ = tkzs.zsamp_.atIndex( tkzs.nrZ()-2 );
    mRunStandardTestWithError(
	    stretcher->addVolumeOfInterest(tkzs,zistrans) == 0 &&
	    stretcher->loadDataIfMissing(0),
	    "Load input velocity model (to zDomain)",
	    stretcher->errMsg().getString() )

    sidepthval = depthmval_;
    depthval = SI().depthsInFeet() ? sidepthval * mToFeetFactorF : sidepthval;
    timeval = stretcher->transformTrc( tk, depthval );
    mRunStandardTest( mIsEqual(timeval,timeval_,1e-4f),
		      mMsg("Transformed Z value") )

    depthval = stretcher->transformTrcBack( tk, timeval_ );
    sidepthval = SI().depthsInFeet() ? depthval * mFromFeetFactorF : depthval;
    mRunStandardTest( mIsEqual(sidepthval,depthmval_,1e-1f),
		      mMsg("Transformed back Z value") )

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
			       IOM().message() )

    VelocityDesc desc;
    mRunStandardTest( desc.usePar( velobj->pars() ),
		      mMsg("Reading velocity tag") )
    mRunStandardTest( desc.isVelocity(),
		      mMsg("Dataset is a velocity model") )

    VelocityModelScanner scanner( *velobj, desc );
    mRunStandardTestWithError( scanner.execute(),
			       mMsg("Velocity Model Scanner execution"),
			       toString(scanner.uiMessage()) )

    const bool zistime = SI().zIsTime();
    const bool zinfeet = SI().zInFeet();
    Interval<float> truergtop( topvavg(), topvavg() );
    Interval<float> truergbot = zistime
			? botvavgTWT() : (zinfeet ? botvavgFT() : botvavgM() );
    const UnitOfMeasure* veluom = scanner.velUnit();
    truergtop.start_ = veluom->getUserValueFromSI( truergtop.start_ );
    truergtop.stop_ = veluom->getUserValueFromSI( truergtop.stop_ );
    truergbot.start_ = veluom->getUserValueFromSI( truergbot.start_ );
    truergbot.stop_ = veluom->getUserValueFromSI( truergbot.stop_ );

    mRunStandardTest( isVavgOK( scanner.getTopVAvg(), truergtop, defveleps_ ),
		      mMsg("Velocity average range at Top") )
    mRunStandardTest( isVavgOK( scanner.getBotVAvg(), truergbot, defveleps_ ),
		      mMsg("Velocity average range at Bottom") )

    return true;
}


static bool testMeter2FeetStretcher()
{
    ConstRefMan<ZAxisTransform> stretcher = new SimpleMeterFeetTransform();
    mRunStandardTest( stretcher->isOK(), "Meter-to-Feet stretcher" );
    mRunStandardTest( stretcher->fromZDomainInfo().isDepthMeter() &&
		      stretcher->toZDomainInfo().isDepthFeet(),
		      "Meter-to-Feet stretcher from-to domains" );
    const ZSampling zrgfrom( 0.f, 2100.f, 5.f );
    const ZSampling zrgto = stretcher->getZInterval( false, false, &zrgfrom );
    mRunStandardTest( !zrgfrom.isUdf() && !zrgto.isUdf() &&
		      mIsEqual(zrgto.stop_,6889.76f,1e-2f) &&
		      mIsEqual(zrgto.step_,16.4042f,1e-6f),
		      "Meter-to-Feet stretcher ranges" );
    mPrintWorkZrg()

    return true;
}


static bool testFeet2MeterStretcher()
{
    ConstRefMan<ZAxisTransform> stretcher = new SimpleFeetMeterTransform();
    mRunStandardTest( stretcher->isOK(), "Feet-to-Meter stretcher" );
    mRunStandardTest( stretcher->fromZDomainInfo().isDepthFeet() &&
		      stretcher->toZDomainInfo().isDepthMeter(),
		      "Feet-to-Meter stretcher from-to domains" );
    const ZSampling zrgfrom( 0.f, 7000.f, 15.f );
    const ZSampling zrgto = stretcher->getZInterval( false, false, &zrgfrom );
    mRunStandardTest( !zrgfrom.isUdf() && !zrgto.isUdf() &&
		      mIsEqual(zrgto.stop_,2133.6f,1e-2f) &&
		      mIsEqual(zrgto.step_,4.572f,1e-6f),
		      "Feet-to-Meter stretcher ranges" );
    mPrintWorkZrg()

    return true;
}


static bool testSimpleDepthModel()
{
    return testMeter2FeetStretcher() && testFeet2MeterStretcher();
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
			       toString(uirv) )

    printranges_ = clParser().hasKey( "print" );

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

    const MultiID tablet2did_m( 100090,6 );
    const MultiID tablet2did_ft( 100090,7 );
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

	if ( !testSimpleTimeDepthModel(tablet2did_m) ||
	     !testSimpleTimeDepthModel(tablet2did_ft) ||
	     !testLinearVelTransform() )
	    return 1;

	int idx = 0;
	for ( const auto& velidset : velids )
	{
	    const MultiID velmid = getVelID( velidset, isurv );
	    if ( !testVelocityStretcher(velmid) )
		return 1;

	    if ( idx==0 && !testVelocityModelScanner(velmid) )
		return 1;

	    idx++;
	}

	isurv++;
    }

    if ( !testSimpleDepthModel() )
	return 1;

    return 0;
}
