/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: viscolorseq.cc,v 1.2 2002-03-11 14:37:30 kristofer Exp $";

#include "viscolorseq.h"
#include "colortab.h"


visBase::ColorSequence::ColorSequence()
    : coltab( *new ColorTable )
    , change( this )
{
    coltab.scaleTo( Interval<float>( 0, 1 ) );
    coltab.calcList( 256 );
}


visBase::ColorSequence::~ColorSequence()
{
    delete &coltab;
}


void visBase::ColorSequence::loadFromStorage( const char* newnm )
{
    ColorTable::get( newnm, coltab );
    setName( newnm );

    change.trigger();
}


ColorTable& visBase::ColorSequence::colors() { return coltab; }


const ColorTable& visBase::ColorSequence::colors() const { return coltab; }


void visBase::ColorSequence::colorsChanged()
{
    coltab.scaleTo( Interval<float>( 0, 1 ) );
    coltab.calcList( 256 );
    change.trigger();
}
