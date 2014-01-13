#include "odmemory.h"
#include "timefun.h"
#include "testprog.h"
#include "thread.h"

#define mTotNrElems 1000000000

#define mPrTime(s) \
	curms = Time::getMilliSeconds(); \
	od_cout() << od_tab << s << ": " << curms - prevms << od_endl


static bool testSpeed()
{
    if ( quiet )
	return true;

    od_cout() << "\nTotal number of floats: " << mTotNrElems << od_newline
	      << "Processors: " << Threads::getNrProcessors() << od_newline
	      << od_endl;

    for ( int arrsz=100; arrsz<=1000; arrsz+=50 )
    {
	float* vals = new float [arrsz];
	float* copy = new float [arrsz];
	const int nrruns = mTotNrElems / arrsz;
	od_cout() << "Arrsz=" << arrsz << " (" << nrruns << " runs)" << od_endl;

	int prevms = Time::getMilliSeconds();

	for ( int irun=0; irun<nrruns; irun++ )
	{
	    const float* curptr = vals;
	    float* cpptr = copy;
	    for ( const float* stopptr = vals + arrsz; curptr != stopptr;
		    curptr++, cpptr++  )
		*cpptr = *curptr;
	}

	int mPrTime( "PtrLoop" );

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

    return true;
}


static bool testCopySet()
{
    float* vals = new float [mTotNrElems];
    float* copy = new float [mTotNrElems];
    for ( int idx=0; idx<mTotNrElems; idx++ )
	vals[idx] = idx * 0.01f + 1.0f;

    if ( !quiet )
	od_cout() << "Testing copy size: ";

    for ( int arrsz=10; arrsz<=mTotNrElems; arrsz*=10 )
    {
	if ( !quiet )
	    { od_cout() << arrsz << ' '; od_cout().flush(); }

	OD::memCopy( copy, vals, arrsz * sizeof(float) );
	for ( int idx=0; idx<arrsz; idx++ )
	{
	    if ( !isFPEqual( copy[idx], vals[idx], mDefEpsF ) )
	    {
		od_cout() << "OD::memCopy failure: vals[" << idx << "]="
		    << vals[idx] << " but copy[idx]=" << copy[idx] << od_endl;
		return false;
	    }
	}

	OD::memZero( copy, arrsz * sizeof(float) );
	for ( int idx=0; idx<arrsz; idx++ )
	{
	    if ( !isFPZero( copy[idx], mDefEpsF ) )
	    {
		od_cout() << "OD::memZero failure: copy[" << idx << "]="
			    << copy[idx] << od_endl;
		return false;
	    }
	}
    }

    delete [] vals;
    delete [] copy;
    return true;
}


int main( int argc, char** argv )
{
    mInitTestProg();

    /*
    if ( !testCopySet() )
	ExitProgram( 1 );
	*/
    if ( !testSpeed() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}
