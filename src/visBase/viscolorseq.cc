/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Nov 2003
 RCS:           $Id: viscolorseq.cc,v 1.14 2006-09-05 20:53:06 cvskris Exp $
________________________________________________________________________

-*/

#include "viscolorseq.h"
#include "colortab.h"
#include "envvars.h"

mCreateFactoryEntry( visBase::ColorSequence );

namespace visBase
{

ColorSequence::ColorSequence()
    : coltab_( *new ColorTable("") )
    , change( this )
{
    if ( coltab_.cvs_.size() == 0 ) // In case 'default' table is not found
	ColorTable::get( "Red-White-Blue", coltab_ );

    coltab_.scaleTo( Interval<float>( 0, 1 ) );
    coltab_.calcList( 255 );
    setName( coltab_.name() );
}


ColorSequence::~ColorSequence()
{
    delete &coltab_;
}


void ColorSequence::loadFromStorage( const char* newnm )
{
    ColorTable::get( newnm, coltab_ );
    setName( newnm );

    colorsChanged();
}


ColorTable& ColorSequence::colors() { return coltab_; }


const ColorTable& ColorSequence::colors() const { return coltab_; }


void ColorSequence::colorsChanged()
{
    setName( coltab_.name() );
    coltab_.scaleTo( Interval<float>( 0, 1 ) );
    coltab_.calcList( 255 );
    change.trigger();
}


int ColorSequence::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res != 1 ) return res;

    coltab_.usePar( par );
    return 1;
}


void ColorSequence::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );
    coltab_.fillPar( par );
}

}; // namespace visBase
