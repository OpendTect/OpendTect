/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          18/08/1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uilayout.h"
#include "errh.h"


uiConstraint::uiConstraint( constraintType tp, i_LayoutItem* o, int marg )
    : other_(o)
    , type_(tp)
    , margin_(marg)
    , enabled_(true)
{
    if ( !other_ && (type_<leftBorder || type_>hCentered) )
    { pErrMsg("No attachment defined!!"); }
}


bool uiConstraint::operator==( const uiConstraint& oth ) const
{
    return type_ == oth.type_ && other_ == oth.other_
	&& margin_ == oth.margin_ && enabled_ == oth.enabled_;
}


bool uiConstraint::operator!=( const uiConstraint& oth ) const
{ return !(*this == oth); }

bool uiConstraint::enabled() const		{ return enabled_ ; }
void uiConstraint::disable( bool yn=true )	{ enabled_ = !yn; }
