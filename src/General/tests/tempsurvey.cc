/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "surveyfile.h"

#include "file.h"
#include "filepath.h"
#include "moddepmgr.h"
#include "testprog.h"
#include "ziputils.h"

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


//Main Program
int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    OD::ModDeps().ensureLoaded( "General" );

    const bool managed = !clParser().hasKey( "keep" );

    tstStream() << "--------Temporary Empty Survey-------------" << od_endl;
    EmptyTempSurvey tempsurvey( "Temporary test project" );
    tempsurvey.setManaged( managed );
    if ( !testSurvey(tempsurvey) )
    {
	tstStream( true ) << tempsurvey.errMsg() << od_endl;
	return 1;
    }

    tstStream() << "-------Temporary Survey from zip file-------" << od_endl;
    SurveyFile surveyfile( tempsurvey.getZipArchiveLocation() );
    surveyfile.setManaged( managed );
    if ( !testSurvey(surveyfile) )
    {
	tstStream( true ) << surveyfile.errMsg() << od_endl;
	return 1;
    }

    if ( managed )
    {
	File::removeDir( tempsurvey.getTempBaseDir() );
	File::removeDir( surveyfile.getTempBaseDir() );
	FilePath fp( tempsurvey.getZipArchiveLocation() );
	File::remove( fp.fullPath() );
	fp.setExtension( SurveyFile::bckupExtStr() );
	File::remove( fp.fullPath() );
    }

    return 0;
}
