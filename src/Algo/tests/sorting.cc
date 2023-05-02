/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "sorting.h"

#include "arrayndalgo.h"
#include "file.h"
#include "filepath.h"
#include "sorting.h"
#include "statrand.h"
#include "statparallelcalc.h"
#include "testprog.h"
#include "timefun.h"
#include "threadwork.h"
#include "varlenarray.h"

#include <limits>

static bool secondpass_ = false;
static BufferString logfilenm_;

#define mTic() \
{ \
    if ( dotime ) \
	counter.start(); \
}
# define mTac() \
{\
    if ( dotime ) \
    { \
	elapsed = counter.elapsed(); \
	ostrm << "Drawn in: " << elapsed << "ms\n"; \
    } \
}

#define mTestValD( expval, res, useeps, msg ) \
{ \
    if ( dodiff ) \
    { \
	ostrm << msg << "; Absolute difference: " ; \
	ostrm << toStringPrecise(Math::Abs(expval-res)) << od_newline;\
	if ( !mIsZero(expval,1e-6f) ) \
	{ \
	    ostrm << msg << "; Relative difference: " ; \
	    ostrm << toStringPrecise(Math::Abs((expval-res)/expval)); \
	    ostrm << od_newline; \
	} \
    } \
    else \
    {  \
	const bool isok = mIsEqual(res,expval,useeps); \
	if ( isok ) \
	    ostrm << "[OK] " << msg << od_newline; \
	else \
	{ \
	    ostrm << msg << ": Expected: " << toStringPrecise(expval); \
	    ostrm << " - got: " << toStringPrecise(res) << od_newline; \
	    if ( !secondpass_ ) \
		return false; \
	} \
    } \
}
#define mTestValI( expval, res, useeps, msg ) \
{ \
    if ( dodiff ) \
    { \
	ostrm << msg << "; Absolute difference: " ; \
	ostrm << toStringPrecise(Math::Abs(expval-res)) << od_newline;\
    } \
    else \
    {  \
	const bool isok = mIsEqual(res,expval,useeps); \
	if ( isok ) \
	    ostrm << "[OK] " << msg << od_newline; \
	else \
	{ \
	    ostrm << msg << ": Expected: " << expval; \
	    ostrm << " - got: " << res << od_newline; \
	    if ( !secondpass_ ) \
		return false; \
	} \
    } \
}

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


