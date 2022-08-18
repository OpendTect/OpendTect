/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"
#include "typeset.h"
#include <iostream>
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
	   "ess_attrib -primaryhost 192.168.4.20 -primaryport 37500 -jobid 0 '/"
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
    const char* multikeystr = "--multi";
    const char* multistr = multikeystr+2;
    const char* sixstr = "6";
    const char* sevenstr = "7";
    const char* multivalkeystr = "--multival";
    const char* multivalstr = multivalkeystr+2;

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
    argv1 += multikeystr;
    argv1 += sixstr;
    argv1 += multikeystr;
    argv1 += fivestr;
    argv1 += sixstr;
    argv1 += multivalkeystr;
    argv1 += fivestr;
    argv1 += sixstr;
    argv1 += sevenstr;


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
    float createfloat = 0.f;
    double createdouble = 0.f;
    BufferString flagfile;
    BufferString valstr;
    BufferStringSet keyvals;
    TypeSet<int> vals;

    mRunStandardTest( testparser.getVal(createstr,createint) && createint==5,
		 "Int value" );
    mRunStandardTest( testparser.getVal(createstr,createfloat) &&
		createfloat==5.f,
		"Float value");
    mRunStandardTest( testparser.getVal(createstr,createdouble) &&
		      createdouble==5., "Double value");
    mRunStandardTest( testparser.getVal("flag", flagfile) && flagfile==file2str,
		"String value");
    mRunStandardTest( testparser.getVal("nonexistingkey", flagfile)==false,
		"Non-existing string" );
    mRunStandardTest( testparser.getVal("nonexistingkey", flagfile, true),
		"Non-existing value - accept none" );
    mRunStandardTest( testparser.getVal(inbgstr, createint)==false,
		"Non existing integer" );
    mRunStandardTest( testparser.getVal("nonexistingkey", keyvals)==false,
		"Non-existing key in multi-key reader" );
    mRunStandardTest( testparser.getVal(createstr, keyvals) &&
		      keyvals.size()==2 && keyvals.get(0)==fivestr,
		"Single key-value read in multi-key reader" );
    mRunStandardTest( testparser.getVal(multistr, keyvals) &&
		      keyvals.size()==3 && keyvals.get(0)==fivestr &&
		      keyvals.get(1)==sixstr && keyvals.get(2)==sixstr,
		"Multiple key-value read in multi-key reader" );
    testparser.getVal(multivalstr, vals);
    mRunStandardTest( testparser.getVal(multivalstr, vals) &&
		      vals.size()==3 && vals[0]==5 &&
		      vals[1]==6 && vals[2]==7,
		"Single key-multivalue read" );

    BufferStringSet normalargs;
    testparser.getNormalArguments(normalargs);
    mRunStandardTest( normalargs.size()==14, "getNormalArguments()" );

    testparser.setKeyHasValue( createstr );
    mRunStandardTest( testparser.isKeyValue(1), "setKeyHasValue" );

    testparser.getNormalArguments(normalargs);
    mRunStandardTest( normalargs.size()==13,
	    "getNormalArguments() with 1 key with value" );

    testparser.setKeyHasValue( multistr );
    testparser.getNormalArguments(normalargs);
    mRunStandardTest( normalargs.size()==11,
	    "getNormalArguments() with multi-key" );

    testparser.setKeyHasValue( multivalstr, 3 );
    testparser.getNormalArguments(normalargs);
    mRunStandardTest( normalargs.size()==8,
	    "getNormalArguments() with multi-value key" );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testNormalOperation(clParser()) || !testStringParsing() )
	return 1;

    return 0;
}
