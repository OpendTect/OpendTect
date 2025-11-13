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
    for ( int idx=0; idx<sz; idx++ )
    {
	wcalc.clear();
	for ( int i=idx-rad; i<=idx+rad; i++ )
	{
	    if ( i >= 0 && i < sz )
	    {
		const float val = inplog_.value( i );
		if ( !mIsUdf(val) )
		    wcalc += val;
	    }
	}

	const float dah = inplog_.dah( idx );
	outplog_.addValue( dah, wcalc.median() );
    }

    outplog_.setUnitMeasLabel( inplog_.unitMeasLabel() );
    outplog_.setMnemonicLabel( inplog_.mnemonicLabel() );

    return true;
}
