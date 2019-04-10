/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/

#include "seisps3dprovider.h"
#include "cubesubsel.h"
#include "posinfo.h"
#include "prestackgather.h"
#include "seisfetcher.h"
#include "seisbuf.h"
#include "seispsioprov.h"
#include "uistrings.h"


namespace Seis
{

/*\brief Gets required traces from 3D data store.  */

class PS3DFetcher : public Fetcher3D
{ mODTextTranslationClass(Seis::PS3DFetcher);
public:

PS3DFetcher( PS3DProvider& p )
    : Fetcher3D(p)
{
}

~PS3DFetcher()
{
    delete rdr_;
}

PS3DProvider& prov()
{
    return static_cast<PS3DProvider&>( prov_ );
}

const PS3DProvider& prov() const
{
    return static_cast<const PS3DProvider&>( prov_ );
}

const GatherSetDataPack& dp() const
{
    return *static_cast<const GatherSetDataPack*>( dp_.ptr() );
}

    bool		mkReader();
    void		prepWork();
    void		getAt(const BinID&,SeisTrcBuf&);
    void		getCur(SeisTrcBuf&);

    SeisPS3DReader*	rdr_		= nullptr;

};

} // namespace Seis


bool Seis::PS3DFetcher::mkReader()
{
    delete rdr_;
    rdr_ = prov().ioobj_ ? SPSIOPF().get3DReader( *prov().ioobj_ ) : 0;
    if ( rdr_ )
	uirv_.setOK();
    else
	uirv_.set( uiStrings::phrCannotOpen(uiStrings::sDataStore()) );
    return uirv_.isOK();
}


void Seis::PS3DFetcher::prepWork()
{
    getDataPack();
}


void Seis::PS3DFetcher::getAt( const BinID& bid, SeisTrcBuf& tbuf )
{
    tbuf.deepErase();

    uirv_.setEmpty();
    bool havefilled = false;
    if ( haveDP() )
	havefilled = dp().fillGatherBuf( tbuf, bid );

    if ( !havefilled && !rdr_->getGather(bid,tbuf) )
	uirv_.set( rdr_->errMsg() );
}



Seis::PS3DProvider::PS3DProvider()
    : fetcher_(*new PS3DFetcher(*this))
{
}


Seis::PS3DProvider::~PS3DProvider()
{
    delete &fetcher_;
}


Seis::Fetcher3D& Seis::PS3DProvider::fetcher() const
{
    return mNonConst( fetcher_ );
}


void Seis::PS3DProvider::getLocationData( uiRetVal& uirv ) const
{
    if ( !fetcher_.mkReader() )
	uirv = fetcher_.uirv_;
    else
    {
	cubedata_ = fetcher_.rdr_->posData();
	css_.setZRange( fetcher_.rdr_->getZRange() );
    }
}


void Seis::PS3DProvider::prepWork( uiRetVal& ) const
{
    fetcher_.getDataPack();
    if ( fetcher_.haveDP() && !fetcher_.dp().zRange().includes(css_.zRange()) )
	fetcher_.dp_ = 0;
}


int Seis::PS3DProvider::gtNrOffsets() const
{
    if ( cubedata_.isEmpty() )
	return 1;

    const int linenr = cubedata_.size() / 2;
    const auto& ld = *cubedata_.get( linenr );
    const int segnr = ld.segments_.size() / 2;
    const BinID bid( ld.linenr_, ld.segments_[segnr].center() );
    SeisTrcBuf tbuf( true );
    if ( !fetcher_.rdr_->getGather(bid,tbuf) || tbuf.isEmpty() )
	return 1;

    return tbuf.size();
}


bool Seis::PS3DProvider::doGoTo( const BinID& bid, uiRetVal* uirv ) const
{
    const CubeDataPos cdp = cubedata_.cubeDataPos( bid );
    if ( cdp.isValid() )
	{ cdp_ = cdp; return true; }

    if ( uirv )
	*uirv = tr("No gather at %1/%2").arg( bid.inl() ).arg( bid.crl() );
    return false;
}


void Seis::PS3DProvider::gtCurGather( SeisTrcBuf& tbuf, uiRetVal& uirv ) const
{
    fetcher_.getAt( curBinID(), tbuf );
    uirv = fetcher_.uirv_;
}


void Seis::PS3DProvider::gtGatherAt( const BinID& bid, SeisTrcBuf& tbuf,
				     uiRetVal& uirv ) const
{
    fetcher_.getAt( bid, tbuf );
    uirv = fetcher_.uirv_;
}
