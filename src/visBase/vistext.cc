/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: vistext.cc,v 1.6 2002-05-27 14:20:04 nanne Exp $";


#include "vistext.h"
#include "iopar.h"

const char* visBase::Text::stringstr = "Text";
const char* visBase::Text::fontsizestr = "Font size";
const char* visBase::Text::justificationstr = "Justification";
const char* visBase::Text::positionstr = "Position";

#include "Inventor/nodes/SoText2.h"
#include "Inventor/nodes/SoFont.h"
#include "Inventor/nodes/SoTranslation.h"

mCreateFactoryEntry( visBase::Text );

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


BufferString visBase::Text::getText() const
{
    SbString val;
    text->string.get( val );
    BufferString res = val.getString();
    return res;
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
    else if ( just==Left )
	text->justification.setValue( SoText2::LEFT );
    else
	text->justification.setValue( SoText2::RIGHT );
}


void visBase::Text::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    Geometry::Pos pos = position();
    par.set( positionstr, pos.x, pos.y, pos.z );

    par.set( justificationstr, (int) justification() );
    par.set( fontsizestr, size() );
    par.set( stringstr, (const char* ) getText() );
}


int visBase::Text::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    Geometry::Pos pos;
    if ( !par.get( positionstr, pos.x, pos.y, pos.z ) )
	return -1;

    int just;
    if ( !par.get( justificationstr, just ))
	return -1;

    float fontsz;
    if ( !par.get( fontsizestr, fontsz ))
	return -1;

    const char* str = par.find( stringstr );
    if ( !str ) return -1;

    setText( str );
    setPosition( pos );
    setSize( fontsz );
    setJustification( (Justification) just );

    return 1;
}
