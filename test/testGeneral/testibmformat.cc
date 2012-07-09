/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION : Seg-Y word functions
-*/

static const char* rcsID mUnusedVar = "$Id: testibmformat.cc,v 1.1 2012-07-09 20:52:34 cvskris Exp $";

#include "ibmformat.h"
#include "math2.h"
#include "task.h"

#include <iostream>

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
            
            unsigned int origin = idx;
            const float val = IbmFormat::asFloat( &origin );
            if ( !Math::IsNormalNumber(val) )
                continue;
            
            unsigned int buf;
            IbmFormat::putFloat( val, &buf );
            
            if ( val != IbmFormat::asFloat( &buf ) )
            {
                std::cerr << "Failure at index " << idx << "\n";
                return false;
            }
        }
        
        return true;
    }

};

int main( int narg, char** argv )
{
    IbmFormatTester tester;
    return tester.execute() ? 0 : 1;
}
