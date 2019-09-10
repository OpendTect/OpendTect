#pragma once
/* -*-c++-*- OpenSceneGraph - Copyright (C) 2009-2010 Mathias Froehlich
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.	The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

#include "uiosgmod.h"
#include "commondefs.h"
#include <osgText/Font>
#include <QtGui/QFont>
#include <string>

mExpClass(uiOSG) ODFontImplementation : public osgText::Font::FontImplementation
{
public:
    ODFontImplementation(const QFont& font);
    virtual ~ODFontImplementation();

    virtual std::string getFileName() const;

    virtual bool supportsMultipleFontResolutions() const { return true; }

    virtual osgText::Glyph* getGlyph(const osgText::FontResolution& fontRes,
				     unsigned int charcode);
    virtual osgText::Glyph3D*	getGlyph3D(const osgText::FontResolution&,
					   unsigned int /*charcode*/)
				{ return nullptr; }

    virtual osg::Vec2		getKerning(const osgText::FontResolution&,
					   unsigned int leftcharcode,
					   unsigned int rightcharcode,
					   osgText::KerningType kerningType)
				{ return osg::Vec2(0,0); }
/*
    virtual osgText::Glyph3D*	getGlyph3D(unsigned int )
				{ return 0; }
    virtual osg::Vec2		getKerning(unsigned int leftcharcode,
					   unsigned int rightcharcode,
					   osgText::KerningType kerningType)
				{ return osg::Vec2(0, 0); }
*/

    virtual bool		hasVertical() const
				{ return true; }

protected:

    std::string		    _filename;
    QFont		    _font;
};
