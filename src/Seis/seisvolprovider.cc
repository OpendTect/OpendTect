/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/

#include "seisproviderimpl.h"
#include "seisfetcher.h"
#include "cubesubsel.h"
#include "cubedata.h"
#include "iostrm.h"
#include "file.h"
#include "seispacketinfo.h"
#include "seisrangeseldata.h"
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
			    : Fetcher3D(p)		{}
			~VolFetcher()			{ delete trl_; }

    bool		isPS() const override		{ return false; }
    void		getComponentInfo(BufferStringSet&,
					 DataType&) const override;
    void		getPossibleExtents() override;
    void		prepWork() override;
    const STTrl*	curTransl() const override   { return trl_; }

    bool		setPosition(const BinID&) override;
    void		getTrc(TraceData&,SeisTrcInfo&);

protected:

    STTrl*		trl_		= nullptr;
    BinID		dpbid_;

    bool		isMultiConn() const;
    bool		createTranslator(inl_type) const;
    bool		ensureRightTransl(inl_type inl=-1) const;
    const RegularSeisDataPack& dp() const { return regSeisDP(); }

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


void Seis::VolFetcher::getPossibleExtents()
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
	    PosInfo::CubeData cd;
	    if ( ensureRightTransl(inl) && trl_->getGeometryInfo(cd) )
		prov_.possiblepositions_.merge( cd, true );
	}
    }

    if ( trl_ )
	prov_.allzsubsels_[0].setOutputZRange( trl_->packetInfo().zrg );
}


void Seis::VolFetcher::prepWork()
{
    deleteAndZeroPtr( trl_ ); // make sure we select the updated ranges etc.
    ensureRightTransl();
    handleGeomIDChange( 0 );
}


bool Seis::VolFetcher::setPosition( const BinID& bid )
{
    dpbid_.inl() = -1;

    if ( !ensureRightTransl(bid.inl()) )
	return false;

    if ( useDP(bid) )
	{ dpbid_ = bid; return true; }
    if ( trl_ && !trl_->supportsGoTo() )
	return true;
    if ( trl_ && trl_->goTo(bid) )
	return true;

    return false;
}


void Seis::VolFetcher::getTrc( TraceData& td, SeisTrcInfo& ti )
{
    if ( dpbid_.inl() != -1 )
	fillFromDP( dpbid_, ti, td );
    else
    {
	if ( !trl_ )
	    { uirv_.set( mINTERNAL("Read after setPosition fail") ); return; }
	else if ( !trl_->readInfo(ti) || !trl_->readData(&td) )
	    { uirv_.set( trl_->errMsg() ); return; }
    }

    if ( ti.is2D() )
	{ pErrMsg("trcinf is 2D"); ti.setPos(SI().transform(ti.coord_)); }
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

    auto* sd = new Seis::RangeSelData( &prov_.dbKey().surveyInfo() );
    sd->zSubSel().limitTo( prov_.zSubSel() );
    newtransl->setSelData( sd );
    delete sd;

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


#include "seisproviderimpldefs.h"
mDefNonPSProvFns( 3D, Vol )
