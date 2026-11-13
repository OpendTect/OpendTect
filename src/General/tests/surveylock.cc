/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "surveyfile.h"

#include "bufstringset.h"
#include "commandlineparser.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "moddepmgr.h"
#include "oddirs.h"
#include "stringview.h"
#include "survinfo.h"
#include "testprog.h"

#include <csignal>
#include <cstdlib>


static const char* sLockFileName()	{ return ".od_survey.lock"; }
static const char* sMutexFileName()	{ return ".od_survey.lock.mutex"; }

static BufferStringSet trackedsurveydirs;

static BufferString surveyFullPath( const SurveyCreator& surv )
{
    return FilePath( surv.getTempBaseDir(), surv.getSurveyDir() ).fullPath();
}


static BufferString lockFilePath( const char* surveydir )
{
    return FilePath( surveydir, sLockFileName() ).fullPath();
}


static void cleanupLockFiles( const char* surveydir )
{
    if ( StringView(surveydir).isEmpty() )
	return;

    File::remove( lockFilePath(surveydir) );
    File::remove( FilePath(surveydir,sMutexFileName()).fullPath() );
}


static void trackSurveyDir( const char* surveydir )
{
    if ( !StringView(surveydir).isEmpty() )
	trackedsurveydirs.addIfNew( surveydir );
}


static void cleanup()
{
    for ( const auto* dir : trackedsurveydirs )
	cleanupLockFiles( dir->buf() );
}


static void signalHandler( int )
{
    cleanup();
    std::exit( 1 );
}


static BufferString msg( const char* label, const char* txt )
{
    return BufferString( label, ": ", txt );
}


static bool appendForeignHolder( const char* surveydir )
{
    const BufferString lockfnm = lockFilePath( surveydir );
    BufferString content;
    if ( File::exists(lockfnm) && !File::getContent(lockfnm,content) )
	return false;

    if ( !content.isEmpty() && !content.endsWith("\n") )
	content.add( "\n" );

    content.add( "remote-other-host|99999|od_test|foreign-user\n" );
    return File::putContent( content, lockfnm );
}


static bool testSoleHolder( const char* surveydir, const char* label )
{
    mRunStandardTest( File::exists(lockFilePath(surveydir)),
		      msg(label,"Lock file exists after IOM open") )

    mRunStandardTest( !IOM().hasOtherSurveyHolders(surveydir),
		      msg(label,"IOM: no other holders for sole registrant") )

    BufferStringSet infos;
    IOM().getOtherSurveyHolders( surveydir, infos );
    mRunStandardTest( infos.isEmpty(), msg(label,
		      "IOM: getOtherSurveyHolders empty for sole registrant") )

    return true;
}


static bool testForeignHolder( const char* surveydir, const char* label )
{
    mRunStandardTest( appendForeignHolder(surveydir),
		      msg(label,"Append foreign holder to lock file") )

    mRunStandardTest( IOM().hasOtherSurveyHolders(surveydir),
		      msg(label,"IOM: foreign remote holder is detected") )

    BufferStringSet infos;
    IOM().getOtherSurveyHolders( surveydir, infos );
    mRunStandardTest( infos.size()==1,
		      msg(label,"IOM: exactly one other holder reported") )
    mRunStandardTest( infos.validIdx(0) &&
		      infos.get(0).find("remote-other-host"),
		      msg(label,
			  "IOM: other holder info contains foreign hostname") )
    mRunStandardTest( infos.validIdx(0) &&
		      infos.get(0).find("foreign-user"),
		      msg(label,
			  "IOM: other holder info contains foreign user") )

    return true;
}


static bool isUsableSurveyDir( const char* dir )
{
    return File::isDirectory(dir) &&
	   SurveyInfo::isValidSurveyDir(dir).isOK();
}


static BufferString resolveDataRoot()
{
    BufferString basedir;
    clParser().setKeyHasValue( "datadir" );
    clParser().setKeyHasValue( CommandLineParser::sDataRootArg() );
    if ( !clParser().getVal("datadir",basedir) )
	clParser().getVal( CommandLineParser::sDataRootArg(), basedir );

    if ( basedir.isEmpty() )
	basedir = GetBaseDataDir();

    return basedir;
}


static const char* sTestSurveyNames[] = {
	"F3_Test_Survey",
	"F3_Test_Survey_DisplayFT",
	"F3_Test_Survey_DepthFT__XYinft_",
	"F3_Test_Survey_DepthM",
	nullptr
};


