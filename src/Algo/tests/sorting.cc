/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/


#include "sorting.h"

#include "testprog.h"
#include "statrand.h"
#include "varlenarray.h"
#include "sorting.h"
#include "threadwork.h"

//Check that all values are ascending
template <class T>
bool checkSorting( T* vals, int sz )
{
    for ( int idx=1; idx<sz; idx++ )
    {
	if ( vals[idx-1]>vals[idx] )
	{
	    return false;
	}
    }

    return true;
}

//Sort 1M values using quicksort, and make sure results are correct
static bool test_quicksort()
{
    const int sz = 1024*1024;
    mAllocVarLenArr( int, vals, sz );

    Stats::RandGen gen;
    for ( int idx=0; idx<sz; idx++ )
    {
	vals[idx] = gen.getInt();
    }

    quickSort( mVarLenArr(vals), sz );
    return checkSorting( mVarLenArr(vals), sz );
}

#define mNrTesters 100

//Since quicksort uses static seeds, it can be sensitive to
//parallel loads, hence this check

static bool testParallelQuickSort()
{
    TypeSet<Threads::Work> workload;
    for ( int idx=0; idx<mNrTesters; idx++ )
	workload += Threads::Work( &test_quicksort );

    mRunStandardTest( Threads::WorkManager::twm().addWork( workload ),
		      "parallel quickSort" );

    return true;
}

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    return testParallelQuickSort() ? 0 : 1;
}
