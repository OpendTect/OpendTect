/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tutlogtools.h"
#include "statruncalc.h"
#include "welllog.h"

Tut::LogTools::LogTools( const Well::Log& inp, Well::Log& outp )
    : inplog_(inp)
    , outplog_(outp)
{
}


Tut::LogTools::~LogTools()
{
}


bool Tut::LogTools::runSmooth( const int inpgate )
{
    const int gate = inpgate % 2 ? inpgate : inpgate + 1;
    const int rad = gate / 2;
    Stats::WindowedCalc<float> wcalc(
			Stats::CalcSetup().require(Stats::Median), gate );
    const int sz = inplog_.size();
    for ( int idx=0; idx<sz+rad; idx++ )
    {
	const int cpos = idx - rad;
	const float dah = inplog_.dah( cpos );
	const float cposval = inplog_.value( cpos );
	if ( idx < sz )
	{
	    const float inval = inplog_.value( idx );
	    if ( !mIsUdf(inval) )
		wcalc += inval;

	    if ( cpos >= rad )
		outplog_.addValue( dah, wcalc.median() );
	}
	else
	    outplog_.addValue( dah, cposval );

	if ( cpos<rad && cpos>=0 )
	    outplog_.addValue( dah, cposval );
    }

    outplog_.setUnitMeasLabel( inplog_.unitMeasLabel() );
    outplog_.setMnemonicLabel( inplog_.mnemonicLabel() );

    return true;
}
