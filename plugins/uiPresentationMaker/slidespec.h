#ifndef slidespec_h
#define slidespec_h

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


mExpClass(uiPresentationMaker) SlideSpec
{
public:
			SlideSpec();
			~SlideSpec();

    void		getPythonScript(BufferString&) const;

    BufferString	title_;

    BufferString	imagefnm_;
    uiSize		imagesz_;
    uiPoint		imagepos_;

    int			layoutindex_;
    int			index_;
};

#endif
