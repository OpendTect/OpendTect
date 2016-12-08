/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/

#include "seisps3dprovider.h"
#include "seisfetcher.h"
#include "seistrctr.h"
#include "uistrings.h"
#include "seispacketinfo.h"
#include "seisselection.h"
#include "seispsioprov.h"
#include "posinfo.h"


namespace Seis
{

/*\brief Gets required traces from 3D data store.  */

class PS3DFetcher : public Fetcher3D
{ mODTextTranslationClass(Seis::PS3DFetcher);
public:

PS3DFetcher( PS3DProvider& p )
    : Fetcher3D(p)
    , rdr_(0)
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

    void		reset();
    TrcKeyZSampling	getDefaultCS() const;

    void		openStore();
    void		moveNextBinID();
    void		getAt(const BinID&,SeisTrcBuf&);
    static SeisPS3DReader*  getReader(const IOObj&);

    void		get(const BinID&,SeisTrcBuf&);
    void		getNext(SeisTrcBuf&);

    SeisPS3DReader*	rdr_;
    PosInfo::CubeDataIterator*	cditer_;
    bool		atend_;

};

} // namespace Seis


void Seis::PS3DFetcher::reset()
{
    Fetcher3D::reset();
    delete cditer_; cditer_ = 0;
    delete rdr_; rdr_ = 0;
    atend_ = false;

    openStore();
}


TrcKeyZSampling Seis::PS3DFetcher::getDefaultCS() const
{
    TrcKeyZSampling ret;
    if ( !cditer_ || cditer_->cd_.isEmpty() )
	ret = TrcKeyZSampling( true );
    else
    {
	StepInterval<int> inlrg, crlrg;
	cditer_->cd_.getInlRange( inlrg, true );
	cditer_->cd_.getCrlRange( crlrg, true );
	ret.hsamp_.setInlRange( inlrg );
	ret.hsamp_.setCrlRange( crlrg );
    }
    return ret;
}



SeisPS3DReader* Seis::PS3DFetcher::getReader( const IOObj& ioobj )
{
    return SPSIOPF().get3DReader( ioobj );
}


void Seis::PS3DFetcher::openStore()
{
    if ( !fillIOObj() )
	return;

    rdr_ = getReader( *ioobj_ );
    if ( !rdr_ )
    {
	uirv_ = tr( "Cannot find a reader for this type of data store" );
	return;
    }

    cditer_ = new PosInfo::CubeDataIterator( rdr_->posData() );
    moveNextBinID();
}


void Seis::PS3DFetcher::moveNextBinID()
{
    while ( true )
    {
	atend_ = !cditer_->next( nextbid_ );
	if ( atend_ || !prov().seldata_ || prov().seldata_->isOK(nextbid_) )
	    break;
    }
}


void Seis::PS3DFetcher::getAt( const BinID& bid, SeisTrcBuf& tbuf )
{
    if ( !rdr_ )
    {
	if ( uirv_.isOK() )
	    uirv_.set( uiStrings::phrInternalError("PS3D Reader not created") );
	return;
    }

    nextbid_ = bid;
    if ( !rdr_->getGather(nextbid_,tbuf) )
	uirv_.set( rdr_->errMsg() );
    else
	moveNextBinID();
}


void Seis::PS3DFetcher::get( const BinID& bid, SeisTrcBuf& tbuf )
{
    getAt( bid, tbuf );
}


void Seis::PS3DFetcher::getNext( SeisTrcBuf& tbuf )
{
    if ( atend_ )
	uirv_.set( uiStrings::sFinished() );
    else
	getAt( nextbid_, tbuf );
}



Seis::PS3DProvider::PS3DProvider()
    : fetcher_(*new PS3DFetcher(*this))
{
}


Seis::PS3DProvider::~PS3DProvider()
{
    delete &fetcher_;
}


SeisPS3DReader* Seis::PS3DProvider::mkReader() const
{
    PtrMan<IOObj> ioobj = fetcher_.getIOObj();
    return ioobj ? PS3DFetcher::getReader( *ioobj ) : 0;
}


BufferStringSet Seis::PS3DProvider::getComponentInfo() const
{
    BufferStringSet compnms;
    compnms.add( "Data" );
    return compnms;
}


ZSampling Seis::PS3DProvider::getZSampling() const
{
    ZSampling ret;
    if ( fetcher_.rdr_ )
	ret = fetcher_.rdr_->getZRange();
    else
    {
	PtrMan<SeisPS3DReader> rdr = mkReader();
	ret = rdr ? rdr->getZRange() : TrcKeyZSampling(true).zsamp_;
    }
    return ret;
}


TrcKeySampling Seis::PS3DProvider::getHSampling() const
{
    TrcKeySampling ret;
    if ( fetcher_.cditer_ )
	ret = fetcher_.getDefaultCS().hsamp_;
    else
    {
	PtrMan<SeisPS3DReader> rdr = mkReader();
	if ( !rdr )
	    ret = TrcKeyZSampling(true).hsamp_;
	else
	{
	    const PosInfo::CubeData& cd = rdr->posData();
	    StepInterval<int> rg;
	    cd.getInlRange( rg ); ret.setInlRange( rg );
	    cd.getCrlRange( rg ); ret.setCrlRange( rg );
	}
    }
    return ret;
}


void Seis::PS3DProvider::getGeometryInfo( PosInfo::CubeData& cd ) const
{
    if ( fetcher_.cditer_ )
	cd = fetcher_.cditer_->cd_;
    else
    {
	cd.setEmpty();
	PtrMan<SeisPS3DReader> rdr = mkReader();
	if ( rdr )
	    cd = rdr->posData();
	else
	{
	    const TrcKeySampling hs = TrcKeyZSampling(true).hsamp_;
	    cd.generate( hs.start_, hs.stop_, hs.step_ );
	}
    }
}


void Seis::PS3DProvider::doUsePar( const IOPar& iop, uiRetVal& uirv )
{
    uirv.set( mTODONotImplPhrase() );
}


void Seis::PS3DProvider::doReset( uiRetVal& uirv ) const
{
    fetcher_.reset();
    uirv = fetcher_.uirv_;
}


void Seis::PS3DProvider::doGetNextGather( SeisTrcBuf& tbuf,
					  uiRetVal& uirv ) const
{
    fetcher_.getNext( tbuf );
    uirv = fetcher_.uirv_;
}


void Seis::PS3DProvider::doGetGather( const TrcKey& trcky, SeisTrcBuf& tbuf,
				  uiRetVal& uirv ) const
{
    fetcher_.get( trcky.binID(), tbuf );
    uirv = fetcher_.uirv_;
}
