/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/


#include "testprog.h"
#include "file.h"
#include "surveydisklocation.h"

#undef mInitTestProg
#define mInitTestProg()


#undef mTestMainFnName
#define mTestMainFnName test_main_callback
#include "callback.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_commandlineparser
#include "commandlineparser.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_cubesampling
#include "cubesampling.cc"

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
#define mTestMainFnName test_main_odjson
#include "odjson.cc"

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
#define mTestMainFnName test_main_survgeom
#include "survgeom.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_threads
#include "threads.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_threadwork
#include "threadwork.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_uistring
#include "uistring.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_various_basic
#include "various_basic.cc"

static void initSI()
{
    BufferString survnm = "F3_Demo_Test";
    SurveyDiskLocation survloc( survnm );
    if ( !File::isDirectory(survloc.fullPath()) )
    {
	BufferString lastsurvfnm = GetLastSurveyFileName();
	od_istream strm( lastsurvfnm );
	if ( strm.isOK() )
	{
	    strm.getLine( survnm );
	    if ( !survnm.isEmpty() )
	        survloc = SurveyDiskLocation( survnm );
	}
    }
    SurveyInfo::setSurveyLocation( survloc, false );
}


int testMain( int argc, char** argv )
{
    mInitCompositeTestProg( Basic );
    initSI();

    mRunSubTest( commandlineparser );
    mRunSubTest( cubesampling );
    mRunSubTest( datapack );
    mRunSubTest( dbkey );
    mRunSubTest( geometry );
    mRunSubTest( math2 );
    mRunSubTest( odjson );
    mRunSubTest( refcount );
    mRunSubTest( sets );
    mRunSubTest( string );
    mRunSubTest( survgeom );
    mRunSubTest( uistring );
    mRunSubTest( various_basic );

    mRunSubTest( threads );
    mRunSubTest( threadwork );
    mRunSubTest( callback );

    return 0;
}
