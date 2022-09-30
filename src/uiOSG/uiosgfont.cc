/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiosgfont.h"
#include "uifont.h"

#include <osgText/Font>
#include "odfontimplementation.h"


uiOsgFontCreator::uiOsgFontCreator()
{}


uiOsgFontCreator::~uiOsgFontCreator()
{}


void uiOsgFontCreator::initClass()
{
    visBase::OsgFontCreator::setCreator( new uiOsgFontCreator );
}


osgText::Font* uiOsgFontCreator::createFont( const FontData& fd )
{
    PtrMan<mQtclass(QFont)> qfont = uiFont::createQFont( fd );
    return qfont
	? new osgText::Font( new ODFontImplementation(*qfont) )
	: nullptr;
}
