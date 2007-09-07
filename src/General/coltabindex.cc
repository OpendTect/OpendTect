/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2007
-*/

static const char* rcsID = "$Id: coltabindex.cc,v 1.1 2007-09-07 11:21:01 cvsbert Exp $";

#include "coltabindex.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "basictask.h"

/*

namespace ColTab
{

class Indexer : public ParallelTask
{
public:
    			Indexer( int nt, float x0, float dx,
			         const ColorTable& ct, Color* res )
			    : nrtimes( nt )
			    , x0_( x0 )
			    , dx_( dx )
			    , ct_( ct )
			    , res_( res )
			{}

    bool		doWork( int start, int stop, int threadid )
    			{
			    for ( int idx=start; idx<=stop; idx++ )
				res_[idx] = ct_.color( x0_+dx_*idx );
			    return true;
			}

    int			nrTimes() const { return nrtimes; }

protected:

    int			nrtimes;
    float		x0_;
    float		dx_;
    const ColorTable&	ct_;
    Color*		res_;

};

} // namespace ColTab

*/

ColTab::IndexedLookUpTable::IndexedLookUpTable( const Sequence& seq,
				    const Mapper& map, int nrc, bool seg )
    : seq_(seq)
    , mapper_(map)
    , nrcols_(nrc)
    , segmentised_(seg)
{
    update();
}


ColTab::IndexedLookUpTable::~IndexedLookUpTable()
{
}


void ColTab::IndexedLookUpTable::update()
{
    // TODO implement
}


Color ColTab::IndexedLookUpTable::color( float v ) const
{
    // TODO return the actual indexed stuff
    return seq_.color( mapper_.position(v) );
}
