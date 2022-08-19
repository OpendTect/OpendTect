/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "arrayndalgo.h"
#include "testprog.h"

using namespace ArrayMath;

static bool testSmallFloatArray( Array1D<float>& prices,
				 Array1D<float>& milage )
{
    const int sz = mCast(int,prices.info().getTotalSz());
    Array1DImpl<float> pricesplusmilage( sz ), pricestimesmilage( sz );
    prices.set(  0,   9300.f ); milage.set(  0,  7100.f );
    prices.set(  1,  10565.f ); milage.set(  1, 15500.f );
    prices.set(  2,  15000.f ); milage.set(  2,  4400.f );
    prices.set(  3,  15000.f ); milage.set(  3,  4400.f );
    prices.set(  4,  17764.f ); milage.set(  4,  5900.f );
    prices.set(  5,  57000.f ); milage.set(  5,  4600.f );
    prices.set(  6,  65940.f ); milage.set(  6,  8800.f );
    prices.set(  7,  73676.f ); milage.set(  7,  2000.f );
    prices.set(  8,  77006.f ); milage.set(  8,  2750.f );
    prices.set(  9,  93739.f ); milage.set(  9,  2550.f );
    prices.set( 10, 146088.f ); milage.set( 10,   960.f );
    prices.set( 11, 153260.f ); milage.set( 11,  1025.f );

    Array1DImpl<float> results( 13 );
    float* res = results.getData();

    res[0] = getSum( prices, true, false );
    res[1] = getSum( prices, true, true );
    res[2] = getAverage( prices, true, false );
    res[3] = getAverage( prices, true, true );
    res[4] = getSumProduct( prices, milage, true, false );
    res[5] = getSumProduct( prices, milage, true, true );

    mRunStandardTest( mIsEqual(res[0],734338.0f,1e-1f), "CumSum 1-CPU" );
    mRunStandardTest( mIsEqual(res[1],734338.0f,1e-1f), "CumSum N-CPU" );
    mRunStandardTest( mIsEqual(res[2],61194.83f,1e-2f), "Average 1-CPU" );
    mRunStandardTest( mIsEqual(res[3],61194.83f,1e-2f), "Average N-CPU" );
    mRunStandardTest( mIsEqual(res[4],2.2045560e9f,1e3f), "SumProduct 1-CPU" );
    mRunStandardTest( mIsEqual(res[5],2.2045560e9f,1e3f), "SumProduct N-CPU" );

    const float summilage = getSum( milage, true, true );
    getSum( prices, milage, pricesplusmilage, true, true );
    const float sumsumtest1 = getSum( pricesplusmilage, true, true );
    mRunStandardTest( mIsEqual(res[1]+summilage,sumsumtest1,1e-1f),
		      "Sum of Sum test" )

    getSumArrays( prices, milage, pricesplusmilage, 2., 3., true, true );
    const float sumsumtest2 = getSum( pricesplusmilage, true, true );
    mRunStandardTest( mIsEqual(2.f*res[1]+3.f*summilage,sumsumtest2,1e-1f),
		      "Sum of scaled Sum" )

    getProduct( prices, milage, pricestimesmilage, true, true );
    const float sumprodtest = getSum( pricestimesmilage, true, true );
    mRunStandardTest( mIsEqual(res[5],sumprodtest,1e3f), "Sum of product test" )

    res[6] = getSumSq( prices, true, true );
    res[7] = getNorm2( prices, true, true );
    res[8] = getRMS( prices, true, true );
    res[9] = getResidual( prices, milage, true,true );
    res[10] = getSumXMY2( prices, milage, true, true );
    res[11] = getSumX2PY2( prices, milage, true, true );
    res[12] = getSumX2MY2( prices, milage, true, true );

    mRunStandardTest( mIsEqual(res[6],7.35361610e10f,1e3f), "SumSq N-CPU" );
    mRunStandardTest( mIsEqual(res[7],2.71175517e5f,1e-2f), "Norm2 N-CPU" );
    mRunStandardTest( mIsEqual(res[8],7.82816289e4f,1e-3f), "RMS N-CPU" );
    mRunStandardTest( mIsEqual(res[9],5.70185833e4f,1e-3f), "Residual N-CPU" );
    mRunStandardTest( mIsEqual(res[10],6.96098762e10f,1e3f), "SumXMY2 N-CPU" );
    mRunStandardTest( mIsEqual(res[11],7.40189882e10f,1e3f), "SumX2PY2 N-CPU" );
    mRunStandardTest( mIsEqual(res[12],7.30533338e10f,1e3f), "SumX2MY2 N-CPU" );

    float intercept, gradient;
    if ( !getInterceptGradient<float,float>(milage,&prices,intercept,gradient) )
	return false;

    mRunStandardTest( mIsEqual(intercept,8136.1503f,1e-3f), "Intercept simple" )
    mRunStandardTest( mIsEqual(gradient,-0.051269039f,1e-8f),"Gradient simple" )

    Array1DImpl<float> milagedetrended( milage );
    if ( !removeTrend<float,float>(milagedetrended) )
	return false;

    const float avgmilage = getAverage( milagedetrended, true, true );
    mRunStandardTest( mIsZero(avgmilage,1e-3f), "De-trended average" )

    return true;
}


