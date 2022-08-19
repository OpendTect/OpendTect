#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
