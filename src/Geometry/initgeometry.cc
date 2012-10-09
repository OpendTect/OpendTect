/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "moddepmgr.h"
#include "polyposprovider.h"
#include "tableposprovider.h"

mDefModInitFn(Geometry)
{
    mIfNotFirstTime( return );

    Pos::PolyProvider3D::initClass();
    Pos::TableProvider3D::initClass();
}
