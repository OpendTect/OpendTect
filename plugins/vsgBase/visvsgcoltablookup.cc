/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgcoltablookup.h"

#include "visvsgdata.h"
#include "vsggeocolorlookup.h"

#include "coltabindex.h"
#include "typeset.h"

namespace dGBVis
{

ColTabLookup::ColTabLookup( int nrcolors )
    : lookup_(vsgGeo::ColorLookup::create(nrcolors))
{
    update();
}


ColTabLookup::~ColTabLookup()
{}


void ColTabLookup::setSequence( const ColTab::Sequence& seq )
{
    if ( sequence_ == seq )
	return;

    sequence_ = seq;
    update();
}


int ColTabLookup::nrColors() const
{
    return lookup_->nrColors();
}


vsg::ref_ptr<vsgGeo::ColorLookup> ColTabLookup::colorLookupPtr() const
{
    return lookup_;
}


vsg::vec4 ColTabLookup::colorAt( float pos ) const
{
    return lookup_->colorAt( pos );
}


vsg::vec4 ColTabLookup::undefColor() const
{
    return lookup_->undefColor();
}


void ColTabLookup::update()
{
    const int nrcols = nrColors();
    ColTab::IndexedLookUpTable table( sequence_, nrcols );
    TypeSet<vsg::vec4> colors( nrcols, vsg::vec4() );
    for ( int idx=0; idx<nrcols; idx++ )
	colors[idx] = toVec4( table.colorForIndex(idx) );

    lookup_->setColors( colors.arr(), nrcols, toVec4(sequence_.undefColor()) );
}

} // namespace dGBVis
