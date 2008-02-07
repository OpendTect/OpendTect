/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: initgeometry.cc,v 1.2 2008-02-07 14:53:54 cvsbert Exp $
________________________________________________________________________

-*/

#include "initgeometry.h"
#include "polyposprovider.h"
#include "tableposprovider.h"

void Geometry::initStdClasses()
{
    Pos::PolyProvider3D::initClass();
    Pos::TableProvider3D::initClass();
}
