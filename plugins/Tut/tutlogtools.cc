/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : June 2007
-*/

static const char* rcsID = "$Id: tutlogtools.cc,v 1.1 2007-06-19 09:57:58 cvsraman Exp $";

#include "tutlogtools.h"
#include "statruncalc.h"


bool Tut::LogTools::runSmooth( const int inpgate )
{
    outplog_.setUnitMeasLabel( inplog_.unitMeasLabel() );

    const int gate = inpgate % 2 ? inpgate : inpgate + 1;  
    const int rad = gate / 2;
    Stats::WindowedCalc<float> wcalc(
	    		Stats::RunCalcSetup().require(Stats::Median), gate );
    const int sz = inplog_.size();
    for ( int idx=0; idx<sz+rad; idx++ )
    {
	const int cpos = idx - rad;
	if ( idx < sz )
	{
	    const float inval = inplog_.value(idx);
	    if (!mIsUdf(inval) )
		wcalc += inval;
	    if ( cpos >= rad )
		outplog_.addValue( inplog_.dah(cpos), wcalc.median() );
	}
	else
	    outplog_.addValue( inplog_.dah(cpos),
		    				inplog_.value(cpos) );

	if ( cpos<rad && cpos>=0 )
	    outplog_.addValue( inplog_.dah(cpos), 
					    	inplog_.value(cpos) );
    }

    return true;
}
