/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "file.h"
#include "surveyfile.h"
#include "ptrman.h"
#include "od_istream.h"
#include "ziputils.h"

//Test for creating empty survey
bool tempSurveyCreationIsOK( EmptyTempSurvey& tempsurvey )
{
    mRunStandardTest( tempsurvey.isOK(), "Creating Empty Survey" );
    return true;
}


bool tempSurveyActivate( EmptyTempSurvey& tempsurvey )
{
    uiRetVal ret = tempsurvey.mount();
    mRunStandardTest( !ret.isError(), "Mounting Empty Survey" );
    return true;
}


bool tempSurveyDeactivate( EmptyTempSurvey& tempsurvey )
{
    uiRetVal ret = tempsurvey.unmount();
    mRunStandardTest( !ret.isError(), "UnMounting Empty Survey" );
    return true;
}


bool tempSurveySave( EmptyTempSurvey& tempsurvey )
{
    uiRetVal ret = tempsurvey.save();
    mRunStandardTest( !ret.isError(), "Saving Empty Survey" );
    return true;
}

//Test for Zipping existing survey, unzipping, mouting and unmounting a survey
bool moutingZippedSurvey( SurveyFile& survfile )
{
    uiRetVal ret = survfile.mount();
    mRunStandardTest( !ret.isError(), "Mounting Zipped Survey" );
    return true;
}


bool unMoutingZippedSurvey( SurveyFile& survfile )
{
    uiRetVal ret = survfile.unmount( false );
    mRunStandardTest( !ret.isError(), "UnMounting Zipped Survey" );
    return true;
}


bool savingZippedSurvey( SurveyFile& survfile )
{
    uiRetVal ret = survfile.save();
    mRunStandardTest( !ret.isError(), "Saving Zipped Survey" );
    return true;
}



//Main Program
int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    logStream() << "------------Creating Temporary Survey------------" <<
								    od_endl;
    EmptyTempSurvey tempsurvey;
    if ( !tempSurveyCreationIsOK(tempsurvey) ||
	    !tempSurveyActivate(tempsurvey) ||
		    !tempSurveyDeactivate(tempsurvey) ||
			!tempSurveySave(tempsurvey) )
    {
	errStream() << tempsurvey.errMsg();
	return 1;
    }


    SurveyFile surveyfile( tempsurvey.getZipArchiveLocation() );
    logStream() << "------------UnZipping Temporary Survey------------" <<
								    od_endl;
    if ( !moutingZippedSurvey(surveyfile) ||
	!savingZippedSurvey(surveyfile) || !unMoutingZippedSurvey(surveyfile) )
    {
	errStream() << tempsurvey.errMsg();
	return 1;
    }

    return 0;
}
