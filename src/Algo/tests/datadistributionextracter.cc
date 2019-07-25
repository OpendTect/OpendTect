/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/


#include "datadistributionextracter.h"
#include "testprog.h"

#include <iostream>

static const int cNrDataPts = 10000;


static bool testDistribMaking()
{
    TypeSet<float> data;
    for ( int idx=0; idx<cNrDataPts; idx++ )
	data += (float)idx;

    RangeLimitedDataDistributionExtracter<float> extr( data,
						SilentTaskRunnerProvider() );
    RefMan<FloatDistrib> distr = extr.getDistribution();
    const float expectedtotal = (float)cNrDataPts;
    const float distrtotal = distr->sumOfValues();
    mRunStandardTest( fabs(expectedtotal - distrtotal) < 0.1f ,
		     "Cumulative freq" );

    /*
    for ( int idx=9; idx>-1; idx-- )
    {
	const float val = idx*10;
	od_cout() << "Pos for " << val << ": "
	    << distr->positionForCumulative( val ) << od_endl;
    }

    const float halfway = cNrDataPts * 0.5f;
    for ( int idx=0; idx<20; idx++ )
    {
	const float val = halfway + (idx-10)*10;
	od_cout() << "Pos for " << val << ": "
	    << distr->positionForCumulative( val ) << od_endl;
    }

    for ( int idx=0; idx<20; idx++ )
    {
	const float val = cNrDataPts - idx*10;
	od_cout() << "Pos for " << val << ": "
	    << distr->positionForCumulative( val ) << od_endl;
    }
    */

    const float histval = distr->valueAt( 300.f, false );
    const float cumval = distr->valueAt( 300.f, true );
    od_cout() << "Hist val at " << 300 << ": " << histval << " ; cum: " << cumval << od_endl;

    return true;
}

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testDistribMaking() )
	return 1;

    return 0;
}
