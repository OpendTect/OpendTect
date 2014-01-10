#include "odmemory.h"
#include "timefun.h"
#include "testprog.h"
#include <string.h>

#define mArrSz 10000000
#define mNrRuns 100


int main( int argc, char** argv )
{
    mInitTestProg();

    float* vals = new float [mArrSz];
    float* copy = new float [mArrSz];
    for ( int idx=0; idx<mArrSz; idx++ )
	vals[idx] = idx * 0.001f;

    int prevms = Time::getMilliSeconds();

    for ( int irun=0; irun<mNrRuns; irun++ )
    {
	for ( int idx=0; idx<mArrSz; idx++ )
	    copy[idx] = vals[idx];
    }

#define mPrTime(s) \
    curms = Time::getMilliSeconds(); \
    od_cout() << s << ": " << curms - prevms << od_endl

    int mPrTime( "Simple loop" );

    prevms = Time::getMilliSeconds();
    for ( int irun=0; irun<mNrRuns; irun++ )
    {
	const float* curptr = vals;
	float* cpptr = copy;
	for ( const float* stopptr = vals + mArrSz; curptr != stopptr;
		curptr++, cpptr++  )
	    *cpptr = *curptr;
    }

    mPrTime( "Ptr loop" );

    prevms = Time::getMilliSeconds();
    for ( int irun=0; irun<mNrRuns; irun++ )
	OD::memCopy( copy, vals, mArrSz * sizeof(float) );

    mPrTime( "OD::memCopy" );

    prevms = Time::getMilliSeconds();
    for ( int irun=0; irun<mNrRuns; irun++ )
	memcpy( copy, vals, mArrSz * sizeof(float) );

    mPrTime( "memcpy" );

    prevms = Time::getMilliSeconds();
    for ( int irun=0; irun<mNrRuns; irun++ )
    {
	MemCopier<float> mcp( copy, vals, mArrSz );
	mcp.execute();
    }

    mPrTime( "MemCopier" );

    return ExitProgram( 0 );
}
