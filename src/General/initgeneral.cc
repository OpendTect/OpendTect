/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initgeneral.cc,v 1.5 2008-11-25 15:35:22 cvsbert Exp $";

#include "initgeneral.h"
#include "rangeposprovider.h"

void General::initStdClasses()
{
    Pos::RangeProvider3D::initClass();
    Pos::RangeProvider2D::initClass();
}
