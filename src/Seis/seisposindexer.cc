/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Nov 2008
-*/
static const char* rcsID = "$Id: seisposindexer.cc,v 1.1 2008-11-25 16:40:14 cvsbert Exp $";

#include "seisposindexer.h"
#include "idxable.h"

Seis::PosIndexer::PosIndexer( const Seis::PosKeyList& pkl )
    : pkl_(pkl)
{
    reIndex();
}


Seis::PosIndexer::~PosIndexer()
{
    empty();
}


static int findNr( const TypeSet<int>& nrs, int nr )
{
    int ret;
    IdxAble::findPos( nrs.arr(), nrs.size(), nr, -1, ret );
    return ret;
}


od_int64 Seis::PosIndexer::indexOf( const Seis::PosKey& pk, int offsnr ) const
{
    const int inlidx = is2d_ ? (crlsets_.isEmpty() ? -1 : 0)
					 : findNr( inls_, pk.inLine() );
    if ( inlidx < 0 ) 
	return -1;

    const int crlidx = findNr( *crlsets_[inlidx], pk.xLine() );
    if ( crlidx < 0 )
	return -1;

    const TypeSet<od_int64>& idxs = *((*idxsets_[inlidx])[crlidx]);
    if ( !isps_ || mIsUdf(pk.offset()) )
	return idxs[0];

    if ( offsnr >= 0 )
	return offsnr >= idxs.size() ? -1 : idxs[offsnr];

    const TypeSet<float>& offs = *((*offssets_[inlidx])[crlidx]);
    for ( int idx=0; idx<idxs.size(); idx++ )
    {
	if ( mIsEqual(offs[idx],pk.offset(),1e-4) )
	    return idxs[idx];
    }

    return -1;
}


void Seis::PosIndexer::reIndex()
{
    empty();

    const od_uint64 sz = pkl_.size();
    Seis::GeomType gt( sz > 0 ? pkl_.key(0).geomType() : Seis::Vol );
    is2d_ = Seis::is2D( gt ); isps_ = Seis::isPS( gt );

    for ( od_int64 idx=0; idx<sz; idx++ )
	add( pkl_.key(idx), idx );
}


void Seis::PosIndexer::add( const Seis::PosKey& pk, od_int64 idx )
{
    //TODO implement
}
