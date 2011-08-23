/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initgeneral.cc,v 1.8 2011-08-23 06:54:11 cvsbert Exp $";

#include "initgeneral.h"
#include "rangeposprovider.h"
#include "propertyimpl.h"

void General::initStdClasses()
{
    mIfNotFirstTime( return );

    Pos::RangeProvider3D::initClass();
    Pos::RangeProvider2D::initClass();
    ValueProperty::initClass();
    RangeProperty::initClass();
    MathProperty::initClass();
}
