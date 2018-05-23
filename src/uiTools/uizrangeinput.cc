/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/

#include "uizrangeinput.h"

#include "zdomain.h"


uiZRangeInput::uiZRangeInput( uiParent* p, bool depth, bool withstep )
    : uiGenInput( p, toUiString("%1 %2")
		     .arg((depth ? ZDomain::Depth().userName()
				 : ZDomain::Time().userName()))
		     .arg(uiStrings::sRange())
		     .withUnit(depth ? ZDomain::Depth().unitStr()
				     : ZDomain::Time().unitStr()),
		     FloatInpIntervalSpec(withstep) )
    , isdepth_( depth )
    , withstep_( withstep )
{}


#define mImplGet( fn, tp ) \
StepInterval<tp> uiZRangeInput::get##fn##ZRange() const \
{ \
    if ( withstep_ ) \
    { \
	StepInterval<tp> res = get##fn##StepInterval( mUdf(tp) ); \
	if ( !res.isUdf() && !isdepth_ ) \
	{ \
	    const float scale = 1.f/ZDomain::Time().userFactor(); \
	    res.scale( scale ); \
	} \
\
	return res; \
    } \
\
    Interval<tp> res = get##fn##StepInterval( mUdf(tp) ); \
    if ( !res.isUdf() && !isdepth_ ) \
    { \
	const float scale = 1.f/ZDomain::Time().userFactor(); \
	res.scale( scale ); \
    } \
\
    return StepInterval<tp>( res.start, res.stop, mUdf(tp) ); \
}


mImplGet( F, float )
mImplGet( D, double )
