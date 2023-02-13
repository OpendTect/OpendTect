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
bool testTempSurvey( EmptyTempSurvey& tempsurvey )
{
    mRunStandardTest( tempsurvey.isOK(), "Creating Empty Survey" );

    uiRetVal ret = tempsurvey.mount();
    mRunStandardTest( !ret.isError(), "Mounting Empty Survey" );

    ret = tempsurvey.save();
    mRunStandardTest( !ret.isError(), "Saving Empty Survey" );

    ret = tempsurvey.unmount();
    mRunStandardTest( !ret.isError(), "UnMounting Empty Survey" );

    return true;
}

//Test for Zipping existing survey, unzipping, mouting and unmounting a survey
bool testZipSurvey( SurveyFile& survfile )
{
    uiRetVal ret = survfile.mount();
    mRunStandardTest( !ret.isError(), "Mounting Zipped Survey" );

    ret = survfile.save();
    mRunStandardTest( !ret.isError(), "Saving Zipped Survey" );

    ret = survfile.unmount( false );
    mRunStandardTest( !ret.isError(), "UnMounting Zipped Survey" );

    return true;
}



//Main Program
int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    logStream() << "------------Creating Temporary Survey------------" <<
								    od_endl;
    EmptyTempSurvey tempsurvey;
    if ( !testTempSurvey(tempsurvey) )
    {
	errStream() << tempsurvey.errMsg();
	return 1;
    }


    SurveyFile surveyfile( tempsurvey.getZipArchiveLocation() );
    logStream() << "------------UnZipping Temporary Survey------------" <<
								    od_endl;
    if ( !testZipSurvey(surveyfile) )
    {
	errStream() << surveyfile.errMsg();
	return 1;
    }

    return 0;
}
