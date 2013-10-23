/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Nov 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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


}; // namespace visBase
