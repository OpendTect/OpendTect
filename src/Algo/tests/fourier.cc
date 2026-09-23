/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "fourier.h"

#include "arrayndinfo.h"
#include "testprog.h"
#include "math2.h"
#include "threadlock.h"
#include "threadwork.h"


static Threads::Lock streamlock( false );

class FFTChecker : public Task
{
public:
                FFTChecker(int sz )
                    : sz_( sz )
		{}

    bool	execute() override;

private:

    int			sz_;
};


bool checkRMSDifference( TypeSet<float_complex>& a, TypeSet<float_complex>& b,
                         float eps )
{
    if ( a.size()!=b.size() )
        return false;

    double err2 = 0;

    for ( int idx=a.size()-1; idx>=0; idx-- )
    {
        const float_complex diff = a[idx]-b[idx];
	err2 += diff.real()*diff.real() + diff.imag()*diff.imag();
    }

    err2 /= (a.size()*2);

    const double rms = Math::Sqrt( err2 );
    return rms<eps;
}


#define mTest( testname, test ) \
if ( (test)==true ) \
{ \
    Threads::Locker lock( streamlock ); \
    handleTestResult( true, testname ); \
} \
else \
{ \
    Threads::Locker lock( streamlock ); \
    handleTestResult( false, testname ); \
    return false; \
} \


bool testForwardCC( const TypeSet<float_complex>& input )
{
    TypeSet<float_complex> reference( input.size(), float_complex(0,0) );

    const int nrsamples = input.size();
    const double anglefactor = (-2*M_PI)/nrsamples;
    for ( int idx=nrsamples-1; idx>=0; idx-- )
    {
        std::complex<double> freqsum(0,0);
        const double angleincrement = anglefactor*idx;
        for ( int idy=nrsamples-1; idy>=0; idy-- )
        {
            const double angle = angleincrement*idy;
            const std::complex<double> contrib( cos(angle),
                                                sin(angle) );
            const std::complex<double> val( input[idy].real(),
                                            input[idy].imag() );
            freqsum += contrib * val;
        }

        reference[idx] = float_complex( (float) freqsum.real(),
                                        (float) freqsum.imag() );
    }

    Fourier::FFTCC1D transform;
    TypeSet<float_complex> transformdata( input );

    transform.setSize( input.size() );
    transform.setSample( 1 );
    transform.setDir( true );

    BufferString testname( "Running FFT size ", toString(input.size()) );
    mTest( testname.buf(), transform.run( transformdata.arr() ));

    testname = "FFT results ";
    testname.add( input.size() );
    mTest( testname.buf(), checkRMSDifference(transformdata,reference,2e-5) );

    return true;
}


bool FFTChecker::execute()
{
    TypeSet<float_complex> testdata( sz_, float_complex(0,0) );
    testdata[1] = float_complex(1,0);

    if ( !testForwardCC( testdata ) )
	return false;

    return true;
}


class CCFFTChecker : public Task
{
public:
    CCFFTChecker( int sz, float eps )
    : sz_( sz )
    , eps_(eps)

    {}

    bool	execute() override;

private:

    int		sz_;
    float	eps_;
};


bool testCCRoundTrip( const TypeSet<float_complex>& input, float eps )
{
    TypeSet<float_complex> data( input );
    TypeSet<float_complex> reference( input );

    Fourier::CC cc;
    cc.setInputInfo( Array1DInfoImpl( input.size() ) );
    cc.setDir( true );
    cc.setNormalization( true );
    cc.setInput( data.arr() );
    cc.setOutput( data.arr() );

    if ( !cc.run( false ) )
	return false;

    cc.setDir( false );
    if ( !cc.run( false ) )
	return false;

    BufferString testname( "CC roundtrip size ", toString( input.size() ) );
    mTest( testname.buf(), checkRMSDifference( data, reference, eps ) );
    return true;
}


bool CCFFTChecker::execute()
{
    TypeSet<float_complex> testdata( sz_, float_complex(0,0) );

    const double anglefactor = 2.0 * M_PI / sz_;
    for ( int idx=sz_-1; idx>=0; idx-- )
    {
	const double angle = anglefactor * idx;
	testdata[idx] = float_complex( (float)cos(angle), (float)sin(angle) );
    }

    return testCCRoundTrip( testdata, eps_ );
}


bool testFastnessSizes()
{
    Fourier::CC cc;
    mRunStandardTest( cc.isFast( 720720 ), "Fast size 720720 detected" );
    mRunStandardTest( !cc.isFast( 720721 ), "Oversized 720721 not fast" );
    mRunStandardTest( !cc.isFast( 1048576 ), "Oversized 1048576 not fast" );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testFastnessSizes() )
	return 1;

    const int sizes[] = { 4, 8, 9, 22, 37, 182, 1111, 9873, 12345, -1};

    TypeSet<Threads::Work> workload;

    int idx = 0;
    while ( sizes[idx]>0 )
    {
        int sz = sizes[idx];
        workload += Threads::Work(
                        *new FFTChecker(sz), true );

        const int fastsz = Fourier::FFTCC1D::getFastSize(sz);

        if ( fastsz != sz )
            workload += Threads::Work(*new FFTChecker(fastsz), true );

        idx++;
    }

    const int oversized[] = { 720720, 720721, 1048576, -1 };
//
// Per size eps needed - note 720721 is prime and falls back to DFT
// 104876 falls back to radix 4 FFT
//
    const float overeps[] = { 2e-5f, 5e-3f, 2e-4f, -1 };
    idx = 0;
    while ( oversized[idx]>0 )
    {
	workload += Threads::Work(
	    *new CCFFTChecker( oversized[idx], overeps[idx] ), true );
	idx++;
    }

    if ( !Threads::WorkManager::twm().addWork( workload ) )
        return 1;

    return 0;
}
