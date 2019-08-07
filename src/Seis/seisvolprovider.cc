/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/

#include "seisvolprovider.h"
#include "seisfetcher.h"
#include "iostrm.h"
#include "file.h"
#include "posinfo.h"
#include "seispacketinfo.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seisdatapack.h"
#include "survinfo.h"
#include "uistrings.h"


namespace Seis
{

class VolFetcher : public Fetcher3D
{
public:

    typedef IdxPair::pos_type	inl_type;

			VolFetcher( VolProvider& p )
			    : Fetcher3D(p), trl_(0)	{}
			~VolFetcher()			{ delete trl_; }

    bool		isPS() const override		{ return false; }
    void		getComponentInfo(BufferStringSet&,
					 DataType&) const override;
    void		getPossiblePositions() override;
    void		prepWork() override;
    const STTrl*	curTransl() const override   { return trl_; }

    bool		setPosition(const BinID&) override;
    void		getTrc(TraceData&,SeisTrcInfo&);

protected:

    STTrl*		trl_;
    BinID		dpbid_;

    bool		isMultiConn() const;
    bool		createTranslator(inl_type) const;
    bool		ensureRightTransl(inl_type inl=-1) const;
    const RegularSeisDataPack& dp() const
		{ return *static_cast<const RegularSeisDataPack*>(dp_.ptr()); }

};

} // namespace Seis


void Seis::VolFetcher::getComponentInfo( BufferStringSet& nms,
					 DataType& dt ) const
{
    if ( haveDP() )
    {
	for ( auto icd=0; icd<dp().nrComponents(); icd++ )
	{
	    nms.add( dp().getComponentName(icd) );
	    dt = Seis::UnknownData;
	}
    }
    else
    {
	ensureRightTransl();
	if ( !trl_ )
	    Provider::getFallbackComponentInfo( nms, dt );
	else
	{
	    for ( auto icd=0; icd<trl_->componentInfo().size(); icd++ )
	    {
		const SeisTrcTranslator::ComponentData& cd
			    = *trl_->componentInfo()[icd];
		nms.add( cd.name() );
		dt = trl_->dataType();
	    }
	}
    }
}


void Seis::VolFetcher::getPossiblePositions()
{
    if ( !isMultiConn() )
    {
	if ( !ensureRightTransl() )
	    return;
	if ( !trl_->getGeometryInfo(prov_.possiblepositions_) )
	    uirv_ = trl_->errMsg();
    }
    else
    {
	const auto fnrs = prov_.ioobj_->asStream()->fileSpec().nrs_;
	const auto nrfiles = fnrs.nrSteps() + 1;
	prov_.possiblepositions_.setEmpty();
	for ( auto idx=0; idx<nrfiles; idx++ )
	{
	    const auto inl = fnrs.atIndex( idx );
	    PosInfo::LineCollData linecd;
	    if ( ensureRightTransl(inl) && trl_->getGeometryInfo(linecd) )
		prov_.possiblepositions_.merge( linecd, true );
	}
    }

    if ( trl_ )
	prov_.zsubsels_.add( ZSubSel(trl_->packetInfo().zrg) );
    else
	prov_.zsubsels_.add( ZSubSel(SI().zRange()) );
}


void Seis::VolFetcher::prepWork()
{
    ensureDPIfAvailable( 0 );
}


bool Seis::VolFetcher::setPosition( const BinID& bid )
{
    dpbid_.inl() = -1;

    if ( !ensureRightTransl(bid.inl()) )
	return false;

    if ( haveDP() && dp().sampling().hsamp_.includes( bid ) )
	{ dpbid_ = bid; return true; }
    if ( trl_ && trl_->goTo(bid) )
	return true;

    return false;
}


void Seis::VolFetcher::getTrc( TraceData& td, SeisTrcInfo& ti )
{
    if ( dpbid_.inl() != -1 )
    {
	const TrcKey tk( dpbid_ );
	dp().fillTraceData( tk, td );
	dp().fillTraceInfo( tk, ti );
    }
    else
    {
	if ( !trl_ )
	    { uirv_.set( mINTERNAL("Read after setPosition fail") ); return; }
	else if ( !trl_->readInfo(ti) || !trl_->readData(&td) )
	    { uirv_.set( trl_->errMsg() ); return; }
    }

    ti.trcKey().setIs2D( false );
}


bool Seis::VolFetcher::isMultiConn() const
{
    const auto* ioobj = prov_.ioObj();
    return ioobj && ioobj->isStream() && ioobj->asStream()->isMultiConn();
}


bool Seis::VolFetcher::createTranslator( inl_type inl ) const
{
    const auto* ioobj = prov_.ioObj();
    auto* newtrl = ioobj->createTranslator();
    mDynamicCastGet( SeisTrcTranslator*, newtransl, newtrl );
    if ( !newtransl )
    {
	uirv_ = mINTERNAL("Cannot create appropriate data reader."
	    "\nThis is an installation problem or a data corruption issue.");
	return false;
    }

    if ( isMultiConn() )
    {
	const auto& iostrm = *ioobj->asStream();
	const auto nrs = iostrm.fileSpec().nrs_;
	if ( inl == -1 )
	    inl = nrs.snappedCenter( OD::SnapNearest );
	const auto connidx = iostrm.fileSpec().nrs_.getIndex( inl );
	iostrm.setConnIdx( connidx );
    }

    auto* conn = ioobj->getConn( true );
    if ( !newtransl->initRead(conn,prov_.readmode_) )
	{ uirv_ = newtransl->errMsg(); delete newtransl; return false; }

    if ( prov_.haveSelComps() )
	for ( auto icd=0; icd<newtransl->componentInfo().size(); icd++ )
	    if ( !prov_.selectedcomponents_.isPresent(icd) )
		newtransl->componentInfo()[icd]->selected_ = false;

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
	if ( inl == -1 || !isMultiConn() )
	    return true;

	const auto& iostrm = *prov_.ioObj()->asStream();
	const auto connidx = iostrm.curConnIdx();
	const auto conninl = iostrm.fileSpec().nrs_.atIndex(connidx);
	if ( conninl == inl )
	    return true;
    }

    return createTranslator( inl );
}



Seis::VolProvider::VolProvider()
    : fetcher_(*new VolFetcher(*this))
{
}


Seis::Fetcher& Seis::VolProvider::gtFetcher()
{
    return fetcher_;
}


void Seis::VolProvider::gtTrc( TraceData& td, SeisTrcInfo& ti,
			       uiRetVal& uirv ) const
{
    if ( !fetcher_.setPosition(trcpos_) )
	uirv.set( uiStrings::phrUnexpected(uiStrings::sPosition(),
					    trcpos_.usrDispStr()) );
    else
    {
	fetcher_.getTrc( td, ti );
	uirv = fetcher_.uirv_;
    }
}
