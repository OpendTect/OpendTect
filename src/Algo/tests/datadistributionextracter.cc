/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/


#include "datadistributionextracter.h"
#include "testprog.h"

#include <iostream>


static bool testDistribMaking()
{
    TypeSet<float> data;
    for ( int idx=0; idx<12025; idx++ )
	data += (float)idx;

    DataDistributionExtracter<float> extr( data );
    if ( !extr.execute() )
	return false;

    RefMan<DataDistribution<float> > distr = extr.getDistribution();
    const float val3 = (*distr)[3];
    mRunStandardTest( val3 > 62 && val3 < 67, "Distribution Extracter" );

    return true;
}

int testMain( int argc, char** argv )
{
    mInitTestProg();

    if ( !testDistribMaking() )
	return 1;

    return 0;
}