static bool testBigVector()
{
    const int sz = 120001;
    const double sr = 0.001;
    Array1DImpl<double> tvecd( sz ), data( sz );
    Array1DImpl<float> tvec( sz ), dataf( sz );
    if ( !tvecd.isOK() || !data.isOK() || !tvec.isOK() || !dataf.isOK() )
	return false;

    const double intercept = -152.4687;
    const double gradient = 3.316467;
    const double A1 = 5.1635;
    const double f1 = 50.;
    const double A2 = 1.36516;
    const double f2 = 90.;

    for ( int idx=0; idx<sz; idx++ )
    {
	const double t = sr * idx;
	tvecd.set( idx, t );
	tvec.set( idx, mCast(float,t) );
	const double coswave = A1 * cos( 2.*M_PI*f1*t );
	const double sinwave = A2 * sin( 2.*M_PI*f2*t );
	const double trend = intercept + gradient * t;
	const double val = coswave + sinwave + trend;
	data.set( idx, val );
	dataf.set( idx, mCast(float,val) );
    }

    double res = getSum( data, true, true );
    mRunStandardTest( mIsEqual(res,5582370.1,1e-1), "Sum large double" )
    float resf = getSum( dataf, true, true );
    mRunStandardTest( mIsEqual(resf,5582370.1f,1e2f), "Sum large float" )

    const double calcavg = getAverage( data, true, true );
    mRunStandardTest( mIsEqual(calcavg,46.519363,1e-6), "Average large double" )
    const float calcavgf = getAverage( dataf, true, true );
    mRunStandardTest( mIsEqual(calcavgf,46.519363f,1e-3f),"Average large float")

    double calcintercept, calcgradient;
    if ( !getInterceptGradient<double,double>(data,&tvecd,calcintercept,
					      calcgradient) )
	return false;
    mRunStandardTest(mIsEqual(calcgradient,3.316465,1e-6),"Gradient double")
    mRunStandardTest(mIsEqual(calcintercept,-152.46854,1e-5),"Intercept double")

    float calcinterceptf, calcgradientf;
    if ( !getInterceptGradient<float,float>(dataf,&tvec,calcinterceptf,
					    calcgradientf) )
	return false;
    mRunStandardTest(mIsEqual(calcgradientf,3.316465f,1e-3f),"Gradient float" )
    mRunStandardTest(mIsEqual(calcinterceptf,-152.46854f,1e-2f),
		     "Intercept float")

    if ( !removeBias<double,double>(data) || !removeBias<float,float>(dataf) )
	return false;

    res = getSum( data, true, true );
    mRunStandardTest( mIsZero(res,1e-8), "Sum no bias - double" )
    res = getAverage( data, true, true );
    mRunStandardTest( mIsZero(res,1e-13), "Average no bias - double")

    resf = getSum( dataf, true, true );
    mRunStandardTest( mIsZero(resf,1e2f), "Sum no bias - float" )
    resf = getAverage( dataf, true, true );
    mRunStandardTest( mIsZero(resf,1e-3f), "Average no bias - float" )

    //Restore initial signal
    getScaledArray<double>( data, 0, 1., calcavg, true, true );
    for ( int idx=0; idx<sz; idx++ )
	dataf.set( idx, mCast(float,data.get(idx)) );

    if ( !removeTrend<double,double>(data) || !removeTrend<float,float>(dataf))
	return false;

    res = getSum( data, true, true );
    mRunStandardTest( mIsZero(res,1e-7), "Sum no trend - double" )
    res = getAverage( data, true, true );
    mRunStandardTest( mIsZero(res,1e-12), "Average no trend - double")

    res = getSumSq( data, true, true );
    mRunStandardTest( mIsEqual(res,1711550.3,1e-1), "SumSq no trend - double" )
    resf = mCast(float,getSumProductD( data, data, true, true ) );
    mRunStandardTest( mIsEqual(res,1711550.3f,1e-1f), "SumSq no trend - float" )
    res = getNorm2( data, true, true );
    mRunStandardTest( mIsEqual(res,1308.2623,1e-4), "Norm2 no trend - double" )
    resf = mCast(float,getNorm2D( dataf, true, true ));
    mRunStandardTest( mIsEqual(res,1308.2623f,1e-4f), "Norm2 no trend - float" )
    res = getRMS( data, true, true );
    mRunStandardTest( mIsEqual(res,3.7766123,1e-7), "RMS no trend - double" )

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    const int sz = 12;
    Array1DImpl<float> prices( sz ), milage( sz );

    if ( !testSmallFloatArray(prices,milage) || !testBigVector() )
	return 1;

    return 0;
}
