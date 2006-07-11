/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Nov 2003
 RCS:           $Id: viscolorseq.cc,v 1.13 2006-07-11 17:08:50 cvsbert Exp $
________________________________________________________________________

-*/

#include "viscolorseq.h"
#include "colortab.h"
#include "envvars.h"

mCreateFactoryEntry( visBase::ColorSequence );

namespace visBase
{

ColorSequence::ColorSequence()
    : coltab( *new ColorTable("") )
    , change( this )
{
    if ( coltab.cvs_.size() == 0 ) // In case 'default' table is not found
	ColorTable::get( "Red-White-Blue", coltab );

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
