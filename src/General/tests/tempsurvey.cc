/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "surveyfile.h"

#include "coordsystem.h"
#include "file.h"
#include "filepath.h"
//#include "ioman.h"
#include "jsonkeystrs.h"
//#include "keystrs.h"
#include "moddepmgr.h"
#include "odjson.h"
//#include "plugins.h"
//#include "survinfo.h"
#include "testprog.h"
#include "ziputils.h"

#include <csignal>
#include <cstdlib>

static bool sCleanupAtExit = true;
static BufferString jsonworkdir_;
static BufferString jsonsurvdir_;
static BufferString jsonsurvzip_;
static BufferString defsurvbasedir_;
static BufferString defsurvzip_;
static BufferString zipbasedir_;


static void registerSurveyPaths( const SurveyCreator& surv,
				 BufferString& basedir, BufferString& zipfp )
{
    basedir.set( surv.getTempBaseDir() );
    zipfp.set( surv.getZipArchiveLocation() );
}


static void registerJsonSurveyPaths( const SurveyCreator& surv )
{
    const FilePath survfp( surv.getTempBaseDir(), surv.getSurveyDir() );
    jsonsurvdir_.set( survfp.fullPath() );
    jsonsurvzip_.set( surv.getZipArchiveLocation() );
}


static BufferString coordKey( int idx, bool isx )
{
    return BufferString( "coord", idx, isx ? "X" : "Y" );
}


static void removeSurveyZipFile( const char* zipfp )
{
    if ( !zipfp || !*zipfp || !File::exists(zipfp) )
	return;

    File::remove( zipfp );
    FilePath fp( zipfp );
    fp.setExtension( SurveyFile::bckupExtStr() );
    File::remove( fp.fullPath() );
}


static void cleanup()
{
    if ( !sCleanupAtExit )
	return;

    if ( File::exists(jsonsurvdir_.buf()) )
	File::removeDir( jsonsurvdir_.buf() );

    if ( File::exists(jsonworkdir_.buf()) )
	File::removeDir( jsonworkdir_.buf() );

    removeSurveyZipFile( jsonsurvzip_.buf() );

    if ( File::exists(defsurvbasedir_.buf()) )
	File::removeDir( defsurvbasedir_.buf() );

    if ( File::exists(zipbasedir_.buf()) )
	File::removeDir( zipbasedir_.buf() );

    removeSurveyZipFile( defsurvzip_.buf() );
}


static void signalHandler( int signum )
{
    errStream() << "Interrupt signal (" << signum << ") received." << od_endl;
    cleanup();
    exit( signum );
}

static BufferString expectValErrMsg( const char* expectval, const char* gotval )
{
    BufferString errmsg( "Expected : ", expectval, ", Got : " );
    errmsg.add( gotval );
    return errmsg;
}


static BufferString expectValErrMsg( double expectval, double gotval )
{
    BufferString errmsg( "Expected : " );
    errmsg.add( expectval ).add( ", Got : " ).add( gotval );
    return errmsg;
}


static bool pathsAreEqual( const char* p1, const char* p2 )
{
    return FilePath(p1).fullPath() == FilePath(p2).fullPath();
}


bool testSurvey( SurveyCreator& surv )
{
    mRunStandardTest( surv.isOK(), "Constructed Survey" );

    uiRetVal ret = surv.mount();
    mRunStandardTestWithError( ret.isOK() && surv.isMounted(), "Mount",
			       ret.getText() );

    ret = surv.activate();
    mRunStandardTestWithError( ret.isOK(), "Activate", ret.getText() );

    ret = surv.save();
    mRunStandardTestWithError( ret.isOK(), "Save", ret.getText() );

    ret = surv.unmount( false );
    mRunStandardTestWithError( ret.isOK() && !surv.isMounted(), "Unmount",
			       ret.getText() );

    return true;
}


