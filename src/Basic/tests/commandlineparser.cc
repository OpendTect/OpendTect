/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2013
 * FUNCTION : 
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "commandlineparser.h"
#include "typeset.h"
#include "keystrs.h"

#include <iostream>

#define mRunTest( test ) \
if ( !(testparser.test) ) \
{ \
    std::cerr << "Test " << #test << " FAILED\n"; \
    return 1; \
} \
else if ( !quiet ) \
{ \
    std::cerr << "Test " << #test << " - SUCCESS\n"; \
}
    


int main( int narg, char** argv )
{
    SetProgramArgs( narg, argv );
    CommandLineParser parser;
    
    const bool quiet = parser.hasKey( sKey::quiet() );
    
    const char* createkeystr = "--create";
    const char* createstr = createkeystr+2;
    const char* file1str = "file1.cc";
    const char* file2str = "file2.cc";
    const char* file3str = "file3.cc";
    const char* dummystr = "dummy";
    const char* nonestr = "none";
    const char* fivestr = "5";
    
    TypeSet<const char*> argv1;
    argv1 += parser.getExecutable();
    argv1 += createkeystr;
    argv1 += fivestr;
    argv1 += file1str;
    argv1 += "-flag";
    argv1 += file2str;
    argv1 += file3str;
    argv1 += dummystr; //not a key
    argv1 += "-5"; //not a key
    argv1 += "-.5"; //not a key
    
    CommandLineParser testparser( argv1.size(), (char**) argv1.arr() );
    
    mRunTest( getExecutable()==argv1[0] );
    mRunTest( nrArgs()==argv1.size()-1 );
    mRunTest( isPresent(dummystr) );
    mRunTest( isPresent(nonestr)==false );
    mRunTest( getArg(3)==argv1[4] );
    mRunTest( lastArg()==argv1.last() );
    mRunTest( hasKey(dummystr)==false );
    mRunTest( hasKey("5")==false );
    mRunTest( isKey(0) );
    mRunTest( isKey(1)==false );
    mRunTest( isKey(3) );
    mRunTest( isKeyValue(0)==false );
    mRunTest( isKeyValue(1)==false );
    mRunTest( isKeyValue(2)==false );
    mRunTest( isKeyValue(3)==false );
    
    int createint;
    float createfloat;
    double createdouble;
    BufferString flagfile;
    
    mRunTest( getVal(createstr,createint) && createint==5 );
    mRunTest( getVal(createstr,createfloat) && createfloat==5 );
    mRunTest( getVal(createstr,createdouble) && createdouble==5 );
    mRunTest( getVal("flag", flagfile) && flagfile==file2str );

    BufferStringSet normalargs;
    testparser.getNormalArguments(normalargs);
    if ( normalargs.size()!=7 )
    {
	std::cerr << "getNormalArguments() - FAILED\n";
	return 1;
    }
    else if ( !quiet )
    {
	std::cerr << "getNormalArguments() - SUCCESS\n";
    }
    
    testparser.setKeyHasValue( createstr );
    mRunTest( isKeyValue(1) );
    
    testparser.getNormalArguments(normalargs);
    if ( normalargs.size()!=6 )
    {
	std::cerr << "getNormalArguments() with 1 key with value - FAILED\n";
	return 1;
    }
    else if ( !quiet )
    {
	std::cerr << "getNormalArguments() with 1 key with value - SUCCESS\n";
    }

    return 0;
}
