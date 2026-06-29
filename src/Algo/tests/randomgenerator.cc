/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "arrayndalgo.h"
#include "file.h"
#include "filepath.h"
#include "statrand.h"
#include "statparallelcalc.h"
#include "timefun.h"

#include <limits>


static bool secondpass_			= false;
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


constexpr int cNrDraws			= 20;
constexpr int cNrReproSeq		= 20;
constexpr int cNrShortSeq		= 10;
constexpr int cNrApiDraws		= 100;

constexpr int cUrgSeed			= 12345;
constexpr int cNrgSeed			= 54321;
constexpr int cUrgSeedDrawTwice		= 424242;
constexpr int cNrgSeedDrawTwice		= 131313;
constexpr int cSeedChangeA		= 42;
constexpr int cSeedChangeB		= 99;
constexpr int cBaseInterfaceSeed	= 77;
constexpr int cUniformApiSeed		= 31415;
constexpr int cNormalApiSeed		= 27182;

constexpr int cInvalidSeed		= 0;
constexpr int cGetIndexSz0		= 0;
constexpr int cGetIndexSz1		= 1;

constexpr int cIntRangeMin		= 20;
constexpr int cIntRangeMax		= 30;
constexpr int cIntEqualBound		= 7;

constexpr double cNormalExpect		= 3.5;
constexpr double cNormalStdev		= 0.25;


static void drawUniformSet( Stats::RandGen& gen, double* vals, int nr )
{
    for ( int idx=0; idx<nr; idx++ )
	vals[idx] = gen.get();
}


static void drawNormalSet( Stats::NormalRandGen& gen, double* vals, int nr )
{
    for ( int idx=0; idx<nr; idx++ )
	vals[idx] = gen.get();
}


static bool sameSet( const double* v1, const double* v2, int nr )
{
    for ( int idx=0; idx<nr; idx++ )
    {
	if ( v1[idx] != v2[idx] )
	    return false;
    }

    return true;
}


static bool compareUniformSequences( Stats::RandGen& g1, Stats::RandGen& g2,
				     int nr )
{
    for ( int idx=0; idx<nr; idx++ )
    {
	if ( g1.get() != g2.get() )
	    return false;

	if ( g1.getInt() != g2.getInt() )
	    return false;
    }

    return true;
}


static bool compareNormalSequences( Stats::NormalRandGen& g1,
				    Stats::NormalRandGen& g2, int nr )
{
    for ( int idx=0; idx<nr; idx++ )
    {
	if ( g1.get() != g2.get() )
	    return false;
    }

    return true;
}


static bool testSetSeedRejectsZeroOrUdf()
{
    Stats::RandGen gen, expected;
    mRunStandardTest( gen.setSeed(cUrgSeed), "RandGen::setSeed" );
    mRunStandardTest( expected.setSeed(cUrgSeed),
		      "RandGen::setSeed on expected" );
    gen.get();
    expected.get();

    const bool crashaborted = DBG::setCrashOnProgError( false );
    mRunStandardTest( !gen.setSeed(cInvalidSeed),
		      "RandGen::setSeed(0) returns false" );
    mRunStandardTest( gen.get()==expected.get(),
		      "RandGen sequence unchanged after setSeed(0)" );

    gen.setSeed( cUrgSeed );
    expected.setSeed( cUrgSeed );
    gen.get();
    expected.get();
    mRunStandardTest( !gen.setSeed(mUdf(int)),
		      "RandGen::setSeed(mUdf(int)) returns false" );
    mRunStandardTest( gen.get()==expected.get(),
		      "RandGen sequence unchanged after setSeed(mUdf(int))" );

    Stats::NormalRandGen ngen, nexpected;
    mRunStandardTest( ngen.setSeed(cNrgSeed), "NormalRandGen::setSeed" );
    mRunStandardTest( nexpected.setSeed(cNrgSeed),
		      "NormalRandGen::setSeed on expected" );
    ngen.get();
    nexpected.get();
    mRunStandardTest( !ngen.setSeed(cInvalidSeed),
		      "NormalRandGen::setSeed(0) returns false" );
    mRunStandardTest( ngen.get()==nexpected.get(),
		      "NormalRandGen sequence unchanged after setSeed(0)" );

    ngen.setSeed( cNrgSeed );
    nexpected.setSeed( cNrgSeed );
    ngen.get();
    nexpected.get();
    mRunStandardTest( !ngen.setSeed(mUdf(int)),
		  "NormalRandGen::setSeed(mUdf(int)) returns false" );
    mRunStandardTest( ngen.get()==nexpected.get(),
		  "NormalRandGen sequence unchanged after setSeed(mUdf(int))" );
    DBG::setCrashOnProgError( crashaborted );

    return true;
}