static bool collectSurveyDirs( const char* basedir, BufferStringSet& dirs )
{
    dirs.setEmpty();
    mRunStandardTest( !StringView(basedir).isEmpty() &&
		      File::isDirectory(basedir), "Data root exists" )
    if ( StringView(basedir).isEmpty() || !File::isDirectory(basedir) )
	return false;

    for ( int idx=0; sTestSurveyNames[idx]; idx++ )
    {
	const BufferString fp =
		FilePath( basedir, sTestSurveyNames[idx] ).fullPath();
	mRunStandardTest( isUsableSurveyDir(fp),
			  BufferString("Usable survey: ",
				       sTestSurveyNames[idx]) )
	if ( !isUsableSurveyDir(fp) )
	    return false;

	dirs.add( fp );
    }

    return true;
}


static bool testOnTempSurvey()
{
    EmptyTempSurvey tempsurvey( "SurveyLock IOM test" );
    tempsurvey.setManaged( true );
    mRunStandardTest( tempsurvey.isOK(), "Construct EmptyTempSurvey" )
    if ( !tempsurvey.isOK() )
    {
	tstStream( true ) << tempsurvey.errMsg() << od_endl;
	return false;
    }

    uiRetVal ret = tempsurvey.mount();
    mRunStandardTestWithError( ret.isOK() && tempsurvey.isMounted(),
			       "Mount temp survey", ret.getText() )
    if ( !ret.isOK() )
	return false;

    const BufferString surveydir = surveyFullPath( tempsurvey );
    trackSurveyDir( surveydir );
    cleanupLockFiles( surveydir.buf() );

    ret = tempsurvey.activate();
    mRunStandardTestWithError( ret.isOK() && IOMan::isOK(),
			       "Activate temp survey (IOM registers lock)",
			       ret.getText() )
    if ( !ret.isOK() )
	return false;

    if ( !testSoleHolder(surveydir.buf(),"Temp") )
	return false;

    if ( !testForeignHolder(surveydir.buf(),"Temp") )
	return false;

    ret = tempsurvey.unmount( false );
    mRunStandardTestWithError( ret.isOK(), "Unmount temp survey",
			       ret.getText() )

    cleanupLockFiles( surveydir.buf() );
    return ret.isOK();
}


static bool testOnRealSurveys( const char* basedir )
{
    BufferStringSet surveydirs;
    if ( !collectSurveyDirs(basedir,surveydirs) )
	return false;

    for ( const auto* dir : surveydirs )
    {
	trackSurveyDir( dir->buf() );
	cleanupLockFiles( dir->buf() );
    }

    const BufferString& surv_a = surveydirs.get( 0 );
    const BufferString& surv_b = surveydirs.get( 1 );
    const FilePath fp_a( surv_a.buf() );
    const BufferString basedatadir( fp_a.pathOnly() );
    const BufferString survdirnm_a( fp_a.fileName() );
    const BufferString survdirnm_b( FilePath(surv_b).fileName() );

    uiRetVal uirv = IOMan::setDataSource( basedatadir.buf(),
					  survdirnm_a.buf() );
    mRunStandardTestWithError( uirv.isOK(),
			       "Real: IOM setDataSource survey A",
			       toString(uirv) )
    if ( !uirv.isOK() )
	return false;

    if ( !testSoleHolder(surv_a.buf(),"Real") )
	return false;

    if ( !testForeignHolder(surv_a.buf(),"Real") )
	return false;

    IOMan::newSurvey( nullptr );
    cleanupLockFiles( surv_a.buf() );

    uirv = IOMan::setDataSource( basedatadir.buf(), survdirnm_a.buf() );
    mRunStandardTestWithError( uirv.isOK(),
			       "Real: re-open survey A before switch",
			       toString(uirv) )
    if ( !uirv.isOK() )
	return false;

    mRunStandardTest( File::exists(lockFilePath(surv_a)),
		      "Real: lock present on survey A" )

    uirv = IOMan::setDataSource( basedatadir.buf(), survdirnm_b.buf() );
    mRunStandardTestWithError( uirv.isOK(),
			       "Real: IOM switch to survey B",
			       toString(uirv) )
    if ( !uirv.isOK() )
	return false;

    mRunStandardTest( !File::exists(lockFilePath(surv_a)),
		      "Real: lock removed from survey A after switch" )
    mRunStandardTest( File::exists(lockFilePath(surv_b)),
		      "Real: lock present on survey B after switch" )

    IOMan::newSurvey( nullptr );

    for ( const auto* dir : surveydirs )
	cleanupLockFiles( dir->buf() );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProgDR();

    OD::ModDeps().ensureLoaded( "General" );
    OD::ModDeps().ensureLoaded( "Geometry" );

    signal( SIGINT, signalHandler );
    signal( SIGTERM, signalHandler );
    atexit( cleanup );

    if ( !testOnTempSurvey() )
	return 1;

    const BufferString basedir = resolveDataRoot();
    return testOnRealSurveys( basedir.buf() ) ? 0 : 1;
}
