/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";



#include "moddepmgr.h"

#include "settings.h"
#include "visshape.h"
#include "indexedshape.h"

#include <osgGeo/LayeredTexture>


mDefModInitFn(visBase)
{
    mIfNotFirstTime( return );

    int maxsize = -1;
    mSettUse( get, "dTect", "Max texture size override", maxsize );
    osgGeo::LayeredTexture::overrideGraphicsContextMaxTextureSize( maxsize );

    Geometry::PrimitiveSetCreator::setCreator(
				    new visBase::PrimitiveSetCreator );
}
