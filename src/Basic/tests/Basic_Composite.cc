/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/


#include "testprog.h"
#undef mInitTestProg
#define mInitTestProg()


#undef mTestMainFnName
#define mTestMainFnName test_main_basic
#include "basic.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_callback
#include "callback.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_commandlineparser
#include "commandlineparser.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_datapack
#include "datapack.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_dbkey
#include "dbkey.cc"



#define mRunSubTest( nm ) \
    tstStream() << "\n\n\n->" << #nm << " subtest\n\n"; \
    status = test_main_##nm( argc, argv ); \
    if ( status != 0 ) \
	return status

int testMain( int argc, char** argv )
{
    mTestProgInits();

    tstStream() << "** 'Basic' composite test\n\n";

    int status;
    mRunSubTest( basic );
    mRunSubTest( callback );
    mRunSubTest( commandlineparser );
    mRunSubTest( datapack );
    mRunSubTest( dbkey );
    //TODO add more suitable subtests

    return 0;
}
