/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/

#include "seisproviderimpl.h"
#include "cubedata.h"
#include "prestackgather.h"
#include "seisfetcher.h"
#include "seisbuf.h"
#include "seispsioprov.h"
#include "uistrings.h"


namespace Seis
{

/*\brief Gets required traces from 3D data store.  */

class PS3DFetcher : public Fetcher3D
{
public:

    mUseType( Provider,	size_type );

			PS3DFetcher( PS3DProvider& p )
			    : Fetcher3D(p)		{}
			~PS3DFetcher()			{ delete rdr_; }

    bool		isPS() const override		{ return true; }
    void		getComponentInfo(BufferStringSet&,
					 DataType&) const override;
    void		getPossibleExtents() override;
    size_type		nrOffsets() const;
    void		prepWork() override;

    bool		setPosition(const BinID&) override;
    void		getGather(SeisTrcBuf&);

protected:

    SeisPS3DReader*	rdr_		= nullptr;
    BinID		curbid_;

    bool		ensureReader() const;
    const GatherSetDataPack& dp() const { return gathDP(); }

};

} // namespace Seis


void Seis::PS3DFetcher::getComponentInfo( BufferStringSet& nms,
                                         DataType& dt ) const
{
    return Provider::getFallbackComponentInfo( nms, dt );
}


Seis::PS3DFetcher::size_type Seis::PS3DFetcher::nrOffsets() const
{
    if ( prov_.possiblepositions_.isEmpty() || !ensureReader() )
	return 1;

    const BinID cbid = prov_.as3D()->possibleCubeData().centerPos();
    SeisTrcBuf tbuf( true );
    if ( !rdr_->getGather(cbid,tbuf) || tbuf.isEmpty() )
	return 1;

    return tbuf.size();
}


void Seis::PS3DFetcher::getPossibleExtents()
{
    if ( !ensureReader() )
	return;

    prov_.possiblepositions_ = rdr_->posData();
    prov_.allzsubsels_[0].setOutputZRange( rdr_->getZRange() );
}


void Seis::PS3DFetcher::prepWork()
{
    ensureReader();
    handleGeomIDChange( 0 );
}


bool Seis::PS3DFetcher::setPosition( const BinID& bid )
{
    curbid_.inl() = -1;
    if ( !ensureReader() )
	return false;

    curbid_ = bid;
    return true;
}


void Seis::PS3DFetcher::getGather( SeisTrcBuf& tbuf )
{
    tbuf.deepErase();

    uirv_.setEmpty();
    bool havefilled = false;
    if ( haveDP() )
	havefilled = dp().fillGatherBuf( tbuf, curbid_ );
    if ( !havefilled && !rdr_->getGather(curbid_,tbuf) )
	uirv_.set( rdr_->errMsg() );
}


bool Seis::PS3DFetcher::ensureReader() const
{
    if ( rdr_ )
	return true;

    mSelf().rdr_ = prov_.ioobj_ ? SPSIOPF().get3DReader( *prov_.ioobj_ )
				: nullptr;
    if ( rdr_ )
	uirv_.setOK();
    else
	uirv_.set( uiStrings::phrCannotOpen(uiStrings::sDataStore()) );

    return uirv_.isOK();
}


#include "seisproviderimpldefs.h"
mDefPSProvFns( 3D, PS3D )
