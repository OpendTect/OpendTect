/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: uizaxistransform.cc 28800 2013-03-12 16:30:43Z kristofer.tingdahl@dgbes.com $";

#include "uizrangeinput.h"

#include "zdomain.h"

#include "datainpspec.h"
#include "refcount.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uidialog.h"
#include "zaxistransform.h"
#include "uimsg.h"


uiZRangeInput::uiZRangeInput( uiParent* p, bool depth, bool withstep )
    : uiGenInput( p, BufferString(
	depth ? ZDomain::Depth().userName() : ZDomain::Time().userName(),
	" Range ",
	depth ? ZDomain::Depth().unitStr(true) : ZDomain::Time().unitStr(true)),
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
	    res.scale( ZDomain::Time().userFactor() ); \
\
	return res; \
    } \
\
    Interval<tp> res = get##fn##StepInterval( mUdf(tp) ); \
    if ( !res.isUdf() && !isdepth_ ) \
	res.scale( ZDomain::Time().userFactor() ); \
\
    return StepInterval<tp>( res.start, res.stop, mUdf(tp) ); \
}


mImplGet( F, float )
mImplGet( D, double )
