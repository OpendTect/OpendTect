#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2015
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "uipresentationmakermod.h"
#include "namedobj.h"

#include "uigeom.h"
#include "bufstring.h"

mExpClass(uiPresentationMaker) SlideLayout
{
public:
			SlideLayout();

    void		saveToSettings();
    void		readFromSettings();

    float		maxWidth() const;
    float		maxHeigth() const;

    int			format_;
    float		width_;
    float		height_;
    float		left_;
    float		right_;
    float		top_;
    float		bottom_;
};


mExpClass(uiPresentationMaker) SlideContent
{
public:
			SlideContent();
			~SlideContent();

    void		getPythonScript(BufferString&) const;

    BufferString	title_;

    BufferString	imagefnm_;
    uiSize		imagesz_;
    uiPoint		imagepos_;

    int			index_;
    int			layoutindex_;
};
