/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          07/02/2002
________________________________________________________________________

-*/

#include "uiosgfont.h"
#include "uifont.h"

#include <osgText/Font>
#include "odfontimplementation.h"


void uiOsgFontCreator::initClass()
{
    visBase::OsgFontCreator::setCreator( new uiOsgFontCreator );
}


osgText::Font* uiOsgFontCreator::createFont( const FontData& fd )
{
    PtrMan<mQtclass(QFont)> qfont = uiFont::createQFont( fd );
    return qfont
	? new osgText::Font( new ODFontImplementation(*qfont) )
	: 0;
}
