/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: viscolorseq.cc,v 1.11 2005-02-04 14:31:34 kristofer Exp $";

#include "viscolorseq.h"
#include "colortab.h"

namespace visBase
{
mCreateFactoryEntry( ColorSequence );


ColorSequence::ColorSequence()
    : coltab( *new ColorTable )
    , change( this )
{
    coltab.scaleTo( Interval<float>( 0, 1 ) );
    coltab.calcList( 255 );
    setName( coltab.name() );
}


ColorSequence::~ColorSequence()
{
    delete &coltab;
}


void ColorSequence::loadFromStorage( const char* newnm )
{
    ColorTable::get( newnm, coltab );
    setName( newnm );

    colorsChanged();
}


ColorTable& ColorSequence::colors() { return coltab; }


const ColorTable& ColorSequence::colors() const { return coltab; }


void ColorSequence::colorsChanged()
{
    setName( coltab.name() );
    coltab.scaleTo( Interval<float>( 0, 1 ) );
    coltab.calcList( 255 );
    change.trigger();
}


int ColorSequence::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res != 1 ) return res;

    coltab.usePar( par );
    return 1;
}


void ColorSequence::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );
    coltab.fillPar( par );
}

}; // namespace visBase
