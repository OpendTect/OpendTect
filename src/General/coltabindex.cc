/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2007
-*/

static const char* rcsID = "$Id: coltabindex.cc,v 1.11 2012-03-08 13:15:14 cvskris Exp $";

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
    mParallelApplyToAll( ColTab::IndexedLookUpTable, (*this),
	    self.cols_.size(),
	    const float dx = 1./(self.cols_.size()-1),
	    self.cols_[idx] = self.seq_.color( idx*dx ),
	    , )
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
