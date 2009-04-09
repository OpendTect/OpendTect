/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2007
-*/

static const char* rcsID = "$Id: coltabindex.cc,v 1.9 2009-04-09 00:49:10 cvskris Exp $";

#include "coltabindex.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "task.h"
#include "math2.h"

namespace ColTab
{

class Indexer : public ParallelTask
{
public:
    			Indexer( IndexedLookUpTable& l, int n )
			    : ilut_(l), nrcols_(n), dx_(1./(n-1))
			{
			    l.cols_.erase();
			    l.cols_.setSize( nrcols_, l.seq_.undefColor() );
			}

    bool		doWork( od_int64 start, od_int64 stop, int threadid )
    			{
			    for ( int idx=start; idx<=stop;idx++,addToNrDone(1))
				ilut_.cols_[idx] = ilut_.seq_.color( dx_*idx );
			    return true;
			}

    od_int64		nrIterations() const { return nrcols_; }

protected:

    int			nrcols_;
    float		dx_;
    IndexedLookUpTable&	ilut_;

};

} // namespace ColTab


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
    Indexer idxer( *this, nrcols_ );
    idxer.execute();
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
