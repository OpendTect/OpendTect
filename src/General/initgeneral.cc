/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initgeneral.cc,v 1.7 2010-10-14 09:58:06 cvsbert Exp $";

#include "initgeneral.h"
#include "rangeposprovider.h"
#include "propertyimpl.h"

void General::initStdClasses()
{
    Pos::RangeProvider3D::initClass();
    Pos::RangeProvider2D::initClass();
    ValueProperty::initClass();
    RangeProperty::initClass();
    MathProperty::initClass();
}
