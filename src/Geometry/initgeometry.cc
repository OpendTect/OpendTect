/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initgeometry.cc,v 1.6 2011/08/23 14:51:33 cvsbert Exp $";

#include "moddepmgr.h"
#include "polyposprovider.h"
#include "tableposprovider.h"

mDefModInitFn(Geometry)
{
    mIfNotFirstTime( return );

    Pos::PolyProvider3D::initClass();
    Pos::TableProvider3D::initClass();
}
