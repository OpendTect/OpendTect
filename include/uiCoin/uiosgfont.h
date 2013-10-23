#ifndef uiosgfont_h
#define uiosgfont_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          07/02/2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "vistext.h"

#include "uicoinmod.h"


mClass(uiCoin) uiOsgFontCreator : public visBase::OsgFontCreator
{
public:
    static void			initClass();
    osgText::Font*		createFont(const FontData&);
};

#endif

