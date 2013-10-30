/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "fourier.h"

#include "commandlineparser.h"
#include "keystrs.h"
#include "math2.h"
#include "threadlock.h"
#include "threadwork.h"

#include <iostream>

static Threads::Lock streamlock( false );

class FFTChecker : public Task
{
public:
                FFTChecker(bool quiet,int sz )
                    : quiet_(quiet)
                    , sz_( sz )
    		{}

    bool	execute();

private:

    bool		quiet_;
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
    if ( !quiet ) \
    { \
	Threads::Locker lock( streamlock ); \
	std::cout << testname << ": OK\n"; \
    } \
} \
else \
{ \
    Threads::Locker lock( streamlock ); \
    std::cout << testname << ": Failed\n"; \
    return false; \
} \


bool testForwardCC( bool quiet, const TypeSet<float_complex>& input )
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

        reference[idx] = float_complex( freqsum.real(),
                                        freqsum.imag() );
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
    mTest( testname.buf(), checkRMSDifference( transformdata, reference, 2e-5 ) );

    return true;
}


bool FFTChecker::execute()
{
    TypeSet<float_complex> testdata( sz_, float_complex(0,0) );
    testdata[1] = float_complex(1,0);

    if ( !testForwardCC( quiet_, testdata ) )
    	return false;

    return true;
}



int main( int argc, char** argv )
{
    od_init_test_program( argc, argv );

    const bool quiet = CommandLineParser().hasKey( sKey::Quiet() );

    const int sizes[] = { 4, 8, 9, 22, 37, 182, 1111, 9873, 12345, -1};

    TypeSet<Threads::Work> workload;

    int idx = 0;
    while ( sizes[idx]>0 )
    {
        int sz = sizes[idx];
        workload += Threads::Work(
                        *new FFTChecker(quiet,sz), true );

        const int fastsz = Fourier::FFTCC1D::getFastSize(sz);

        if ( fastsz != sz )
            workload += Threads::Work(*new FFTChecker(quiet,fastsz), true );

        idx++;
    }

    if ( !Threads::WorkManager::twm().addWork( workload ) )
        ExitProgram( 1 );

    return ExitProgram( 0 );
}

