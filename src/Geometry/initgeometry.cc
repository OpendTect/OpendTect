/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: initgeometry.cc,v 1.1 2008-02-05 14:25:26 cvsbert Exp $
________________________________________________________________________

-*/

#include "initgeometry.h"
#include "polyposprovider.h"

void Geometry::initStdClasses()
{
    Pos::PolyProvider3D::initClass();
}
