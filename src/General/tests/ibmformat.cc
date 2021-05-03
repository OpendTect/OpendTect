/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/


#include "ibmformat.h"
#include "testprog.h"
#include "math2.h"
#include "limits.h"
#include "paralleltask.h"


bool testFloatIndex( int origin, float target )
{
    const float fval = IbmFormat::asFloat( &origin );
    if ( !Math::IsNormalNumber(fval) )
	return true;

    if ( !mIsUdf(target) && fval!=target )
    {
	od_cout() << "Conversion 1 failed for origin " << origin
		  << ": Should be " << target << " but is " << fval <<"\n";
	return false;
    }

    unsigned int buf;
    IbmFormat::putFloat( fval, &buf );
    if ( buf!=origin )
    {
	od_cout() << "Conversion 2 failed for origin " << origin
		  << ": Should be " << origin << " but is " << buf <<"\n";
	return false;
    }

    const float res = IbmFormat::asFloat( &buf );

    if ( fval != res )
    {
	od_cout() << "Conversion 3 failed at origin " << origin
		  << ": Should be " << fval << " but is " << res <<"\n";
	return false;
    }

    return true;
}

class IbmFormatTester : public ParallelTask
{
public:
    od_int64	nrIterations() const { return UINT_MAX; }

    bool doWork( od_int64 start, od_int64 stop, int )
    {
        for ( od_int64 idx=start; idx<=stop; idx++ )
        {
            if ( !(idx%1000000) && !shouldContinue() )
                return false;

	    if ( !testFloatIndex( mCast(int,idx), mUdf(float) ) )
	    { return false; }
        }

        return true;
    }

};

#define mTestVal( tp, func, bufval, correctval ) \
{ \
    buf = bufval; \
    const tp val = IbmFormat::as##func( &buf ); \
    if ( !mComp( val, correctval ) ) \
    { \
	od_cout() << "Failed test with IbmFormat::as##func( " \
		  << bufval << " )\n"; \
	return 1; \
    } \
 \
    IbmFormat::put##func( correctval, &resbuf ); \
    if ( resbuf!=buf ) \
    { \
	od_cout() << "Failed test with IbmFormat::put##func( " \
		  << bufval << " )\n"; \
	return 1; \
    } \
}



int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

#define mComp( v1, v2 ) ( v1==v2 )
    int buf;
    int resbuf;

    //Test all cases in asN / putN
    mTestVal( int, Int, 0x01010101, 16843009 );
    mTestVal( int, Int, 0x010101FF, -16711423 );

    mTestVal( short, Short, 0x01010101, 257 );
    mTestVal( short, Short, 0x010101FF, -255 );

    mTestVal( unsigned short, UnsignedShort, 0x01010101, 257 );
    mTestVal( unsigned short, UnsignedShort, 0x010101FF, 65281 );

    //Test two known problem-spots
     if ( !testFloatIndex( 152776, -1409417216.000000f ) )
	 return 1;

    if ( !testFloatIndex( 0, 0) )
	return 1;

    return 0;

    //Optional, run entire number-space.
    //IbmFormatTester tester;
    //return tester.execute() ? 0 : 1;
}