static bool testReproducibility()
{
    Stats::RandGen g1, g2;
    mRunStandardTest( g1.setSeed(cUrgSeed), "RandGen::setSeed" );
    mRunStandardTest( g2.setSeed(cUrgSeed), "RandGen::setSeed" );
    mRunStandardTest( compareUniformSequences(g1,g2,cNrReproSeq),
		      "RandGen reproducible sequence" );

    Stats::NormalRandGen n1, n2;
    mRunStandardTest( n1.setSeed(cNrgSeed), "NormalRandGen::setSeed" );
    mRunStandardTest( n2.setSeed(cNrgSeed), "NormalRandGen::setSeed" );
    mRunStandardTest( compareNormalSequences(n1,n2,cNrReproSeq),
		      "NormalRandGen reproducible sequence" );

    return true;
}


static bool testDrawTwiceWithSeed()
{
    double first[cNrDraws], second[cNrDraws];

    Stats::RandGen gen;
    mRunStandardTest( gen.setSeed(cUrgSeedDrawTwice),
		      "RandGen::setSeed for draw twice" );
    drawUniformSet( gen, first, cNrDraws );
    mRunStandardTest( gen.setSeed(cUrgSeedDrawTwice),
		      "RandGen::setSeed replay" );
    drawUniformSet( gen, second, cNrDraws );
    mRunStandardTest( sameSet(first,second,cNrDraws),
		      "RandGen same set drawn twice with seed" );

    Stats::NormalRandGen ngen;
    mRunStandardTest( ngen.setSeed(cNrgSeedDrawTwice),
		      "NormalRandGen::setSeed for draw twice" );
    drawNormalSet( ngen, first, cNrDraws );
    mRunStandardTest( ngen.setSeed(cNrgSeedDrawTwice),
		      "NormalRandGen::setSeed replay" );
    drawNormalSet( ngen, second, cNrDraws );
    mRunStandardTest( sameSet(first,second,cNrDraws),
		      "NormalRandGen same set drawn twice with seed" );

    return true;
}


static bool testDrawTwiceWithoutSeed()
{
    double first[cNrDraws], second[cNrDraws];

    Stats::RandGen gen1, gen2;
    drawUniformSet( gen1, first, cNrDraws );
    drawUniformSet( gen2, second, cNrDraws );
    mRunStandardTest( !sameSet(first,second,cNrDraws),
		      "RandGen different sets without seed" );

    Stats::RandGen gen;
    drawUniformSet( gen, first, cNrDraws );
    gen.clearSeed();
    drawUniformSet( gen, second, cNrDraws );
    mRunStandardTest( !sameSet(first,second,cNrDraws),
		      "RandGen different sets after clearSeed" );

    Stats::NormalRandGen ngen1, ngen2;
    drawNormalSet( ngen1, first, cNrDraws );
    drawNormalSet( ngen2, second, cNrDraws );
    mRunStandardTest( !sameSet(first,second,cNrDraws),
		      "NormalRandGen different sets without seed" );

    Stats::NormalRandGen ngen;
    drawNormalSet( ngen, first, cNrDraws );
    ngen.clearSeed();
    drawNormalSet( ngen, second, cNrDraws );
    mRunStandardTest( !sameSet(first,second,cNrDraws),
		      "NormalRandGen different sets after clearSeed" );

    return true;
}


static bool testChangeSeed()
{
    Stats::RandGen g;
    mRunStandardTest( g.setSeed(cSeedChangeA), "RandGen::setSeed(42)" );
    const double first = g.get();

    mRunStandardTest( g.setSeed(cSeedChangeB), "RandGen::setSeed(99)" );
    const double otherseed = g.get();

    mRunStandardTest( g.setSeed(cSeedChangeA), "RandGen::setSeed(42) again" );
    const double replay = g.get();

    mRunStandardTest( first==replay, "setSeed restarts the sequence" );
    mRunStandardTest( first!=otherseed,
		      "Different seeds give different values" );

    return true;
}


static bool testClearAndReseed()
{
    Stats::RandGen used, fresh;
    mRunStandardTest( used.setSeed(cUrgSeed), "RandGen::setSeed" );
    used.get();
    used.get();
    used.clearSeed();
    mRunStandardTest( used.setSeed(cUrgSeed),
		      "RandGen::setSeed after clearSeed" );

    mRunStandardTest( fresh.setSeed(cUrgSeed),
		      "RandGen::setSeed on fresh gen" );
    mRunStandardTest( compareUniformSequences(used,fresh,cNrShortSeq),
		      "clearSeed then setSeed matches fresh generator" );

    Stats::NormalRandGen nused, nfresh;
    mRunStandardTest( nused.setSeed(cNrgSeed), "NormalRandGen::setSeed" );
    nused.get();
    nused.clearSeed();
    mRunStandardTest( nused.setSeed(cNrgSeed),
		      "NormalRandGen::setSeed after clear" );
    mRunStandardTest( nfresh.setSeed(cNrgSeed),
		      "NormalRandGen::setSeed on fresh" );
    mRunStandardTest( compareNormalSequences(nused,nfresh,cNrShortSeq),
		      "NormalRandGen clearSeed then setSeed matches fresh" );

    return true;
}


