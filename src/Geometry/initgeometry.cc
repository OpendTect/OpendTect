/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initgeometry.cc,v 1.3 2008-11-25 15:35:22 cvsbert Exp $";

#include "initgeometry.h"
#include "polyposprovider.h"
#include "tableposprovider.h"

void Geometry::initStdClasses()
{
    Pos::PolyProvider3D::initClass();
    Pos::TableProvider3D::initClass();
}
