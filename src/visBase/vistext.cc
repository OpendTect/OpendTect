/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Apr 2002
 RCS:           $Id: vistext.cc,v 1.12 2005-02-11 11:13:37 nanne Exp $
________________________________________________________________________

-*/

#include "vistext.h"

#include "iopar.h"
#include "vistransform.h"
#include "vismaterial.h"

#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTranslation.h>

mCreateFactoryEntry( visBase::Text2 );

namespace visBase
{

const char* Text::stringstr = "Text";
const char* Text::fontsizestr = "Font size";
const char* Text::justificationstr = "Justification";
const char* Text::positionstr = "Position";


Text::Text()
    : VisualObjectImpl(false)
    , textpos(new SoTranslation)
    , font(new SoFont)
    , transformation(0)
{
    addChild( textpos );
    addChild( font );
}


Text::~Text()
{
    if ( transformation ) transformation->unRef();
}


void Text::setPosition( const Coord3& pos_ )
{
    const Coord3 pos = transformation ? transformation->transform( pos_ ) 
				      : pos_;
    textpos->translation.setValue( pos.x, pos.y, pos.z );
}


Coord3 Text::position() const
{
    SbVec3f pos = textpos->translation.getValue();
    Coord3 res( pos[0], pos[1], pos[2] );
    return transformation ? transformation->transformBack( res ) : res;
}


void Text::setSize( float ns )
{
    font->size.setValue( ns );
}


float Text::size() const
{
    return font->size.getValue();
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


void Text::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    par.set( positionstr, position() );
    par.set( justificationstr, (int)justification() );
    par.set( fontsizestr, size() );
    par.set( stringstr, getText() );
}


int Text::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    Coord3 pos;
    if ( !par.get(positionstr,pos) )
	return -1;

    int just = 1;
    par.get( justificationstr, just );

    float fontsz = size();
    par.get( fontsizestr, fontsz );

    BufferString str( "" );
    par.get( stringstr, str );

    setText( str );
    setPosition( pos );
    setSize( fontsz );
    setJustification( (Justification)just );

    return 1;
}


// Text2 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Text2::Text2()
    : Text()
    , text(new SoText2)
{
    addChild( text );
}


void Text2::setText( const char* newtext )
{
    text->string.setValue( newtext );
}


const char* Text2::getText() const
{
    SbString val;
    text->string.get( val );
    static BufferString res;
    res = val.getString();
    return res;
}


void Text2::setJustification( Justification just )
{
    if ( just==Center )
	text->justification.setValue( SoText2::CENTER );
    else if ( just==Left )
	text->justification.setValue( SoText2::LEFT );
    else
	text->justification.setValue( SoText2::RIGHT );
}


Text::Justification Text2::justification() const
{
    if ( text->justification.getValue() == SoText2::CENTER )
	return Center;
    if ( text->justification.getValue() == SoText2::LEFT )
	return Left;

    return Right;
}


}; // namespace visBase
