/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: vistext.cc,v 1.11 2005-02-04 14:31:34 kristofer Exp $";


#include "vistext.h"

#include "iopar.h"
#include "vistransform.h"

#include "Inventor/nodes/SoText2.h"
#include "Inventor/nodes/SoFont.h"
#include "Inventor/nodes/SoTranslation.h"

namespace visBase
{

const char* Text::stringstr = "Text";
const char* Text::fontsizestr = "Font size";
const char* Text::justificationstr = "Justification";
const char* Text::positionstr = "Position";

mCreateFactoryEntry( Text );

Text::Text()
    : VisualObjectImpl(false)
    , text( new SoText2 )
    , font( new SoFont )
    , textpos( new SoTranslation )
    , transformation( 0 )
{
    addChild( textpos );
    addChild( font );
    addChild( text );
}


Text::~Text()
{
    if ( transformation ) transformation->unRef();
}


Coord3 Text::position() const
{
    SbVec3f pos = textpos->translation.getValue();
    Coord3 res( pos[0], pos[1], pos[2] );
    return transformation ? transformation->transformBack( res ) : res;
}


void Text::setPosition(const Coord3& pos_ )
{
    const Coord3 pos = transformation
	? transformation->transform( pos_ ) : pos_;
    textpos->translation.setValue( pos.x, pos.y, pos.z );
}


BufferString Text::getText() const
{
    SbString val;
    text->string.get( val );
    BufferString res = val.getString();
    return res;
}


void Text::setText(const char* newtext)
{
    text->string.setValue(newtext);
}


float Text::size() const
{
    return font->size.getValue();
}


void Text::setSize( float ns )
{
    font->size.setValue( ns );
}


Text::Justification Text::justification() const
{
    if ( text->justification.getValue() == SoText2::CENTER )
	return Center;
    if ( text->justification.getValue() == SoText2::LEFT )
	return Left;

    return Right;
}


void Text::setJustification( Justification just )
{
    if ( just==Center )
	text->justification.setValue( SoText2::CENTER );
    else if ( just==Left )
	text->justification.setValue( SoText2::LEFT );
    else
	text->justification.setValue( SoText2::RIGHT );
}


void Text::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    Coord3 pos = position();
    par.set( positionstr, pos.x, pos.y, pos.z );

    par.set( justificationstr, (int) justification() );
    par.set( fontsizestr, size() );
    par.set( stringstr, (const char* ) getText() );
}


int Text::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    Coord3 pos;
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


void Text::setDisplayTransformation( Transformation* nt )
{
    const Coord3 pos = position();

    if ( transformation ) transformation->unRef();
    transformation = nt;
    if ( transformation ) transformation->ref();
    setPosition( pos );
}	


Transformation* Text::getDisplayTransformation()
{
    return transformation;
}

}; // namespace visBase
