/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2006
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "binidsorting.h"
#include "undefval.h"


bool BinIDSorting::isValid( bool is2d, const BinID& prev, const BinID& cur,
			    bool inlsort, bool inlupw, bool crlupw )
{
    if ( mIsUdf(prev.crl) || (!is2d && mIsUdf(prev.inl)) )
	return true;
    else if ( mIsUdf(cur.crl) || (!is2d && mIsUdf(cur.inl)) )
	return false;

    if ( is2d )
    {
	if ( !mIsUdf(cur.inl) && !mIsUdf(cur.inl) && prev.inl != cur.inl )
	    return true;

	return (crlupw && prev.crl <= cur.crl)
	    || (!crlupw && prev.crl >= cur.crl);
    }

    const int previnl = inlupw ? prev.inl : -prev.inl;
    const int curinl = inlupw ? cur.inl : -cur.inl;
    const int prevcrl = crlupw ? prev.crl : -prev.crl;
    const int curcrl = crlupw ? cur.crl : -cur.crl;
    if ( inlsort )
	return previnl<curinl || (previnl==curinl && prevcrl<=curcrl);
    else
	return prevcrl<curcrl || (prevcrl==curcrl && previnl<=curinl);
}


const char* BinIDSorting::description( bool is2d,
				       bool inlsort, bool inlupw, bool crlupw )
{
    static BufferString ret;
    if ( is2d )
    {
	ret = "Trace numbers go ";
	ret += crlupw ? "up" : "down";
    }
    else
    {
	if ( inlsort )
	    { ret = "In-line sorted ("; ret += inlupw ? "in" : "de"; }
	else
	    { ret = "Cross-line sorted ("; ret += crlupw ? "in" : "de"; }
	ret += "creasing); ";
	if ( inlsort )
	    ret += crlupw ? "Cross-lines go up" : "Cross-lines go down";
	else
	    ret += inlupw ? "In-lines go up" : "In-lines go down";
    }
    return ret;
}


bool BinIDSorting::isValid( const BinID& prev, const BinID& cur ) const
{
    return isValid( is2d_, prev, cur, inlSorted(), inlUpward(), crlUpward() );
}


const char* BinIDSorting::description() const
{
    return description( is2d_, inlSorted(), inlUpward(), crlUpward() );
}


void BinIDSorting::set( bool inl, bool inlupw, bool crlupw )
{
    state_ = is2d_
	   ? (crlupw ? 4 : 6)
	   : 4 * (inl ? 0 : 1) + (inlupw ? 0 : 1) + 2 * (crlupw ? 0 : 1);
}


BinIDSortingAnalyser::BinIDSortingAnalyser( bool is2d )
	: prev_(mUdf(int),mUdf(int))
	, is2d_(is2d)
{
    st_[0] = st_[1] = st_[2] = st_[3] = st_[4] = st_[5] = st_[6] = st_[7] =true;
}


bool BinIDSortingAnalyser::add( const BinID& cur )
{
    if ( mIsUdf(cur.inl) )
	return false;
    if ( mIsUdf(prev_.inl) || mIsUdf(prev_.crl) )
	{ prev_ = cur; return false; }

    bool rv = false;
    if ( is2d_ )
    {
	if ( !mIsUdf(prev_.crl)
	  && prev_.inl == cur.inl
	  && prev_.crl != cur.crl )
	{
	    st_[0] = prev_.crl < cur.crl;
	    rv = true;
	}
    }
    else
    {
	int nrvalid = 0;
	int firststillvalid = -1;
	for ( int idx=0; idx<8; idx++ )
	{
	    if ( st_[idx] )
	    {
		firststillvalid = idx;
		st_[idx] = BinIDSorting(false,idx).isValid( prev_, cur );
		if ( st_[idx] ) nrvalid++;
	    }
	}

	if ( nrvalid < 1 )
	{
	    errmsg_ = "Input data is not sorted on inline or crossline";
	    if ( firststillvalid >= 0 )
		st_[firststillvalid] = true;
	    return false;
	}
	rv = nrvalid == 1;
    }

    prev_ = cur;
    return rv;
}


BinIDSorting BinIDSortingAnalyser::getSorting() const
{
    if ( is2d_ )
	return BinIDSorting( true, st_[0] ? 4 : 6);

    for ( int idx=0; idx<8; idx++ )
    {
	if ( st_[idx] )
	    return BinIDSorting( false, idx );
    }

    return BinIDSorting( false, 0 );
}
