/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";



#include "moddepmgr.h"

#include "genc.h"
#include "settings.h"
#include "visshape.h"
#include "indexedshape.h"
#include "visrgbatexturechannel2rgba.h"

#include <osgGeo/LayeredTexture>
#include <osg/Notify>


mDefModInitFn(visBase)
{
    mIfNotFirstTime( return );

    if( !getenv("OSG_NOTIFY_LEVEL") && !getenv("OSGNOTIFYLEVEL") )
#ifdef __debug__
	setNotifyLevel( osg::WARN );
#else
	setNotifyLevel( osg::ALWAYS ); // which strangely enough means never.
#endif

    int maxsize = -1;
    mSettUse( get, "dTect", "Max texture size override", maxsize );
    osgGeo::LayeredTexture::overrideGraphicsContextMaxTextureSize( maxsize );

    Geometry::PrimitiveSetCreator::setCreator(
				    new visBase::PrimitiveSetCreator );

    visBase::ColTabTextureChannel2RGBA::initClass();
    visBase::RGBATextureChannel2RGBA::initClass();

    //Notify threads to shut down at program end.
    NotifyExitProgram(osgGeo::LayeredTexture::shutdownThreading);
}
