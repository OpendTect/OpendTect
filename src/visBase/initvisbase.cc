/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "envvars.h"
#include "genc.h"
#include "legal.h"
#include "settings.h"
#include "visshape.h"
#include "indexedshape.h"
#include "visrgbatexturechannel2rgba.h"

#include <osgGeo/LayeredTexture>
#include <osg/DisplaySettings>
#include <osg/Notify>
#include <osg/Version>

static uiString* osgLegalText();
static uiString* openGLLegalText();


mDefModInitFn(visBase)
{
    mIfNotFirstTime( return );

    const BufferString osgnotif( GetEnvVar( "OSG_NOTIFY_LEVEL" ) );
    const BufferString osgnotif2( GetEnvVar( "OSGNOTIFYLEVEL" ) );

    if( osgnotif.isEmpty() && osgnotif2.isEmpty() )
#ifdef __debug__
	setNotifyLevel( osg::WARN );
#else
	setNotifyLevel( osg::ALWAYS ); // which strangely enough means never.
#endif

    int maxsize = -1;
    mSettUse( get, "dTect", "Max texture size override", maxsize );
    osgGeo::LayeredTexture::overrideGraphicsContextMaxTextureSize( maxsize );

#if OSG_MIN_VERSION_REQUIRED(3,6,3)
// "GREYSCALE", "SDF", "ALL_FEATURES", "NO_TEXT_SHADER"
    const char* textshader = GetEnvVar( "OD_TEXT_SHADER" );
    if ( textshader )
	osg::DisplaySettings::instance()->setTextShaderTechnique( textshader );
#endif

    Geometry::PrimitiveSetCreator::setCreator(
				    new visBase::PrimitiveSetCreator );

    visBase::ColTabTextureChannel2RGBA::initClass();
    visBase::RGBATextureChannel2RGBA::initClass();

    legalInformation().addCreator( osgLegalText, "OpenSceneGraph" );
    legalInformation().addCreator( openGLLegalText, "OpenGL" );
}

static uiString* osgLegalText()
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

static uiString* openGLLegalText()
{
    //This is because GL/glext.h is needed to compile opendTect
    //(or osgGeo to be specific) and is added to the repository
    uiString* res = new uiString;
    *res = toUiString(
   "Copyright (c) 2013-2016 The Khronos Group Inc.\n"
   "\n"
   "Permission is hereby granted, free of charge, to any person obtaining a\n"
   "copy of this software and/or associated documentation files (the\n"
   "\"Materials\"), to deal in the Materials without restriction, including\n"
   "without limitation the rights to use, copy, modify, merge, publish,\n"
   "distribute, sublicense, and/or sell copies of the Materials, and to\n"
   "permit persons to whom the Materials are furnished to do so, subject to\n"
   "the following conditions:\n"
   "\n"
   "The above copyright notice and this permission notice shall be included\n"
   "in all copies or substantial portions of the Materials.\n");


    return res;
}
