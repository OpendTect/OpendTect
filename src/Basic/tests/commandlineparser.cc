/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2013
 * FUNCTION :
-*/


#include "testprog.h"

#include "typeset.h"

bool testStringParsing()
{
    {
	const char* string =
	    "\"/dsk/d20/kristofer tingdahl/dev/od/bin/lux64/Debug/od_batch_la"
	    "uncher\" "
	    "od_process_attrib \"/d20/kristofer/ODData/F3_Demo_2012/Proc/Ener"
	    "gy.par\" --needmonitor --monitorfnm \"/d20/kristofer/ODData/F3_D"
	    "emo_2012/Proc/Energy_log.txt\"";

	CommandLineParser parser( string );
	mRunStandardTest( parser.nrArgs()==5,
		"Number of arguments (double quotes)" );
    }
    {
	const char* string =
	    "'/dsk/d20/raman singh/dev/od-git/bin/od_exec'  --nice 19 --inbg "
	    "od_proc"
	    "ess_attrib -masterhost 192.168.4.20 -masterport 37500 -jobid 0 '/"
	    "d20/raman/surveys/F3_Demo_2012/Proc/dgbindia20_3706_1/dgbindia20_"
	    "0.par'";

	CommandLineParser parser( string );
	mRunStandardTest( parser.nrArgs()==11,
		"Number of arguments (single quotes)" );
    }

    return true;
}


bool testNormalOperation( const CommandLineParser& clparser )
{
    const char* createkeystr = "--create";
    const char* createstr = createkeystr+2;
    const char* inbgkeystr = "--bg";
    const char* inbgstr = inbgkeystr+2;
    const char* file1str = "file1.cc";
    const char* file2str = "file2.cc";
    const char* file3str = "file3.cc";
    const char* dummystr = "dummy";
    const char* nonestr = "none";
    const char* fivestr = "5";

    TypeSet<const char*> argv1;
    argv1 += clparser.getExecutable();
    argv1 += createkeystr;
    argv1 += fivestr;
    argv1 += file1str;
    argv1 += "-flag";
    argv1 += file2str;
    argv1 += file3str;
    argv1 += dummystr; //not a key
    argv1 += "-5"; //not a key
    argv1 += "-.5"; //not a key
    argv1 += inbgkeystr;
    argv1 += "not_a_number";

    CommandLineParser testparser( argv1.size(), (char**)argv1.arr() );

    mRunStandardTest( testparser.getExecutable()==argv1[0],
	    "Correct executable" );
    mRunStandardTest( testparser.nrArgs()==argv1.size()-1,
	    "Number of arguments" );
    mRunStandardTest( testparser.isPresent(dummystr),
	    "Is present (positive)" );
    mRunStandardTest( testparser.isPresent(nonestr)==false,
	    "Is present (negative)" );
    mRunStandardTest( testparser.getArg(3)==argv1[4], "Intermediate argument" );
    mRunStandardTest( testparser.lastArg()==argv1.last(), "Last argument" );
    mRunStandardTest( testparser.hasKey(dummystr)==false,"Has key (negative)" );
    mRunStandardTest( testparser.hasKey("5")==false, "Has key (negative)" );
    mRunStandardTest( testparser.isKey(0), "isKey (positive)" );
    mRunStandardTest( testparser.isKey(1)==false, "isKey (negative)" );
    mRunStandardTest( testparser.isKey(3), "isKey (positive)");
    mRunStandardTest( testparser.isKeyValue(0)==false, "Is keyvalue 0" );
    mRunStandardTest( testparser.isKeyValue(1)==false, "Is keyvalue 1" );
    mRunStandardTest( testparser.isKeyValue(2)==false, "Is keyvalue 2" );
    mRunStandardTest( testparser.isKeyValue(3)==false, "Is keyvalue 3" );

    int createint;
    float createfloat;
    double createdouble;
    BufferString flagfile;

    testparser.setKeyHasValue( createstr );
    mRunStandardTest( testparser.getKeyedInfo(createstr,createint)
			&& createint==5, "Int value" );
    mRunStandardTest( testparser.getKeyedInfo(createstr,createfloat) &&
		createfloat==5, "Float value");
    mRunStandardTest( testparser.getKeyedInfo(createstr,createdouble) &&
		      createdouble==5, "Double value");
    testparser.setKeyHasValue( "flag" );
    mRunStandardTest( testparser.getKeyedInfo("flag", flagfile)
			&& flagfile==file2str, "String value" );
    mRunStandardTest( testparser.getKeyedInfo("nonexistingkey",
		      flagfile)==false, "Non-existing string" );
    mRunStandardTest( testparser.getKeyedInfo("nonexistingkey", flagfile, true),
			"Non-existing value - accept none" );
    const bool docrash = DBG::setCrashOnProgError( false );
    mRunStandardTest( testparser.getKeyedInfo(inbgstr, createint)==false,
		"Non existing integer. Should issue a PE message." );
    DBG::setCrashOnProgError( docrash );

    BufferStringSet normalargs;
    testparser.getNormalArguments( normalargs );
    mRunStandardTest( normalargs.size()==6, "getNormalArguments()" );

    mRunStandardTest( testparser.isKeyValue(1), "setKeyHasValue" );

    const CommandLineParser testparsercopy( argv1.size(), (char**)argv1.arr() );
    BufferStringSet arglist = testparsercopy.keyedList( createstr );
    mRunStandardTest( arglist.size() == 2, "keyedList size" );
    mRunStandardTest( arglist.get(1) == file1str, "keyedList last value" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testNormalOperation(clParser()) || !testStringParsing() )
	return 1;

    return 0;
}