static const char* surveycreatejson_body = R"({
"zDomain":"Time",
"depthInFeet":false,
"xyInFeet":false,
"seismicReferenceDatum":0,
"crs":"ProjectionBased System PROJCS[\"ED_1950_UTM_Zone_31N\",GEOGCS[\"GCS_European_1950\",DATUM[\"D_European_1950\",SPHEROID[\"International_1924\",6378388.0,297.0]],PRIMEM[\"Greenwich\",0.0],UNIT[\"Degree\",0.0174532925199433]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"False_Easting\",500000.0],PARAMETER[\"False_Northing\",0.0],PARAMETER[\"Central_Meridian\",3.0],PARAMETER[\"Scale_Factor\",0.9996],PARAMETER[\"Latitude_Of_Origin\",0.0],UNIT[\"Meter\",1.0],AUTHORITY[\"EPSG\",23031]]",
"firstZ":0,
"lastZ":1.848,
"stepZ":0.004,
"firstInline":100,
"lastInline":750,
"stepInline":1,
"firstCrossline":300,
"lastCrossline":1250,
"stepCrossline":1,
"coord0X":605835.4734984258,
"coord0Y":6073556.3992593428,
"coord1X":629122.5574241461,
"coord1Y":6090463.2016048711,
"coord2X":629576.519655482,
"coord2Y":6074219.9061388588
})";


static bool initJsonTestWorkDir()
{
    FilePath fp( FilePath::getTempDir(), "od_test_tempsurvey_json" );
    jsonworkdir_.set( fp.fullPath() );

    if ( File::exists(jsonworkdir_.buf()) )
	File::removeDir( jsonworkdir_.buf() );

    const bool crdir = File::createDir( jsonworkdir_.buf() );
    mRunStandardTest( crdir, "Create JSON test work directory" );
    return crdir;
}


static BufferString buildSurveyCreateJson()
{
    BufferString json( "{\"dataRoot\":\"" );
    json.add( FilePath(jsonworkdir_).fullPath(FilePath::Unix) )
	.add( "\",\"survey\":\"Test Survey from JSON\"," );
    json.add( surveycreatejson_body + 1 );
    return json;
}


static bool testSurveyAgainstJSON( const OD::JSON::Object& obj )
{
    const BufferString survey( obj.getStringValue(sJSONKey::Survey()) );
    mRunStandardTestWithError( survey == SI().name(), "Survey name",
		       expectValErrMsg(survey.buf(),SI().name().buf()) );

    const BufferString dataroot(
			obj.getFilePath(sJSONKey::DataRoot()).fullPath() );
    const BufferString gotdataroot = SI().diskLocation().basePath();
    mRunStandardTestWithError(
	pathsAreEqual(dataroot.buf(),gotdataroot.buf()), "Data root",
	expectValErrMsg(dataroot.buf(),gotdataroot.buf()) );

    const BufferString zdom( obj.getStringValue(sJSONKey::ZDomain()) );
    const bool exptime = zdom.isEqual( sKey::Time() );
    mRunStandardTest( SI().zIsTime()==exptime, "Z domain" );

    const bool expdepthinft =
			obj.getBoolValue( sJSONKey::DepthInFeet() );
    mRunStandardTest( SI().depthsInFeet()==expdepthinft, "Depth in feet" );

    const bool expxyinft = obj.getBoolValue( sJSONKey::XYInFeet() );
    mRunStandardTest( SI().xyInFeet()==expxyinft, "XY in feet" );

    const double expsrd =
		obj.getDoubleValue( sJSONKey::SeismicRefDatum() );
    mRunStandardTestWithError(
	mIsEqual(expsrd,mCast(double,SI().seismicReferenceDatum()),1e-4),
	"Seismic reference datum",
	expectValErrMsg(expsrd,SI().seismicReferenceDatum()) );

    const StepInterval<float>& zrg = SI().zRange();
    mRunStandardTestWithError(
	mIsEqual(obj.getDoubleValue(sJSONKey::FirstZ()),zrg.start_,1e-6f),
	"First Z", expectValErrMsg(obj.getDoubleValue(sJSONKey::FirstZ()),
				   zrg.start_) );
    mRunStandardTestWithError(
	mIsEqual(obj.getDoubleValue(sJSONKey::LastZ()),zrg.stop_,1e-6f),
	"Last Z", expectValErrMsg(obj.getDoubleValue(sJSONKey::LastZ()),
				   zrg.stop_) );
    mRunStandardTestWithError(
	mIsEqual(obj.getDoubleValue(sJSONKey::StepZ()),zrg.step_,1e-6f),
	"Z step", expectValErrMsg(obj.getDoubleValue(sJSONKey::StepZ()),
				   zrg.step_) );

    const StepInterval<int> inlrg = SI().inlRange();
    mRunStandardTestWithError(
	inlrg.start_==obj.getIntValue(sJSONKey::FirstInl()),
	"First inline",
	expectValErrMsg(obj.getIntValue(sJSONKey::FirstInl()),inlrg.start_));
    mRunStandardTestWithError(
	inlrg.stop_==obj.getIntValue(sJSONKey::LastInl()),
	"Last inline",
	expectValErrMsg(obj.getIntValue(sJSONKey::LastInl()),inlrg.stop_) );
    mRunStandardTestWithError(
	inlrg.step_==obj.getIntValue(sJSONKey::StepInl()),
	"Inline step",
	expectValErrMsg(obj.getIntValue(sJSONKey::StepInl()),inlrg.step_) );

    const StepInterval<int> crlrg = SI().crlRange();
    mRunStandardTestWithError(
	crlrg.start_==obj.getIntValue(sJSONKey::FirstCrl()),
	"First crossline",
	expectValErrMsg(obj.getIntValue(sJSONKey::FirstCrl()),crlrg.start_));
    mRunStandardTestWithError(
	crlrg.stop_==obj.getIntValue(sJSONKey::LastCrl()),
	"Last crossline",
	expectValErrMsg(obj.getIntValue(sJSONKey::LastCrl()),crlrg.stop_) );
    mRunStandardTestWithError(
	crlrg.step_==obj.getIntValue(sJSONKey::StepCrl()),
	"Crossline step",
	expectValErrMsg(obj.getIntValue(sJSONKey::StepCrl()),crlrg.step_) );

    Coord crds[3];
    BinID bids[2];
    int trcnr = 0;
    SI().get3Pts( crds, bids, trcnr );

    for ( int idx=0; idx<3; idx++ )
    {
	const BufferString xkey = coordKey( idx, true );
	const BufferString ykey = coordKey( idx, false );
	const double expx = obj.getDoubleValue( xkey.buf() );
	const double expy = obj.getDoubleValue( ykey.buf() );
	BufferString desc( "Coord " );
	desc.add( idx );
	mRunStandardTestWithError(
	    mIsEqual(expx,crds[idx].x_,1e-2) &&
	    mIsEqual(expy,crds[idx].y_,1e-2),
	    desc.buf(), expectValErrMsg(expx,crds[idx].x_) );
    }

    const BinID startbid( obj.getIntValue(sJSONKey::FirstInl()),
			  obj.getIntValue(sJSONKey::FirstCrl()) );
    const Coord startcrd = SI().transform( startbid );
    mRunStandardTestWithError(
	mIsEqual(obj.getDoubleValue(coordKey(0,true).buf()),
		 startcrd.x_,1e-2) &&
	mIsEqual(obj.getDoubleValue(coordKey(0,false).buf()),
		 startcrd.y_,1e-2),
	"Transform at first inline/crossline",
	expectValErrMsg(
	    obj.getDoubleValue(coordKey(0,true).buf()),
	    startcrd.x_) );

    ConstRefMan<Coords::CoordSystem> crs = SI().getCoordSystem();
    mRunStandardTest( crs && crs->isOK(), "Coordinate system" );
    return true;
}


