/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2007
-*/

static const char* rcsID = "$Id: coltabindex.cc,v 1.3 2007-09-26 11:15:46 cvsbert Exp $";

#include "coltabindex.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "basictask.h"

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

    bool		doWork( int start, int stop, int threadid )
    			{
			    for ( int idx=start; idx<=stop; idx++ )
				ilut_.cols_[idx] = ilut_.seq_.color( dx_*idx );
			    return true;
			}

    int			nrTimes() const { return nrcols_; }

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


int ColTab::IndexedLookUpTable::indexForValue( float v ) const
{
    float ret = mapper_ ? mapper_->position( v ) : v;
    ret *= nrcols_;
    if ( ret > nrcols_- 0.9 ) ret = nrcols_- 0.9;
    return (int)ret;
}
