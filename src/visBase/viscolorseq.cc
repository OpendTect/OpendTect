/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: viscolorseq.cc,v 1.1 2002-03-11 10:46:03 kristofer Exp $";

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


ColorTable& visBase::ColorSequence::colors() { return coltab; }


const ColorTable& visBase::ColorSequence::colors() const { return coltab; }


void visBase::ColorSequence::colorsChanged()
{
    coltab.scaleTo( Interval<float>( 0, 1 ) );
    coltab.calcList( 256 );
    change.trigger();
}
