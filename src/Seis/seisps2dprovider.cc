/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/

#include "seisproviderimpl.h"
#include "posinfo.h"
#include "prestackgather.h"
#include "seisfetcher.h"
#include "seisbuf.h"
#include "seispsioprov.h"
#include "uistrings.h"


namespace Seis
{

/*\brief Gets required traces from 2D data store.  */

class PS2DFetcher : public Fetcher2D
{
public:

    mUseType( Provider,	size_type );
    mUseType( Pos,	GeomID );

			PS2DFetcher( PS2DProvider& p )
			    : Fetcher2D(p)		{}
			~PS2DFetcher()			{ delete rdr_; }

    bool		isPS() const override		{ return true; }
    void		getComponentInfo(BufferStringSet&,
					 DataType&) const override;
    size_type		nrOffsets() const;
    void		prepWork() override;

    bool		setPosition(const Bin2D&) override;
    void		getGather(SeisTrcBuf&);

protected:

    SeisPS2DReader*	rdr_		= nullptr;
    Bin2D		curb2d_;

    bool		ensureRightDataSource(GeomID) const;
    const GatherSetDataPack& dp() const { return gathDP(); }

};

} // namespace Seis


void Seis::PS2DFetcher::getComponentInfo( BufferStringSet& nms,
                                         DataType& dt ) const
{
    return Provider::getFallbackComponentInfo( nms, dt );
}


Seis::PS2DFetcher::size_type Seis::PS2DFetcher::nrOffsets() const
{
    if ( prov_.possiblepositions_.isEmpty() )
	return 1;

    const auto midlidx = prov_.possiblepositions_.size() / 2;
    const auto& ld = *prov_.possiblepositions_.get( midlidx );
    const Bin2D cb2d( GeomID(ld.linenr_), ld.centerNumber() );
    if ( !ensureRightDataSource(cb2d.geomID()) )
	return 1;

    SeisTrcBuf tbuf( true );
    if ( !rdr_->getGath(cb2d.trcNr(),tbuf) || tbuf.isEmpty() )
	return 1;

    return tbuf.size();
}


void Seis::PS2DFetcher::prepWork()
{
}


bool Seis::PS2DFetcher::setPosition( const Bin2D& b2d )
{
    if ( !ensureRightDataSource(b2d.geomID()) )
	return false;

    curb2d_ = b2d;
    return true;
}


void Seis::PS2DFetcher::getGather( SeisTrcBuf& tbuf )
{
    tbuf.deepErase();

    uirv_.setEmpty();
    bool havefilled = false;
    if ( haveDP() )
	havefilled = dp().fillGatherBuf( tbuf, curb2d_ );
    if ( !havefilled && !rdr_->getGath(curb2d_.trcNr(),tbuf) )
	uirv_.set( rdr_->errMsg() );
}


bool Seis::PS2DFetcher::ensureRightDataSource( GeomID gid ) const
{
    if ( rdr_ && rdr_->geomID() == gid )
	return true;

    delete rdr_;
    mSelf().rdr_ = prov_.ioobj_ ? SPSIOPF().get2DReader(*prov_.ioobj_,gid)
				: nullptr;

    if ( !rdr_ )
	uirv_.set( uiStrings::phrCannotOpen( toUiString("%1 [%2]")
			.arg( uiStrings::sDataStore() )
			.arg( gid.name()) ) );
    else
    {
	uirv_.setOK();
	mSelf().handleGeomIDChange( prov2D().lineIdx(gid) );
    }

    return uirv_.isOK();
}


#include "seisproviderimpldefs.h"
mDefPSProvFns( 2D, PS2D )
