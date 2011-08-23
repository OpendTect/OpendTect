/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initgeometry.cc,v 1.5 2011-08-23 06:54:11 cvsbert Exp $";

#include "initgeometry.h"
#include "polyposprovider.h"
#include "tableposprovider.h"

void Geometry::initStdClasses()
{
    mIfNotFirstTime( return );

    Pos::PolyProvider3D::initClass();
    Pos::TableProvider3D::initClass();
}