static bool testUniformAPI()
{
    Stats::RandGen gen;
    gen.setSeed( cUniformApiSeed );

    for ( int idx=0; idx<cNrApiDraws; idx++ )
    {
	const double val = gen.get();
	if ( val<0. || val>=1. )
	{
	    mRunStandardTest( false, "RandGen::get in [0,1)" );
	    return false;
	}
    }

    mRunStandardTest( gen.getIndex(cGetIndexSz0)==cGetIndexSz0,
		      "RandGen::getIndex(0)" );
    mRunStandardTest( gen.getIndex(cGetIndexSz1)==cGetIndexSz0,
		      "RandGen::getIndex(1)" );

    for ( int idx=0; idx<cNrApiDraws; idx++ )
    {
	const int val = gen.getInt( cIntRangeMin, cIntRangeMax );
	if ( val<cIntRangeMin || val>cIntRangeMax )
	{
	    mRunStandardTest( false, "RandGen::getInt range" );
	    return false;
	}
    }

    mRunStandardTest( gen.getInt(cIntEqualBound,cIntEqualBound)==cIntEqualBound,
		      "RandGen::getInt equal bounds" );

    return true;
}


static bool testNormalAPI()
{
    Stats::NormalRandGen gen, replay;
    gen.setSeed( cNormalApiSeed );
    replay.setSeed( cNormalApiSeed );

    for ( int idx=0; idx<cNrApiDraws; idx++ )
    {
	const double val = gen.get( cNormalExpect, cNormalStdev );
	const double replayval = replay.get( cNormalExpect, cNormalStdev );
	if ( val != replayval )
	{
	    mRunStandardTest( false, "NormalRandGen scaled get reproducible" );
	    return false;
	}
    }

    mRunStandardTest( true, "NormalRandGen scaled get reproducible" );
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

    od_ostream& ostrm = strm && strm->isOK() ? *strm.ptr() : logStream();

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

    expstddev = ( double(valrg.stop_) - double(valrg.start_) ) * oneoversqrt12;
    mRunStandardTest( rci.min() >= valrg.start_ && rci.max() <= valrg.stop_,
		      "Limits of get int" );
    mTestValI( 0, rcd.average(), expstddev, "Uniform average of int" );
    mTestValI( expstddev, rcd.stdDev(), expstddev*1e-2,
	      "Uniform stddev of int" );

    valrg.set( 20, 300 );
    mTic(); for ( int idx=0; idx<sz; idx++ )
	ivals[idx] = urg.getInt( valrg.start_, valrg.stop_ );
    mTac();

    rcd.clear(); rci.clear();
    for ( int idx=0; idx<sz; idx++ )
    {
	rcd.addValue( ivals[idx] );
	rci.addValue( ivals[idx] );
    }
    mRunStandardTest( rci.min() >= valrg.start_ && rci.max() <= valrg.stop_,
		      "Limits of get int range" );
    expavg = valrg.center();
    expstddev = valrg.width() * oneoversqrt12;
    mTestValI( valrg.center(), rcd.average(), expstddev*1e-2,
	      "Uniform avg of int range" );
    mTestValI( expstddev, rcd.stdDev(), expstddev*1e-2,
	      "Uniform stddev of int range" );

    valrg.start_ = 0;
    mTic(); for ( int idx=0; idx<sz; idx++ )
	ivals[idx] = urg.getIndex( valrg.stop_ );
    mTac();

    rcd.clear(); rci.clear();
    for ( int idx=0; idx<sz; idx++ )
    {
	rcd.addValue( ivals[idx] );
	rci.addValue( ivals[idx] );
    }
    mRunStandardTest( rci.min() >= valrg.start_ && rci.max() <= valrg.stop_,
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
	llvals[idx] = urg.getIndex( lvalrg.stop_ );
    mTac();

    rcd.clear(); rcll.clear();
    for ( int idx=0; idx<sz; idx++ )
    {
	rcd.addValue( llvals[idx] );
	rcll.addValue( llvals[idx] );
    }
    mRunStandardTest( rcll.min() >= lvalrg.start_ && rcll.max() <= lvalrg.stop_,
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

    if ( !(testSetSeedRejectsZeroOrUdf() &&
	   testReproducibility() &&
	   testDrawTwiceWithSeed() &&
	   testDrawTwiceWithoutSeed() &&
	   testChangeSeed() &&
	   testClearAndReseed() &&
	   testUniformAPI() &&
	   testNormalAPI()) )
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
