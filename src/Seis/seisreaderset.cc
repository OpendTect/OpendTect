/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisreaderset.h"


bool SeisTrcReaderSet::is2D() const
{
    return isEmpty() || (*this)[0]->is2D();
}


bool SeisTrcReaderSet::prepareWork( Seis::ReadMode rm )
{
    for ( int idx=0; idx<size(); idx++ )
	if ( !(*this)[idx]->prepareWork( rm ) )
	    { errmsg_ = (*this)[idx]->errMsg(); return false; }
    return true;
}


bool SeisTrcReaderSet::getSingle( int irdr, SeisTrcInfo& ti, int& res )
{
    SeisTrcReader& rdr = *(*this)[irdr];
    res = rdr.get( ti );
    if ( res < 0 )
	{ errmsg_ = rdr.errMsg(); return false; }
    else if ( res !=1 )
	return false;
    return true;
}


BinID SeisTrcReaderSet::getBinID( int irdr, const SeisTrcInfo& ti ) const
{
    BinID bid( ti.binID() );
    if ( is2D() )
    {
	bid.inl() = (*this)[irdr]->curLineIdx();
	bid.crl() = ti.trcNr();
    }
    return bid;
}


int SeisTrcReaderSet::getTrcInfos( ObjectSet<SeisTrcInfo>& tis )
{
    if ( isEmpty() )
	return 0;
    if ( tis.size() < size() )
	{ pErrMsg("not enough SeisTrcInfo's provided"); return 0; }

    int res;
    if ( !getSingle(0,*tis[0],res) )
	return res;

    //TODO algo only works if the first cube/line is (one of) the smallest
    const BinID targetbid( getBinID(0,*tis[0]) );
    for ( int irdr=1; irdr<size(); irdr++ )
    {
	while ( true )
	{
	    if ( !getSingle(irdr,*tis[irdr],res) )
		return res;
	    if ( getBinID(irdr,*tis[irdr]) == targetbid )
		break;
	}
    }

    return 1;
}


bool SeisTrcReaderSet::getTrcs( ObjectSet<SeisTrc>& trcs )
{
    if ( isEmpty() )
	return false;
    if ( trcs.size() < size() )
	{ pErrMsg("not enough SeisTrc's provided"); return false; }

    for ( int irdr=0; irdr<size(); irdr++ )
    {
	if ( !(*this)[irdr]->get(*trcs[irdr]) )
	{
	    errmsg_ = (*this)[irdr]->errMsg();
	    return false;
	}
    }

    return true;
}
