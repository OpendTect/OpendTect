#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "bufstring.h"
#include "color.h"

mExpClass(General) PixmapDesc
{
public:
			PixmapDesc();
			PixmapDesc(const PixmapDesc&);
			PixmapDesc(const char* src,int width,int height,
				   OD::Color);
			~PixmapDesc();

    PixmapDesc&		operator=(const PixmapDesc&);
    void		set(const char* src,int width,int height,
			    OD::Color col=OD::Color::NoColor());
    bool		isValid() const;
    BufferString	toString() const;
    void		fromString(const char*);

    BufferString	source_;
    int			width_		= 0;
    int			height_		= 0;
    OD::Color		color_		= OD::Color::NoColor();

    static const char*	sKeyXpmSrc()		{ return "[xpm]"; }
    static const char*	sKeyRgbSrc()		{ return "[uiRGBArray]"; }
    static const char*	sKeyGradientSrc()	{ return "[Gradient]"; }
    static const char*	sKeyColorTabSrc()	{ return "[colortable]"; }
    static const char*	sKeySingleColorSrc()	{ return "[singlecolor]"; }
    static const char*	sKeyNoSrc()		{ return "[none]"; }

};
