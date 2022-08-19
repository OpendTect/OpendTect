/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uilayout.h"


uiConstraint::uiConstraint( constraintType tp, i_LayoutItem* o, int marg )
    : type_(tp)
    , other_(o)
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
{
    return !(*this == oth);
}


bool uiConstraint::enabled() const
{
    return enabled_ ;
}


void uiConstraint::disable( bool yn )
{
    enabled_ = !yn;
}
