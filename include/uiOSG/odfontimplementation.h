#pragma once
/* -*-c++-*- OpenSceneGraph - Copyright (C) 2009-2010 Mathias Froehlich
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

#include "uiosgmod.h"
#include "commondefs.h"
#include <osg/Version>
#include <osgText/Font>
#include <QtGui/QFont>
#include <string>

mExpClass(uiOSG) ODFontImplementation : public osgText::Font::FontImplementation
{
public:
    ODFontImplementation(const QFont& font);
    virtual ~ODFontImplementation();

    std::string getFileName() const override;

    bool supportsMultipleFontResolutions() const override { return true; }

    osgText::Glyph*		getGlyph(const osgText::FontResolution&,
					 unsigned int charcode) override;
#if OSG_MIN_VERSION_REQUIRED(3,6,0)
    osgText::Glyph3D*		getGlyph3D(const osgText::FontResolution&,
					   unsigned int /*charcode*/) override
				{ return nullptr; }

    osg::Vec2			getKerning(const osgText::FontResolution&,
					   unsigned int leftcharcode,
					   unsigned int rightcharcode,
				   osgText::KerningType kerningType) override
				{ return osg::Vec2(0,0); }
#else
    osgText::Glyph3D*		getGlyph3D( unsigned int ) override
				{ return nullptr; }
    osg::Vec2			getKerning(unsigned int leftcharcode,
					   unsigned int rightcharcode,
				   osgText::KerningType kerningType ) override
				{ return osg::Vec2(0, 0); }
#endif

    bool			hasVertical() const override	{ return true; }

protected:

    std::string             _filename;
    QFont                   _font;
};
