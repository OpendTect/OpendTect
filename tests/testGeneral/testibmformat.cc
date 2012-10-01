/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION : 
-*/

static const char* rcsID mUsedVar = "$Id: testibmformat.cc,v 1.1 2012/07/13 07:35:07 cvskris Exp $";

#include "ibmformat.h"
#include "math2.h"
#include "task.h"
#include "limits.h"

#include <iostream>


bool testFloatIndex( int origin, float target )
{
    const float fval = IbmFormat::asFloat( &origin );
    if ( !Math::IsNormalNumber(fval) )
	return true;

    if ( !mIsUdf(target) && fval!=target )
    {
	std::cerr << "Failure at index " << origin << ": Should be "
		  << target << " but is " << fval <<"\n";
	return false;
    }
    
    unsigned int buf;
    IbmFormat::putFloat( fval, &buf );

    const float res = IbmFormat::asFloat( &buf );
    
    if ( fval != res )
    {
	std::cerr << "Failure at index " << origin << ": Should be "
		  << fval << " but is " << res <<"\n";
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

	    if ( !testFloatIndex( idx, mUdf(float) ) )
		return false;
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
	std::cerr << "Failed test with IbmFormat::as##func( " \
		  << bufval << " )\n"; \
	return 1; \
    } \
 \
    IbmFormat::put##func( correctval, &resbuf ); \
    if ( resbuf!=buf ) \
    { \
	std::cerr << "Failed test with IbmFormat::put##func( " \
		  << bufval << " )\n"; \
	return 1; \
    } \
} 


    
int main( int narg, char** argv )
{
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
    if ( !testFloatIndex( 291, 4.7019774e-38) ||
	 !testFloatIndex( -2147390723, -3.40282347e+38 ) )
	return 1;

    return 0;

    //Optional, run entire number-space.
    IbmFormatTester tester;
    return tester.execute() ? 0 : 1;
}
