/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh
 * DATE     : June 2007
-*/

static const char* rcsID = "$Id: tutlogtools.cc,v 1.4 2011/10/26 14:20:13 cvsbruno Exp $";

#include "tutlogtools.h"
#include "welllog.h"
#include "statruncalc.h"

Tut::LogTools::LogTools( const Well::Log& inp, Well::Log& outp )
	: inplog_(inp)
	, outplog_(outp)
{
}		

bool Tut::LogTools::runSmooth( const int inpgate )
{
    outplog_.setUnitMeasLabel( inplog_.unitMeasLabel() );

    const int gate = inpgate % 2 ? inpgate : inpgate + 1;  
    const int rad = gate / 2;
    Stats::WindowedCalc<float> wcalc(
	    		Stats::CalcSetup().require(Stats::Median), gate );
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
	    outplog_.addValue( inplog_.dah(cpos), inplog_.value(cpos) );

	if ( cpos<rad && cpos>=0 )
	    outplog_.addValue( inplog_.dah(cpos), inplog_.value(cpos) );
    }

    return true;
}