static bool testEmptyTempSurveyFromJSON( bool managed )
{
    if ( !initJsonTestWorkDir() )
	return false;

    uiRetVal uirv;
    BufferString jsonbuf = buildSurveyCreateJson();
    OD::JSON::Object obj;
    uirv = obj.parseJSon( jsonbuf.getCStr(), jsonbuf.size() );
    mRunStandardTestWithError( uirv.isOK(), "Parse survey create JSON",
			       uirv.getText() );
    if ( !uirv.isOK() )
	return false;

    const BufferString survnm( obj.getStringValue(sJSONKey::Survey()) );
    const BufferString dataroot(
		obj.getFilePath(sJSONKey::DataRoot()).fullPath() );
    EmptyTempSurvey tempsurvey( survnm.buf(), dataroot.buf(), obj,
				false, managed );
    tempsurvey.setManaged( managed );
    mRunStandardTest( tempsurvey.isOK(), "Constructed Survey" );
    if ( !tempsurvey.isOK() )
    {
	tstStream( true ) << tempsurvey.errMsg() << od_endl;
	return false;
    }

    registerJsonSurveyPaths( tempsurvey );
    if ( managed && File::exists(jsonsurvdir_.buf()) )
	File::removeDir( jsonsurvdir_.buf() );

    uiRetVal ret = tempsurvey.mount();
    mRunStandardTestWithError( ret.isOK() && tempsurvey.isMounted(), "Mount",
			       ret.getText() );
    if ( !ret.isOK() )
	return false;

    ret = tempsurvey.activate();
    mRunStandardTestWithError( ret.isOK(), "Activate", ret.getText() );
    if ( !ret.isOK() )
	return false;

    if ( !testSurveyAgainstJSON(obj) )
	return false;

    ret = tempsurvey.unmount( false );
    mRunStandardTestWithError( ret.isOK() && !tempsurvey.isMounted(), "Unmount",
			       ret.getText() );
    if ( !ret.isOK() )
	return false;

    return true;
}


