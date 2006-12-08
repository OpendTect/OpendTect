/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2006
-*/

static const char* rcsID = "$Id: binidsorting.cc,v 1.2 2006-12-08 13:57:02 cvsbert Exp $";

#include "binidsorting.h"
#include "undefval.h"


bool BinIDSorting::isValid( const BinID& prev, const BinID& cur,
			    bool inlsort, bool inlupw, bool crlupw )
{
    if ( mIsUdf(prev.inl) || mIsUdf(prev.crl) )
	return true;
    else if ( mIsUdf(cur.inl) || mIsUdf(cur.crl) )
	return false;

    const int previnl = inlupw ? prev.inl : -prev.inl;
    const int curinl = inlupw ? cur.inl : -cur.inl;
    const int prevcrl = crlupw ? prev.crl : -prev.crl;
    const int curcrl = crlupw ? cur.crl : -cur.crl;
    if ( inlsort )
	return previnl<curinl || (previnl==curinl && prevcrl<=curcrl);
    else
	return prevcrl<curcrl || (prevcrl==curcrl && previnl<=curinl);
}


const char* BinIDSorting::description( bool inlsort, bool inlupw, bool crlupw )
{
    static BufferString ret;
    ret = inlsort ? "In-" : "Cross-";
    ret += "line sorted; In-lines ";
    ret += inlupw ? "in" : "de";
    ret += "crease; Cross-lines ";
    ret += crlupw ? "in" : "de";
    ret += "crease";
    return ret;
}


bool BinIDSorting::isValid( const BinID& prev, const BinID& cur ) const
{
    return isValid( prev, cur, inlSorted(), inlUpward(), crlUpward() );
}


const char* BinIDSorting::description() const
{
    return description( inlSorted(), inlUpward(), crlUpward() );
}


BinIDSortingAnalyser::BinIDSortingAnalyser( BinID bid )
	: prev_(bid)
{
    st_[0] = st_[1] = st_[2] = st_[3] = st_[4] = st_[5] = st_[6] = st_[7] =true;
}


bool BinIDSortingAnalyser::add( const BinID& cur )
{
    if ( mIsUdf(cur.inl) )
	return false;
    if ( mIsUdf(prev_.inl) )
	{ prev_ = cur; return false; }

    int nrvalid = 0;
    for ( int idx=0; idx<8; idx++ )
    {
	if ( st_[idx] )
	{
	    st_[idx] = BinIDSorting(idx).isValid( prev_, cur );
	    if ( st_[idx] ) nrvalid++;
	}
    }

    if ( nrvalid < 1 )
    {
	errmsg_ = "Input data is not sorted on inline or crossline";
	return false;
    }

    prev_ = cur;
    return nrvalid == 1;
}


BinIDSorting BinIDSortingAnalyser::getSorting() const
{
    for ( int idx=0; idx<8; idx++ )
    {
	if ( st_[idx] )
	    return BinIDSorting( idx );
    }

    return BinIDSorting( 0 );
}
