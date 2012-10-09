/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Nov 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "viscolorseq.h"
#include "bufstringset.h"
#include "coltab.h"
#include "coltabsequence.h"
#include "envvars.h"

mCreateFactoryEntry( visBase::ColorSequence );

namespace visBase
{

ColorSequence::ColorSequence()
    : coltabsequence_(*new ColTab::Sequence(""))
    , change( this )
{
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
