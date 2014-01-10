#include "odmemory.h"
#include "timefun.h"
#include "testprog.h"
#include "thread.h"

#define mTotNrElems 1000000000

#define mPrTime(s) \
	curms = Time::getMilliSeconds(); \
	od_cout() << od_tab << s << ": " << curms - prevms << od_endl


int main( int argc, char** argv )
{
    mInitTestProg();

    od_cout() << "\nTotal number of floats: " << mTotNrElems << od_newline
	      << "Processors: " << Threads::getNrProcessors() << od_newline
	      << od_endl;

    for ( int arrsz=10; arrsz<=1000000000; arrsz*=10 )
    {
	float* vals = new float [arrsz];
	float* copy = new float [arrsz];
	const int nrruns = mTotNrElems / arrsz;
	od_cout() << "Arrsz=" << arrsz << " (" << nrruns << " runs)" << od_endl;

	int prevms = Time::getMilliSeconds();

	for ( int irun=0; irun<nrruns; irun++ )
	{
	    for ( int idx=0; idx<arrsz; idx++ )
		copy[idx] = vals[idx];
	}

	int mPrTime( "SimpleLoop" );

	prevms = Time::getMilliSeconds();
	for ( int irun=0; irun<nrruns; irun++ )
	{
	    const float* curptr = vals;
	    float* cpptr = copy;
	    for ( const float* stopptr = vals + arrsz; curptr != stopptr;
		    curptr++, cpptr++  )
		*cpptr = *curptr;
	}

	mPrTime( "PtrLoop" );

	prevms = Time::getMilliSeconds();
	for ( int irun=0; irun<nrruns; irun++ )
	    OD::memCopy( copy, vals, arrsz * sizeof(float) );

	mPrTime( "OD::memCopy" );

	prevms = Time::getMilliSeconds();
	for ( int irun=0; irun<nrruns; irun++ )
	{
	    MemCopier<float> mcp( copy, vals, arrsz );
	    mcp.execute();
	}

	mPrTime( "MemCopier" );

	delete [] vals;
	delete [] copy;
    }

    return ExitProgram( 0 );
}
