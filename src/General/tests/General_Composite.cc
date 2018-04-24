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
#define mTestMainFnName test_main_geojson
#include "geojson.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_ibmformat
#include "ibmformat.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_mathexpression
#include "mathexpression.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_mathformula
#include "mathformula.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_pickset
#include "pickset.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_posidxpairdataset
#include "posidxpairdataset.cc"

#undef mTestMainFnName
#define mTestMainFnName test_main_posidxpairvalset
#include "posidxpairvalset.cc"


int testMain( int argc, char** argv )
{
    mInitCompositeTestProg( General );

    mRunSubTest( geojson );
#ifndef __win32__
    mRunSubTest( ibmformat );
#endif
    mRunSubTest( mathexpression );
    mRunSubTest( mathformula );
    mRunSubTest( pickset );
    mRunSubTest( posidxpairdataset );
    mRunSubTest( posidxpairvalset );

    return 0;
}
