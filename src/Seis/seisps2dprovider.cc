/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/

#include "seisps2dprovider.h"
#include "posinfo2d.h"
#include "prestackgather.h"
#include "seisfetcher.h"
#include "seisbuf.h"
#include "seispsioprov.h"


namespace Seis
{

/*\brief Gets required traces from 2D data store.  */

class PS2DFetcher : public Fetcher2D
{ mODTextTranslationClass(Seis::PS2DFetcher);
public:

PS2DFetcher( PS2DProvider& p )
    : Fetcher2D(p)
{
}

~PS2DFetcher()
{
    delete rdr_;
}

PS2DProvider& prov()
{
    return static_cast<PS2DProvider&>( prov_ );
}

const PS2DProvider& prov() const
{
    return static_cast<const PS2DProvider&>( prov_ );
}

const GatherSetDataPack& dp() const
{
    return *static_cast<const GatherSetDataPack*>( dp_.ptr() );
}

    bool		mkReader(GeomID);
    void		prepWork();
    bool		goTo(GeomID,trcnr_type);
    void		getAt(GeomID,trcnr_type,SeisTrcBuf&);
    void		getCur(SeisTrcBuf&);

    SeisPS2DReader*	rdr_		= nullptr;

};

} // namespace Seis


bool Seis::PS2DFetcher::mkReader( GeomID gid )
{
    delete rdr_;
    rdr_ = prov().ioobj_ ? SPSIOPF().get2DReader( *prov().ioobj_, gid ) : 0;
    if ( !rdr_ )
	uirv_.set( tr("No data for %1").arg( nameOf(gid) ) );
    else
	{ uirv_.setOK(); getDataPack(); }
    return uirv_.isOK();
}


void Seis::PS2DFetcher::prepWork()
{
    getDataPack();
}


void Seis::PS2DFetcher::getAt( GeomID gid, trcnr_type tnr, SeisTrcBuf& tbuf )
{
    tbuf.deepErase();
    const BinID bid( gid.lineNr(), tnr );
    if ( !selectPosition(gid,tnr) )
        return;

    if ( rdr_ && rdr_->geomID() != gid )
	deleteAndZeroPtr( rdr_ );
    if ( !rdr_ && !mkReader(gid) )
	return;

    uirv_.setEmpty();
    bool havefilled = false;
    if ( haveDP() )
	havefilled = dp().fillGatherBuf( tbuf, bid );

    if ( !havefilled && !rdr_->getGather(bid,tbuf) )
	uirv_.set( rdr_->errMsg() );
}



Seis::PS2DProvider::PS2DProvider()
    : fetcher_(*new PS2DFetcher(*this))
{
}


Seis::PS2DProvider::~PS2DProvider()
{
    delete &fetcher_;
}


Seis::Fetcher2D& Seis::PS2DProvider::fetcher() const
{
    return mNonConst( fetcher_ );
}


void Seis::PS2DProvider::prepWork( uiRetVal& ) const
{
    fetcher_.getDataPack();
}


int Seis::PS2DProvider::gtNrOffsets() const
{
    if ( l2dds_.isEmpty() )
	return 1;
    if ( !fetcher_.rdr_ && !fetcher_.mkReader( l2dds_.first()->geomID() ) )
	return 1;

    const auto& rdr = *fetcher_.rdr_;
    const auto& posns = rdr.posData().positions();
    const auto nrtrcs = posns.size();
    if ( posns.isEmpty() )
	return 1;

    SeisTrcBuf tbuf( true );
    if ( !rdr.getGath(posns.get(nrtrcs/2).nr_,tbuf) || tbuf.isEmpty() )
	return 1;

    return tbuf.size();
}


bool Seis::PS2DProvider::doGoTo( GeomID gid, trcnr_type tnr,
				 uiRetVal* uirv ) const
{
    if ( !fetcher_.selectPosition(gid,tnr) )
    {
	if ( uirv )
	    *uirv = fetcher_.uirv_;
	return false;
    }
    return true;
}


void Seis::PS2DProvider::gtCurGather( SeisTrcBuf& tbuf, uiRetVal& uirv ) const
{
    fetcher_.getAt( curGeomID(), curTrcNr(), tbuf );
    uirv = fetcher_.uirv_;
}


void Seis::PS2DProvider::gtGatherAt( GeomID gid, trcnr_type tnr,
				     SeisTrcBuf& tbuf, uiRetVal& uirv ) const
{
    fetcher_.getAt( gid, tnr, tbuf );
    uirv = fetcher_.uirv_;
}
