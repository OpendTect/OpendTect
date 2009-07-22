/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initgeometry.cc,v 1.4 2009-07-22 16:01:33 cvsbert Exp $";

#include "initgeometry.h"
#include "polyposprovider.h"
#include "tableposprovider.h"

void Geometry::initStdClasses()
{
    Pos::PolyProvider3D::initClass();
    Pos::TableProvider3D::initClass();
}
