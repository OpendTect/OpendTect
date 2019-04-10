/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/

#include "seisvolprovider.h"
#include "cubesubsel.h"
#include "iostrm.h"
#include "file.h"
#include "posinfo.h"
#include "seisfetcher.h"
#include "seisioobjinfo.h"
#include "seispacketinfo.h"
#include "seisseldata.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seisdatapack.h"
#include "survinfo.h"
#include "uistrings.h"


namespace Seis
{

/*\brief Gets required traces from either DataPack or Translator

  Both DataPack and Translator have a natural 'next' postion to go to. What we
  need to tackle is when the selection requires reading from both DataPack
  and Translator. The strategy is to set up a required area, the boundary
  of the selection. The Translator has its own 'current' position, but we
  have our own 'next BinID' anyway.

  There is no glue-ing going on in the Z direction. If the DataPack cannot
  satisfy the Z range requirements, then it will not be used.

  */

class VolFetcher : public Fetcher3D
{ mODTextTranslationClass(Seis::VolFetcher);
public:

    mUseType( PosInfo::CubeData, pos_type );
    typedef pos_type	inl_type;

VolFetcher( VolProvider& p )
    : Fetcher3D(p)
    , trl_(0)
{
}

~VolFetcher()
{
    delete trl_;
}

VolProvider& prov()
{
    return static_cast<VolProvider&>( prov_ );
}

const VolProvider& prov() const
{
    return static_cast<const VolProvider&>( prov_ );
}

const RegularSeisDataPack& dp() const
{
    return *static_cast<const RegularSeisDataPack*>( dp_.ptr() );
}

    void		prepWork();
    bool		isMultiConn() const;
    bool		goTo(const BinID&);
    void		getAt(const BinID&,TraceData&,SeisTrcInfo&);
    void		getCur(TraceData&,SeisTrcInfo&);

    SeisTrcTranslator*	trl_;

    bool		getTranslator(inl_type) const;
    bool		ensureRightTransl(inl_type i=0) const;

};

} // namespace Seis


void Seis::VolFetcher::prepWork()
{
    deleteAndZeroPtr( trl_ );
    getDataPack();
    if ( haveDP() && !dp().sampling().zsamp_.includes(prov().css_.zRange()) )
	dp_ = 0;
}


bool Seis::VolFetcher::isMultiConn() const
{
    const auto* ioobj = prov().ioObj();
    return ioobj && ioobj->isStream() && ioobj->asStream()->isMultiConn();
}


bool Seis::VolFetcher::getTranslator( inl_type inl ) const
{
    const auto* ioobj = prov().ioObj();
    auto* newtrl = ioobj->createTranslator();
    mDynamicCastGet( SeisTrcTranslator*, newtransl, newtrl );
    if ( !newtransl )
    {
	uirv_ = tr("Cannot create appropriate data reader."
	    "\nThis is an installation problem or a data corruption issue.");
	return false;
    }

    Seis::SelData* sd = prov().seldata_ ? prov().seldata_->clone() : 0;
    newtransl->setSelData( sd );

    if ( isMultiConn() )
    {
	const auto& iostrm = *ioobj->asStream();
	const auto connidx = iostrm.fileSpec().nrs_.getIndex( inl );
	iostrm.setConnIdx( connidx );
    }

    auto* conn = ioobj->getConn( true );
    if ( !newtransl->initRead(conn,prov().readmode_) )
	{ uirv_ = newtransl->errMsg(); delete newtransl; return false; }

    if ( prov().haveSelComps() )
    {
	for ( auto icd=0; icd<newtransl->componentInfo().size(); icd++ )
	{
	    if ( !prov().selcomps_.isPresent(icd) )
		newtransl->componentInfo()[icd]->selected_ = false;
	}
    }

    if ( !newtransl->commitSelections() )
	{ uirv_ = newtransl->errMsg(); delete newtransl; return false; }

    delete mSelf().trl_;
    mSelf().trl_ = newtransl;
    return true;
}


bool Seis::VolFetcher::ensureRightTransl( inl_type inl ) const
{
    if ( trl_ )
    {
	if ( !isMultiConn() )
	    return true;
	const auto& iostrm = *prov().ioObj()->asStream();
	const auto connidx = iostrm.curConnIdx();
	const auto conninl = iostrm.fileSpec().nrs_.atIndex(connidx);
	if ( conninl == inl )
	    return true;
    }

    return getTranslator( inl );
}


