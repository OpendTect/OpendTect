/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: viscolorseq.cc,v 1.10 2003-11-07 12:22:02 bert Exp $";

#include "viscolorseq.h"
#include "colortab.h"

mCreateFactoryEntry( visBase::ColorSequence );


visBase::ColorSequence::ColorSequence()
    : coltab( *new ColorTable )
    , change( this )
{
    coltab.scaleTo( Interval<float>( 0, 1 ) );
    coltab.calcList( 255 );
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
    coltab.calcList( 255 );
    change.trigger();
}


int visBase::ColorSequence::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res != 1 ) return res;

    coltab.usePar( par );
    return 1;
}


void visBase::ColorSequence::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );
    coltab.fillPar( par );
}