static bool testJsonConstructorSurvey( bool managed )
{
    if ( !initJsonTestWorkDir() )
	return false;

    OD::JSON::Object jsonobj;
    BufferString jsonbuf( surveycreatejson_body );
    const uiRetVal uirv = jsonobj.parseJSon( jsonbuf.getCStr(),
					     jsonbuf.size() );
    mRunStandardTestWithError( uirv.isOK(), "Parse Petrel-style JSON",
			       uirv.getText() );
    if ( uirv.isError() )
	return false;

    mRunStandardTest( jsonobj.isPresent(sJSONKey::ZDomain()),
		      "Petrel-style JSON parsed" );
    mRunStandardTest( !jsonobj.isPresent(sJSONKey::Survey()),
		      "JSON has no survey name" );
    mRunStandardTest( !jsonobj.isPresent(sJSONKey::DataRoot()),
		      "JSON has no data root" );

    const char* survnm = "json_ctor_test";
    EmptyTempSurvey tempsurvey( survnm, jsonworkdir_.buf(), jsonobj,
				false, managed );
    tempsurvey.setManaged( managed );
    mRunStandardTest( tempsurvey.isOK(), "JSON constructor survey" );
    mRunStandardTest( tempsurvey.getSurveyNm()==survnm,
		      "Survey name from constructor" );
    mRunStandardTest( pathsAreEqual(tempsurvey.getTempBaseDir().buf(),
				    jsonworkdir_.buf()),
		      "Data root from constructor" );

    registerJsonSurveyPaths( tempsurvey );
    if ( managed && File::exists(jsonsurvdir_.buf()) )
	File::removeDir( jsonsurvdir_.buf() );

    uiRetVal ret = tempsurvey.mount();
    mRunStandardTestWithError( ret.isOK() && tempsurvey.isMounted(),
			       "Mount JSON constructor survey", ret.getText() );
    if ( !ret.isOK() )
	return false;

    ret = tempsurvey.activate();
    mRunStandardTestWithError( ret.isOK(), "Activate JSON constructor survey",
			       ret.getText() );
    if ( !ret.isOK() )
	return false;

    mRunStandardTest( SI().name()==survnm, "Active survey name" );
    mRunStandardTest( pathsAreEqual(SI().diskLocation().basePath().buf(),
				    jsonworkdir_.buf()),
		      "Active survey data root" );
    mRunStandardTest( SI().diskLocation().dirName()==survnm,
		      "Active survey directory" );

    ret = tempsurvey.unmount( false );
    mRunStandardTestWithError( ret.isOK() && !tempsurvey.isMounted(),
			       "Unmount JSON constructor survey",
			       ret.getText() );

    return ret.isOK();
}


//Main Program
int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    OD::ModDeps().ensureLoaded( "General" );
    OD::ModDeps().ensureLoaded( "Geometry" );

    const bool managed = !clParser().hasKey( "keep" );
    sCleanupAtExit = managed;
    signal( SIGINT, signalHandler );
    signal( SIGTERM, signalHandler );
    atexit( cleanup );

    tstStream() << "--------Temporary Empty Survey-------------" << od_endl;
    EmptyTempSurvey tempsurvey( "Temporary test project" );
    tempsurvey.setManaged( managed );
    registerSurveyPaths( tempsurvey, defsurvbasedir_, defsurvzip_ );
    if ( !testSurvey(tempsurvey) )
    {
	tstStream( true ) << tempsurvey.errMsg() << od_endl;
	return 1;
    }

    tstStream() << "-------Temporary Survey from zip file-------" << od_endl;
    SurveyFile surveyfile( tempsurvey.getZipArchiveLocation() );
    surveyfile.setManaged( managed );
    zipbasedir_.set( surveyfile.getTempBaseDir() );
    if ( !testSurvey(surveyfile) )
    {
	tstStream( true ) << surveyfile.errMsg() << od_endl;
	return 1;
    }

    tstStream() << "----Temporary Empty Survey from JSON---------" << od_endl;
    if ( !testEmptyTempSurveyFromJSON(managed) )
	return 1;

    tstStream() << "----Temporary survey from JSON constructor----" << od_endl;
    if ( !testJsonConstructorSurvey(managed) )
	return 1;

    return 0;
}
