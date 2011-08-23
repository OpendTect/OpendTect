/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initgeneral.cc,v 1.9 2011-08-23 14:51:33 cvsbert Exp $";

#include "moddepmgr.h"
#include "rangeposprovider.h"
#include "propertyimpl.h"

mDefModInitFn(General)
{
    mIfNotFirstTime( return );

    Pos::RangeProvider3D::initClass();
    Pos::RangeProvider2D::initClass();
    ValueProperty::initClass();
    RangeProperty::initClass();
    MathProperty::initClass();
}
