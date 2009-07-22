/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2008
-*/
static const char* rcsID = "$Id: seisposindexer.cc,v 1.7 2009-07-22 16:01:35 cvsbert Exp $";

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


inline static int getIndex( const TypeSet<int>& nrs, int nr, bool& present )
{
    int ret;
    present = IdxAble::findPos( nrs.arr(), nrs.size(), nr, -1, ret );
    return ret;
}


int Seis::PosIndexer::getFirstIdxs( const BinID& bid,
				    int& inlidx, int& crlidx ) const
{
    if ( inls_.isEmpty() )
	return -1;

    bool pres = true;
    inlidx = is2d_ ? 0 : getIndex( inls_, bid.inl, pres );
    if ( !pres )
	{ crlidx = -1; return -1; }

    crlidx = getIndex( *crlsets_[inlidx], bid.crl, pres );
    if ( !pres )
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
    inlrg_.start = inlrg_.stop = crlrg_.start = crlrg_.stop = 0;
    offsrg_.start = offsrg_.stop = 0;

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
    add( prevpk.binID(), firstok );
    Seis::GeomType gt( sz > 0 ? prevpk.geomType() : Seis::Vol );
    is2d_ = Seis::is2D( gt ); isps_ = Seis::isPS( gt );
    inlrg_.start = inlrg_.stop = is2d_ ? 1 : prevpk.inLine();
    crlrg_.start = crlrg_.stop = prevpk.xLine();
    offsrg_.start = offsrg_.stop = isps_ ? prevpk.offset() : 0;

    BinID prevbid( mUdf(int), mUdf(int) );
    od_int64 startidx = firstok;
    for ( od_int64 idx=firstok+1; idx<sz; idx++ )
    {
	const PosKey curpk( pkl_.key(idx) );
	const BinID& curbid( curpk.binID() );
	if ( curpk.isUndef() )
	    continue;

	maxidx_ = idx;
	if ( isps_ ) offsrg_.include( curpk.offset() );
	if ( curbid == prevbid )
	    continue;

	add( curbid, idx );
	if ( !is2d_ ) inlrg_.include( curpk.inLine() );
	crlrg_.include( curpk.xLine() );
	prevbid = curbid;
    }
}


void Seis::PosIndexer::add( const Seis::PosKey& pk, od_int64 posidx )
{
    bool ispresent = !inls_.isEmpty();
    int inlidx = is2d_ ? 0 : getIndex( inls_, pk.inLine(), ispresent );
    if ( !ispresent )
    {
	if ( inlidx >= inls_.size() - 1 )
	{
	    inls_ += pk.inLine();
	    crlsets_ += new TypeSet<int>;
	    idxsets_ += new TypeSet<od_int64>;
	    inlidx = inls_.size() - 1;
	}
	else
	{
	    inlidx++;
	    inls_.insert( inlidx, pk.inLine() );
	    crlsets_.insertAt( new TypeSet<int>, inlidx );
	    idxsets_.insertAt( new TypeSet<od_int64>, inlidx );
	}
    }

    TypeSet<int>& crls = *crlsets_[inlidx];
    TypeSet<od_int64>& idxs = *idxsets_[inlidx];
    int crlidx = getIndex( crls, pk.xLine(), ispresent );
    if ( ispresent ) return;

    if ( crlidx >= crls.size()-1 )
    {
	crls += pk.xLine();
	idxs += posidx;
    }
    else
    {
	crlidx++;
	crls.insert( crlidx, pk.xLine() );
	idxs.insert( crlidx, posidx );
    }
}
