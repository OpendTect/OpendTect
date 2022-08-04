#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          07/02/2002
________________________________________________________________________

-*/

#include "vistext.h"

#include "uiosgmod.h"


mClass(uiOSG) uiOsgFontCreator : public visBase::OsgFontCreator
{
public:
    static void			initClass();
    osgText::Font*		createFont(const FontData&) override;
};

