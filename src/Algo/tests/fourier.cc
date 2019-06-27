/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/

#include "fourier.h"

#include "testprog.h"
#include "arrayndimpl.h"
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

    bool	execute();

private:

    int			sz_;
};


bool checkRMSDifference( float_complex* a, float_complex* b, od_uint64 size,
                         float eps )
{
    double err2 = 0;

    for ( int idx=mCast(int,size-1); idx>=0; idx-- )
    {
        const float_complex diff = a[idx]-b[idx];
	err2 += std::norm( diff );
    }

    err2 /= (size*2);

    const double rms = Math::Sqrt( err2 );
    return rms<eps;
}


#undef mTest
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


//-------------------Test 1D------------------------------------
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
    mTest( testname.buf(),
	   checkRMSDifference( transformdata.arr(), reference.arr(),
			       transformdata.size(), 2e-5 ) );
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

//------------------------------ Test 2D --------------------------------

class FFTChecker2D : public Task
{
public:
                FFTChecker2D( int sz0, int sz1 )
                    : sz0_( sz0 )
		    , sz1_( sz1 )		{}

    bool	execute();

private:

    int		sz0_;
    int		sz1_;
};


//2D test will cover the nD case.
bool testForwardCC2D( const Array2D<float_complex>& input )
{

    const int nrsamplesdim0 = input.info().getSize(0);
    const int nrsamplesdim1 = input.info().getSize(1);
    Array2DImpl<float_complex> reference( nrsamplesdim0, nrsamplesdim1 );
    reference.setAll( float_complex(0,0) );

    const double anglefactor0 = (-2*M_PI)/nrsamplesdim0;
    const double anglefactor1 = (-2*M_PI)/nrsamplesdim1;
    for ( int idx0=nrsamplesdim0-1; idx0>=0; idx0-- )
    {
	for ( int idx1=nrsamplesdim1-1; idx1>=0; idx1-- )
	{
	    std::complex<double> freqsum(0,0);
	    const double angleincrement0 = anglefactor0*idx0;
	    const double angleincrement1 = anglefactor1*idx1;
	    for ( int idy0=nrsamplesdim0-1; idy0>=0; idy0-- )
	    {
		const double angle0 = angleincrement0*idy0;
		for ( int idy1=nrsamplesdim1-1; idy1>=0; idy1-- )
		{
		    const double angle1 = angleincrement1*idy1;
		    const std::complex<double> contrib( cos(angle0+angle1),
							sin(angle0+angle1) );
		    const std::complex<double> val( input.get(idy0,idy1).real(),
						   input.get(idy0,idy1).imag());
		    freqsum += contrib * val;
		}
	    }

	    freqsum /= nrsamplesdim0 * nrsamplesdim1;
	    reference.set( idx0,idx1, float_complex( (float) freqsum.real(),
						     (float) freqsum.imag() ) );
	}
    }

    Fourier::CC transform;
    float_complex* transformdata = const_cast<float_complex*>(input.getData());

    transform.setInputInfo( input.info() );
    transform.setDir( true );

    BufferString testname( "Running FFT size ",
			   toString( input.info().totalSize() ) );
    mTest( testname.buf(), transform.run( transformdata ));

    testname = "FFT results ";
    testname.add( input.info().totalSize() );
    mTest( testname.buf(),
	   checkRMSDifference( transformdata, reference.getData(),
			       reference.info().totalSize(), 2e-5 ) );
    return true;
}


bool FFTChecker2D::execute()
{
    Array2DImpl<float_complex> testdata( sz0_, sz1_ );
    testdata.setAll( float_complex(0,0) );
    testdata.set( 1, 1, float_complex(1,0) );

    if ( !testForwardCC2D( testdata ) )
	return false;

    return true;
}


//------------------------------- test --------------------------------
int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

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

    if ( !Threads::WorkManager::twm().addWork( workload ) )
        return 1;

    return 0;
}
