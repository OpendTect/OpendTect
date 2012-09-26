/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "coltabindex.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "task.h"
#include "math2.h"


ColTab::IndexedLookUpTable::IndexedLookUpTable( const ColTab::Sequence& seq,
				    int nrc, const ColTab::Mapper* map )
    : seq_(seq)
    , mapper_(map)
    , nrcols_(nrc)
{
    update();
}


void ColTab::IndexedLookUpTable::update()
{
    cols_.erase();
    cols_.setSize( nrcols_, seq_.undefColor() );

    mDefParallelCalc2Pars( ColTabIndexAppl,
	    		   TypeSet<Color>&,cols, const Sequence&,seq )
    mDefParallelCalcBody(
	const float dx = 1.f/(sz_-1)
,
	cols_[idx] = seq_.color( idx*dx )
,
	)

    ColTabIndexAppl appl( cols_.size(), cols_, seq_ );
    appl.execute();
}


Color ColTab::IndexedLookUpTable::colorForIndex( int idx ) const
{
    if ( idx < 0 || idx >= nrcols_ )
	return seq_.undefColor();
    return cols_[idx];
}


int ColTab::IndexedLookUpTable::indexForValue( float v ) const
{
    return ColTab::Mapper::snappedPosition( mapper_, v, nrcols_, -1 );
}
