/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: viscolorseq.cc,v 1.5 2002-04-26 07:40:09 kristofer Exp $";

#include "viscolorseq.h"
#include "colortab.h"


visBase::ColorSequence::ColorSequence()
    : coltab( *new ColorTable )
    , change( this )
{
    coltab.scaleTo( Interval<float>( 0, 1 ) );
    coltab.calcList( 256 );
    setName( coltab.name() );
}


visBase::ColorSequence::~ColorSequence()
{
    delete &coltab;
}


void visBase::ColorSequence::loadFromStorage( const char* newnm )
{
    ColorTable::get( newnm, coltab );
    setName( newnm );

    colorsChanged();
}


ColorTable& visBase::ColorSequence::colors() { return coltab; }


const ColorTable& visBase::ColorSequence::colors() const { return coltab; }


void visBase::ColorSequence::colorsChanged()
{
    setName( coltab.name() );
    coltab.scaleTo( Interval<float>( 0, 1 ) );
    coltab.calcList( 256 );
    change.trigger();
}


int visBase::ColorSequence::usePar( const IOPar& par )
{
    coltab.usePar( par );
    return 1;
}


void visBase::ColorSequence::fillPar( IOPar& par ) const
{
    coltab.fillPar( par );
}