bool Seis::VolFetcher::goTo( const BinID& bid )
{
    bool atpos = false;
    if ( ensureRightTransl(bid.inl()) )
    {
	if ( haveDP() && dp().sampling().hsamp_.includes( bid ) )
	    atpos = true;
	else if ( trl_ && trl_->goTo(bid) )
	    atpos = true;
    }

    uirv_.setEmpty();
    if ( !atpos )
	uirv_.set( tr("Position not present: %1/%2")
		    .arg( bid.inl() ).arg( bid.crl() ) );
    else
	prov().cdp_ = prov().cubedata_.cubeDataPos( bid );

    return atpos;
}


void Seis::VolFetcher::getAt( const BinID& bid, TraceData& data,
			      SeisTrcInfo& trcinfo )
{
    if ( goTo(bid) )
	getCur( data, trcinfo );
}


void Seis::VolFetcher::getCur( TraceData& data, SeisTrcInfo& info )
{
    if ( !prov().cdp_.isValid() )
	{ uirv_.set( mINTERNAL("Read after end") ); return; }

    const BinID curbid = prov().cubedata_.binID( prov().cdp_ );
    if ( haveDP() && dp().sampling().hsamp_.includes(curbid) )
    {
	const TrcKey tk( curbid );
	dp().fillTraceData( tk, data );
	dp().fillTraceInfo( tk, info );
    }
    else
    {
	ensureRightTransl( curbid.inl() );
	if ( !trl_ )
	    { uirv_.set( mINTERNAL("Read after prepare fail") ); return; }
	else if ( !trl_->readInfo(info) || !trl_->readData(&data) )
	    { uirv_.set( trl_->errMsg() ); return; }
    }

    info.trcKey().setIs2D( false );
}



Seis::VolProvider::VolProvider()
    : fetcher_(*new VolFetcher(*this))
{
}


Seis::VolProvider::~VolProvider()
{
    delete &fetcher_;
}


Seis::Fetcher3D& Seis::VolProvider::fetcher() const
{
    return mNonConst( fetcher_ );
}


const SeisTrcTranslator* Seis::VolProvider::curTransl() const
{
    return fetcher_.trl_;
}


void Seis::VolProvider::getLocationData( uiRetVal& uirv ) const
{
    if ( !fetcher_.isMultiConn() )
    {
	if ( !fetcher_.ensureRightTransl() )
	    { uirv = fetcher_.uirv_; return; }
	if ( !fetcher_.trl_->getGeometryInfo(cubedata_) )
	    uirv = fetcher_.trl_->errMsg();
    }
    else
    {
	const auto fnrs = ioobj_->asStream()->fileSpec().nrs_;
	const auto nrfiles = fnrs.nrSteps() + 1;
	for ( auto idx=0; idx<nrfiles; idx++ )
	{
	    const auto inl = fnrs.atIndex( idx );
	    CubeData linecd;
	    if ( fetcher_.ensureRightTransl(inl)
	      && fetcher_.trl_->getGeometryInfo(linecd) )
		cubedata_.merge( linecd, true );
	}

	if ( cubedata_.isEmpty() )
	    uirv.set( tr("No data in input") );
    }

    if ( fetcher_.trl_ )
	css_.setZRange( fetcher_.trl_->packetInfo().zrg );
}


void Seis::VolProvider::prepWork( uiRetVal& uirv ) const
{
    fetcher_.prepWork();
    uirv = fetcher_.uirv_;
}


void Seis::VolProvider::gtComponentInfo( BufferStringSet& nms,
					 DataType& dt ) const
{
    if ( fetcher_.haveDP() )
    {
	for ( auto icd=0; icd<fetcher_.dp().nrComponents(); icd++ )
	{
	    nms.add( fetcher_.dp().getComponentName(icd) );
	    dt = Seis::UnknownData;
	}
    }
    else if ( !fetcher_.trl_ )
	getFallbackComponentInfo( nms, dt );
    else
    {
	for ( auto icd=0; icd<fetcher_.trl_->componentInfo().size(); icd++ )
	{
	    const SeisTrcTranslator::ComponentData& cd
			= *fetcher_.trl_->componentInfo()[icd];
	    nms.add( cd.name() );
	    dt = fetcher_.trl_->dataType();
	}
    }
}


bool Seis::VolProvider::doGoTo( const BinID& bid, uiRetVal* uirv ) const
{
    if ( fetcher_.goTo(bid) )
	return true;

    if ( uirv )
	*uirv = fetcher_.uirv_;
    return false;
}


void Seis::VolProvider::gtCur( SeisTrc& trc, uiRetVal& uirv ) const
{
    fetcher_.getCur( trc.data(), trc.info() );
    uirv = fetcher_.uirv_;
}


void Seis::VolProvider::gtAt( const BinID& bid, TraceData& data,
				   SeisTrcInfo& trcinfo, uiRetVal& uirv ) const
{
    fetcher_.getAt( bid, data, trcinfo );
    uirv = fetcher_.uirv_;
}
