/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";



#include "moddepmgr.h"

#include "genc.h"
#include "legal.h"
#include "settings.h"
#include "visshape.h"
#include "indexedshape.h"
#include "visrgbatexturechannel2rgba.h"

#include <osgGeo/LayeredTexture>
#include <osg/Notify>

static uiString* legalText();


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

    legalInformation().addCreator( legalText, "OpenSceneGraph" );
}

static uiString* legalText()
{
    uiString* res = new uiString;
    *res = toUiString(
"Copyright (C) 2015  Robert Osfield\n"
"\n"
"This library is free software; you can redistribute it and/or\n"
"modify it under the terms of the GNU Lesser General Public\n"
"License as published by the Free Software Foundation; either\n"
"version 2.1 of the License, or (at your option) any later version.\n"
"\n"
"This library is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
"Lesser General Public License for more details.\n"
"\n"
"%1"
).arg( lgplV3Text() );
    return res;
}
