/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Nov 2003
 RCS:           $Id: viscolorseq.cc,v 1.16 2008-04-08 05:43:52 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "viscolorseq.h"
#include "bufstringset.h"
#include "coltab.h"
#include "coltabsequence.h"
#include "envvars.h"

mCreateFactoryEntry( visBase::ColorSequence );

namespace visBase
{

ColorSequence::ColorSequence()
    : coltabsequence_(*new ColTab::Sequence("Seismics"))
    , change( this )
{
    if ( ColTab::SM().size() == 0 ) // In case 'default' table is not found
	ColTab::SM().get( "Seismics", coltabsequence_ );

    setName( coltabsequence_.name() );
}


ColorSequence::~ColorSequence()
{
    delete &coltabsequence_;
}


void ColorSequence::loadFromStorage( const char* newnm )
{
    ColTab::SM().get( newnm, coltabsequence_ );
    setName( newnm );

    colorsChanged();
}


ColTab::Sequence& ColorSequence::colors()
{ return coltabsequence_; }


const ColTab::Sequence& ColorSequence::colors() const
{ return coltabsequence_; }


void ColorSequence::colorsChanged()
{
    setName( coltabsequence_.name() );
    change.trigger();
}


int ColorSequence::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res != 1 ) return res;

    coltabsequence_.usePar( par );

    if ( ColTab::SM().indexOf(coltabsequence_.name()) >= 0 )
	ColTab::SM().get( coltabsequence_.name(), coltabsequence_ );
    else
	ColTab::SM().set( coltabsequence_ );

    return 1;
}


void ColorSequence::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );
    coltabsequence_.fillPar( par );
}

}; // namespace visBase
