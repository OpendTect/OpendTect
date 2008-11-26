/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Nov 2008
-*/
static const char* rcsID = "$Id: seisposindexer.cc,v 1.3 2008-11-26 12:50:47 cvsbert Exp $";

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


void Seis::PosIndexer::empty()
{
    deepErase( crlsets_ );
    deepErase( idxsets_ );
    maxidx_ = -1;
}


inline static int getIndex( const TypeSet<int>& nrs, int nr )
{
    int ret;
    IdxAble::findPos( nrs.arr(), nrs.size(), nr, -1, ret );
    return ret;
}


inline static int getIndex( const TypeSet<int>& nrs, int nr, bool& present )
{
    int ret;
    present = IdxAble::findPos( nrs.arr(), nrs.size(), nr, -1, ret );
    return ret;
}


int Seis::PosIndexer::getFirstIdxs( const BinID& bid,
				    int& inlidx, int& crlidx ) const
{
    inlidx = is2d_ ? (inls_.isEmpty() ? -1 : 0) : getIndex( inls_, bid.inl );
    if ( inlidx < 0 ) 
	return -1;

    crlidx = getIndex( *crlsets_[inlidx], bid.crl );
    if ( crlidx < 0 )
	return -2;
    return 0;
}


od_int64 Seis::PosIndexer::findFirst( const BinID& bid ) const
{
    int inlidx, crlidx;
    int res = getFirstIdxs( bid, inlidx, crlidx );
    if ( res < 0 ) return res;

    return (*idxsets_[inlidx])[crlidx];
}


od_int64 Seis::PosIndexer::findFirst( int trcnr ) const
{
    return findFirst( BinID(1,trcnr) );
}


od_int64 Seis::PosIndexer::findFirst( const Seis::PosKey& pk, bool wo ) const
{
    int inlidx, crlidx;
    int res = getFirstIdxs( pk.binID(), inlidx, crlidx );
    if ( res < 0 ) return res;

    od_int64 ret = (*idxsets_[inlidx])[crlidx];
    if ( !wo ) return ret;

    for ( ; ret<=maxidx_; ret++ )
    {
	const PosKey curpk( pkl_.key(ret) );
	if ( curpk.isUndef() )
	    continue;
	if ( curpk.binID() != pk.binID() )
	    break;
	if ( curpk.hasOffset(pk.offset()) )
	    return ret;
    }

    return -3;
}


void Seis::PosIndexer::reIndex()
{
    empty();

    const od_int64 sz = pkl_.size();
    od_int64 firstok = 0;
    for ( ; firstok<sz; firstok++ )
    {
	if ( !pkl_.key(firstok).isUndef() )
	    break;
    }
    if ( firstok >= sz )
	return;

    const PosKey prevpk( pkl_.key(firstok) );
    Seis::GeomType gt( sz > 0 ? prevpk.geomType() : Seis::Vol );
    is2d_ = Seis::is2D( gt ); isps_ = Seis::isPS( gt );

    BinID prevbid( mUdf(int), mUdf(int) );
    od_int64 startidx = firstok;
    for ( od_int64 idx=firstok+1; idx<sz; idx++ )
    {
	const PosKey curpk( pkl_.key(idx) );
	const BinID& curbid( curpk.binID() );
	if ( curpk.isUndef() )
	    continue;
	maxidx_ = idx;
	if ( curbid == prevbid )
	    continue;

	add( curbid, idx );
	prevbid = curbid;
    }
}


void Seis::PosIndexer::add( const Seis::PosKey& pk, od_int64 posidx )
{
    bool ispresent = false;
    const int inlidx = is2d_ ? 0 : getIndex( inls_, pk.inLine(), ispresent );
    if ( !ispresent )
    {
	if ( inlidx >= inls_.size() )
	{
	    inls_ += pk.inLine();
	    crlsets_ += new TypeSet<int>;
	    idxsets_ += new TypeSet<od_int64>;
	}
	else
	{
	    inls_.insert( inlidx, pk.inLine() );
	    crlsets_.insertAt( new TypeSet<int>, inlidx );
	    idxsets_.insertAt( new TypeSet<od_int64>, inlidx );
	}
    }

    TypeSet<int>& crls = *crlsets_[inlidx];
    TypeSet<od_int64>& idxs = *idxsets_[inlidx];
    const int crlidx = getIndex( crls, pk.xLine(), ispresent );
    if ( !ispresent )
    {
	if ( crlidx >= crls.size() )
	{
	    crls += pk.xLine();
	    idxs += posidx;
	}
	else
	{
	    crls.insert( crlidx, pk.xLine() );
	    idxs.insert( crlidx, posidx );
	}
    }
}
