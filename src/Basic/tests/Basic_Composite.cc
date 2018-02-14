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

#undef mTestMainFnName
#define mTestMainFnName test_main_geometry
#include "geometry.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_math2
#include "math2.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_refcount
#include "refcount.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_sets
#include "sets.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_string
#include "string.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_threads
#include "threads.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_threadwork
#include "threadwork.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_trckeyzsampling
#include "trckeyzsampling.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_uistring
#include "uistring.cc"


int testMain( int argc, char** argv )
{
    mInitCompositeTestProg( Basic );

    mRunSubTest( basic );
    mRunSubTest( callback );
    mRunSubTest( commandlineparser );
    mRunSubTest( datapack );
    mRunSubTest( dbkey );
    mRunSubTest( geometry );
    mRunSubTest( math2 );
    mRunSubTest( refcount );
    mRunSubTest( sets );
    mRunSubTest( string );
    mRunSubTest( threads );
    mRunSubTest( threadwork );
    mRunSubTest( trckeyzsampling );
    mRunSubTest( uistring );

    return 0;
}
