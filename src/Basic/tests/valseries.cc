/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "valseries.h"
#include "valseriesimpl.h"


/*!\brief Regression tests for ValueSeriesGetAll<float>::doWork
  (include/Basic/valseries.h), the frame in the 7.0.10 crash report.

  Test B mirrors the uiattribpartserv.cc caller pattern (nrelements from
  the source column, destination sized independently): pre-fix it
  overruns the destination via the arr()!=null memcpy fast path. The
  destination allocation is oversized so the overrun hits a canary
  instead of crashing the harness. Post-fix (Point 2) execute() must
  fail cleanly with the canary intact.
*/

static bool testMatchingCopy()
{
    const od_int64 n = 1000;
    ArrayValueSeries<float,float> src( n );
    ArrayValueSeries<float,float> dst( n );
    mRunStandardTest( src.isOK() && dst.isOK(),
		      "Matching-size series allocate" );

    for ( od_int64 idx=0; idx<n; idx++ )
	src.setValue( idx, (float)idx * 0.5f + 1.f );

    ValueSeriesGetAll<float> copier( src, dst, n );
    mRunStandardTest( copier.execute(), "Matching-size copy executes" );

    for ( od_int64 idx=0; idx<n; idx++ )
    {
	if ( dst.value(idx) != src.value(idx) )
	{
	    errStream() << "Copy mismatch at " << idx << od_endl;
	    return false;
	}
    }

    logStream() << "[OK] Matching-size ValueSeriesGetAll copy" << od_endl;
    return true;
}


static bool testDstSmallerMustNotOverrun()
{
    const od_int64 nsrc = 1000;
    const od_int64 ndst = 100;
    const float kCanary = -999.f;

    ArrayValueSeries<float,float> src( nsrc );
    mRunStandardTest( src.isOK(), "Oversized-copy source allocates" );
    for ( od_int64 idx=0; idx<nsrc; idx++ )
	src.setValue( idx, (float)idx );

    float* dstalloc = new float[nsrc];
    for ( od_int64 idx=0; idx<ndst; idx++ )
	dstalloc[idx] = -1.f;
    for ( od_int64 idx=ndst; idx<nsrc; idx++ )
	dstalloc[idx] = kCanary;

    ArrayValueSeries<float,float> dst( dstalloc, false, ndst );

    ValueSeriesGetAll<float> copier( src, dst, nsrc );
    const bool res = copier.execute();

    for ( od_int64 idx=ndst; idx<nsrc; idx++ )
    {
	if ( dstalloc[idx] != kCanary )
	{
	    errStream() << "Destination overrun at " << idx
			<< ": canary " << kCanary
			<< " overwritten with " << dstalloc[idx] << od_endl;
	    delete [] dstalloc;
	    return false;
	}
    }

    mRunStandardTest( !res, "Oversized copy fails cleanly "
			    "instead of overrunning" );

    delete [] dstalloc;
    logStream() << "[OK] Oversized ValueSeriesGetAll copy is contained"
		<< od_endl;
    return true;
}


static bool testSrcSmallerMustFail()
{
    const od_int64 nsrc = 100;
    const od_int64 nreq = 1000;

    float* srcalloc = new float[nreq];
    for ( od_int64 idx=0; idx<nsrc; idx++ )
	srcalloc[idx] = (float)idx;
    for ( od_int64 idx=nsrc; idx<nreq; idx++ )
	srcalloc[idx] = -999.f;

    ArrayValueSeries<float,float> src( srcalloc, false, nsrc );
    ArrayValueSeries<float,float> dst( nreq );
    mRunStandardTest( dst.isOK(), "Undersized-source destination allocates" );

    ValueSeriesGetAll<float> copier( src, dst, nreq );
    mRunStandardTest( !copier.execute(),
		      "Copy needing more source values than available fails" );

    delete [] srcalloc;
    logStream() << "[OK] Undersized-source ValueSeriesGetAll copy fails"
		<< od_endl;
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testMatchingCopy() || !testDstSmallerMustNotOverrun() ||
	 !testSrcSmallerMustFail() )
	return 1;

    return 0;
}