static bool testRandom( bool dodiff, bool dotime )
{
    PtrMan<od_ostream> strm;
    if ( quiet_ )
    {
	const bool doappend = !logfilenm_.isEmpty();
	if ( !doappend )
	    logfilenm_ = FilePath::getTempFullPath( "test_sorting_log", "txt" );
	strm = new od_ostream( logfilenm_, doappend );
	if ( doappend )
	    strm->setWritePosition( 0, od_stream::End );
    }

    od_ostream& ostrm = strm && strm->isOK() ? *strm.ptr() : tstStream();

    static double epsavg = 1e-3;
    static double epsstd = 1e-3;
    static int sz = 1024*1024*20; //1sec runtime with eps precision
    Array1DImpl<double> dvalsarr( sz );
    Array1DImpl<float> fvalsarr( sz );
    Array1DImpl<od_int64> llvalsarr( sz );
    Array1DImpl<int> ivalsarr( sz );
    double* dvals = dvalsarr.getData();
    float* fvals = fvalsarr.getData();
    od_int64* llvals = llvalsarr.getData();
    int* ivals = ivalsarr.getData();
    mRunStandardTest( dvals && fvals && llvals && ivals, "Has pointers" );

    static double oneoversqrt12 = Math::Sqrt(1./12.);
    Stats::RandGen urg;
    Time::Counter counter; int elapsed = 0;

    Stats::CalcSetup avgstdsu, minmaxsu;
    avgstdsu.require( Stats::Average ).require( Stats::StdDev );
    minmaxsu.require( Stats::Min ).require( Stats::Max );
    Stats::CalcSetup allsu( avgstdsu );
    allsu.require( Stats::Min ).require( Stats::Max );

    Stats::RunCalc<long double> rcld( allsu );
    mTic(); for ( int idx=0; idx<sz; idx++ )
	dvals[idx] = urg.get();
    mTac();

    rcld.clear();
    for ( int idx=0; idx<sz; idx++ )
	rcld.addValue( dvals[idx] );

    double expavg = 0.5;
    double expstddev = oneoversqrt12;
    mTestValD( expavg, double(rcld.average()), epsavg, "Uniform average" );
    mTestValD( expstddev, double(rcld.stdDev()), epsstd, "Uniform stddev" );
    mRunStandardTest( rcld.min()>=0., "Uniform minimum above 0" );
    mRunStandardTest( rcld.max()<=1., "Uniform maximum below 1" );

    Stats::RunCalc<double> rcd( avgstdsu );
    Stats::RunCalc<int> rci( minmaxsu );
    Interval<int> valrg( std::numeric_limits<int>::min(),
			 std::numeric_limits<int>::max() );
    mTic(); for ( int idx=0; idx<sz; idx++ )
	ivals[idx] = urg.getInt();
    mTac();

    rcd.clear(); rci.clear();
    for ( int idx=0; idx<sz; idx++ )
    {
	rcd.addValue( ivals[idx] );
	rci.addValue( ivals[idx] );
    }

    expstddev = ( double(valrg.stop) - double(valrg.start) ) * oneoversqrt12;
    mRunStandardTest( rci.min() >= valrg.start && rci.max() <= valrg.stop,
		      "Limits of get int" );
    mTestValI( 0, rcd.average(), expstddev, "Uniform average of int" );
    mTestValI( expstddev, rcd.stdDev(), expstddev*1e-2,
	      "Uniform stddev of int" );

    valrg.set( 20, 300 );
    mTic(); for ( int idx=0; idx<sz; idx++ )
	ivals[idx] = urg.getInt( valrg.start, valrg.stop );
    mTac();

    rcd.clear(); rci.clear();
    for ( int idx=0; idx<sz; idx++ )
    {
	rcd.addValue( ivals[idx] );
	rci.addValue( ivals[idx] );
    }
    mRunStandardTest( rci.min() >= valrg.start && rci.max() <= valrg.stop,
		      "Limits of get int range" );
    expavg = valrg.center();
    expstddev = valrg.width() * oneoversqrt12;
    mTestValI( valrg.center(), rcd.average(), expstddev*1e-2,
	      "Uniform avg of int range" );
    mTestValI( expstddev, rcd.stdDev(), expstddev*1e-2,
	      "Uniform stddev of int range" );

    valrg.start = 0;
    mTic(); for ( int idx=0; idx<sz; idx++ )
	ivals[idx] = urg.getIndex( valrg.stop );
    mTac();

    rcd.clear(); rci.clear();
    for ( int idx=0; idx<sz; idx++ )
    {
	rcd.addValue( ivals[idx] );
	rci.addValue( ivals[idx] );
    }
    mRunStandardTest( rci.min() >= valrg.start && rci.max() <= valrg.stop,
		      "Limits of get int index" );
    expavg = valrg.center();
    expstddev = valrg.width() * oneoversqrt12;
    mTestValI( valrg.center(), rcd.average(), expstddev*1e-2,
	      "Uniform avg of int index" );
    mTestValI( expstddev, rcd.stdDev(), expstddev*1e-2,
	      "Uniform stddev of int index" );

    Stats::RunCalc<od_int64> rcll( minmaxsu );
    const Interval<od_int64> lvalrg( 0, mDef32GB );
    expavg = lvalrg.center();
    expstddev = lvalrg.width() * oneoversqrt12;
    mTic(); for ( int idx=0; idx<sz; idx++ )
	llvals[idx] = urg.getIndex( lvalrg.stop );
    mTac();

    rcd.clear(); rcll.clear();
    for ( int idx=0; idx<sz; idx++ )
    {
	rcd.addValue( llvals[idx] );
	rcll.addValue( llvals[idx] );
    }
    mRunStandardTest( rcll.min() >= lvalrg.start && rcll.max() <= lvalrg.stop,
		      "Limits of get od_int64 index" );
    mTestValI( lvalrg.center(), rcd.average(), expstddev*1e-2,
	      "Uniform average of od_int64 index" );
    mTestValI( expstddev, rcd.stdDev(), expstddev*1e-2,
	      "Uniform stddev of od_int64 index" );

    Stats::NormalRandGen rg;
    expavg = 0.;
    expstddev = 1.;
    rcld = Stats::RunCalc<long double>( avgstdsu );
    mTic(); for ( int idx=0; idx<sz; idx++ )
	dvals[idx] = rg.get();
    mTac();

    rcld.clear();
    for ( int idx=0; idx<sz; idx++ )
	rcld.addValue( dvals[idx] );
    mTestValD( expavg, double(rcld.average()),epsstd, "Gaussian average - get");
    mTestValD( expstddev, double(rcld.stdDev()), epsstd,
	       "Gaussian stddev - get" );

    expavg = 0.135;
    expstddev = 0.0356;
    mTic(); for ( int idx=0; idx<sz; idx++ )
	dvals[idx] = rg.get(expavg,expstddev);
    mTac();

    rcld.clear();
    for ( int idx=0; idx<sz; idx++ )
	rcld.addValue( dvals[idx] );
    Stats::ParallelCalc<double> rcpcd( allsu, dvals, sz );
    rcpcd.execute();

    ArrayMath::ArrayOperExecSetup avgsetup;
    avgsetup.donormalizesum_ = true;
    ArrayMath::CumArrOperExec<long double,double> avgexec( dvalsarr, true,
							   avgsetup );
    avgexec.execute();
    const long double stddevd =
	    ArrayMath::getStdDev<long double,double>( dvalsarr, true, true );

    mTestValD( expavg, double(rcld.average()), epsavg,
	       "Gaussian average - RunCalc - double" );
    mTestValD( expavg, rcpcd.average(), epsavg,
	       "Gaussian average - ParallelCalc - double" );
    mTestValD( expavg, double(avgexec.getSum()), epsavg,
	       "Gaussian average - ArrayMath - double" );
    mTestValD( expstddev, double(rcld.stdDev()),epsstd,
	       "Gaussian stddev - RunCalc - double" );
    mTestValD( expstddev, rcpcd.stdDev(), epsstd,
	       "Gaussian stddev - ParallelCalc - double" );
    mTestValD( expstddev, double(stddevd), epsstd,
	       "Gaussian stddev - ArrayMath - double" );

    const float expavgf = float(expavg);
    const float expstddevf = float(expstddev);
    mTic(); for ( int idx=0; idx<sz; idx++ )
	fvals[idx] = rg.get(expavgf,expstddevf);
    mTac();

    rcd.clear();
    for ( int idx=0; idx<sz; idx++ )
	rcd.addValue( fvals[idx] );
    Stats::ParallelCalc<float> rcpcf( allsu, fvals, sz );
    rcpcf.execute();

    ArrayMath::CumArrOperExec<double,float> avgexecf( fvalsarr, true, avgsetup);
    avgexecf.execute();
    const double stddevf = ArrayMath::getStdDev<double,float>( fvalsarr, true,
							       true );

    mTestValD( expavgf, float(rcd.average()), epsavg,
	       "Gaussian average - RunCalc - float" );
    mTestValD( expavgf, rcpcf.average(), epsavg,
	       "Gaussian average - ParallelCalc - float" );
    mTestValD( expavgf, float(avgexecf.getSum()), epsavg,
	       "Gaussian average - ArrayMath - float" );
    mTestValD( expstddevf, float(rcd.stdDev()), epsstd,
	       "Gaussian stddev - RunCalc - float");
    mTestValD( expstddevf, rcpcf.stdDev(), epsstd,
	       "Gaussian stddev - ParallelCalc - float" );
    mTestValD( expstddevf, float(stddevf), epsstd,
	       "Gaussian stddev - ArrayMath - float" );

    if ( strm )
    {
	strm->close();
	strm = nullptr;
	if ( !secondpass_ )
	    File::remove( logfilenm_ );
    }

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testParallelQuickSort() )
	return 1;

    const bool printdiff = clParser().hasKey( "dodiff" );
    const bool printtime = clParser().hasKey( "dotime" );
    const bool firstres = testRandom( printdiff, printtime );
    if ( !firstres )
    {
	secondpass_ = true;
	testRandom( true, false );
    }

    return firstres ? 0 : 1;
}
