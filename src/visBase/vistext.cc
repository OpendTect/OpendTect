/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: vistext.cc,v 1.1 2002-04-22 13:38:03 kristofer Exp $";


#include "vistext.h"


#include "Inventor/nodes/SoText2.h"
#include "Inventor/nodes/SoFont.h"
#include "Inventor/nodes/SoTranslation.h"

visBase::Text::Text()
    : VisualObjectImpl(false)
    , text( new SoText2 )
    , font( new SoFont )
    , textpos( new SoTranslation )
{
    addChild( textpos );
    addChild( font );
    addChild( text );
}


visBase::Text::~Text()
{}


Geometry::Pos visBase::Text::position() const
{
    SbVec3f pos = textpos->translation.getValue();
    return Geometry::Pos( pos[0], pos[1], pos[2] );
}


void visBase::Text::setPosition(const Geometry::Pos& pos )
{
    textpos->translation.setValue( pos.x, pos.y, pos.z );
}


const char* visBase::Text::getText() const
{
    SbString val;
    text->string.get( val );
    return val.getString();
}


void visBase::Text::setText(const char* newtext)
{
    text->string.setValue(newtext);
}


float visBase::Text::size() const
{
    return font->size.getValue();
}


void visBase::Text::setSize( float ns )
{
    font->size.setValue( ns );
}


visBase::Text::Justification visBase::Text::justification() const
{
    if ( text->justification.getValue() == SoText2::CENTER )
	return Center;
    if ( text->justification.getValue() == SoText2::LEFT )
	return Left;

    return Right;
}


void visBase::Text::setJustification( Justification just )
{
    if ( just==Center )
	text->justification.setValue( SoText2::CENTER );
    if ( just==Left )
	text->justification.setValue( SoText2::LEFT );
    else
	text->justification.setValue( SoText2::RIGHT );
}

