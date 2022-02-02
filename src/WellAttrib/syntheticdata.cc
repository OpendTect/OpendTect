/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
________________________________________________________________________

-*/

#include "syntheticdataimpl.h"

#include "arrayndalgo.h"
#include "ioman.h"
#include "prestackgather.h"
#include "seisbufadapters.h"

static const char* sKeyDispPar()		{ return "Display Parameter"; }
const char* PostStackSyntheticData::sDataPackCategory()
{ return "Post-stack synthetics"; }


void SynthFVSpecificDispPars::fillPar( IOPar& par ) const
{
    IOPar disppar, vdmapperpar, wvamapperpar;
    vdmapperpar.set( FlatView::DataDispPars::sKeyColTab(), ctab_ );
    wvamapperpar.set( FlatView::DataDispPars::sKeyOverlap(), overlap_ );
    vdmapper_.fillPar( vdmapperpar );
    disppar.mergeComp( vdmapperpar, FlatView::DataDispPars::sKeyVD() );
    wvamapper_.fillPar( wvamapperpar );
    disppar.mergeComp( wvamapperpar, FlatView::DataDispPars::sKeyWVA() );
    par.mergeComp( disppar, sKeyDispPar() );
}


void SynthFVSpecificDispPars::usePar( const IOPar& par )
{
    PtrMan<IOPar> disppar = par.subselect( sKeyDispPar() );
    if ( !disppar )
	return;

    overlap_ = 1.0f;
    disppar->get( FlatView::DataDispPars::sKeyColTab(), ctab_ );
    disppar->get( FlatView::DataDispPars::sKeyOverlap(), overlap_ );
    PtrMan<IOPar> vdmapperpar =
	disppar->subselect( FlatView::DataDispPars::sKeyVD() );
    if ( !vdmapperpar ) // Older par file
    {
	vdmapper_.type_ = ColTab::MapperSetup::Fixed;
	wvamapper_.type_ = ColTab::MapperSetup::Fixed;
	disppar->get( sKey::Range(), vdmapper_.range_ );
	disppar->get( sKey::Range(), wvamapper_.range_ );
    }
    else
    {
	 if ( vdmapperpar )
	 {
	     vdmapper_.usePar( *vdmapperpar );
	     vdmapperpar->get( FlatView::DataDispPars::sKeyColTab(), ctab_ );
	 }
	 PtrMan<IOPar> wvamapperpar =
	     disppar->subselect( FlatView::DataDispPars::sKeyWVA() );
	 if ( wvamapperpar )
	 {
	     wvamapper_.usePar( *wvamapperpar );
	     wvamapperpar->get(FlatView::DataDispPars::sKeyOverlap(),overlap_);
	 }
    }
}



SyntheticData::SyntheticData( const SynthGenParams& sgp,
			      const Seis::SynthGenDataPack& synthgendp,
			      DataPack& dp )
    : SharedObject(sgp.name_)
    , sgp_(sgp)
    , datapack_(dp)
    , synthgendp_(&synthgendp)
{
}


SyntheticData::~SyntheticData()
{
    removePack();
}


void SyntheticData::setName( const char* nm )
{
    SharedObject::setName( nm );
    datapack_.setName( nm );
}


void SyntheticData::removePack()
{
    const DataPack::FullID dpid = datapackid_;
    DataPackMgr::ID packmgrid = DataPackMgr::getID( dpid );
    DPM(packmgrid).release( dpid.ID(1) );
}


void SyntheticData::useGenParams( const SynthGenParams& sgp )
{
    if ( sgp.synthtype_ != sgp_.synthtype_ )
    {
	pErrMsg("Should not change type");
	return;
    }

    IOPar par;
    sgp.fillPar( par );
    sgp_.usePar( par );
    setName( sgp_.name_ );
}


void SyntheticData::fillDispPar( IOPar& par ) const
{
    disppars_.fillPar( par );
}


void SyntheticData::useDispPar( const IOPar& par )
{
    disppars_.usePar( par );
}


const char* SyntheticData::waveletName() const
{
    return sgp_.getWaveletNm();
}


float SyntheticData::getTime( float dpt, int seqnr ) const
{
    const TimeDepthModel* tdmodel = getTDModel( seqnr );
    return tdmodel ? tdmodel->getTime( dpt ) : mUdf( float );
}


float SyntheticData::getDepth( float time, int seqnr ) const
{
    const TimeDepthModel* tdmodel = getTDModel( seqnr );
    return tdmodel ? tdmodel->getDepth( time ) : mUdf( float );
}


const Seis::SynthGenDataPack& SyntheticData::synthGenDP() const
{
    return *synthgendp_.ptr();
}


const ReflectivityModelSet& SyntheticData::getRefModels() const
{
    return synthGenDP().getModels();
}


const ReflectivityModelBase* SyntheticData::getRefModel( int imdl ) const
{
    const ReflectivityModelSet& refmodels = getRefModels();
    return refmodels.validIdx(imdl) ? refmodels.get( imdl ) : nullptr;
}


const TimeDepthModel* SyntheticData::getTDModel( int imdl ) const
{
    const ReflectivityModelBase* refmodel = getRefModel( imdl );
    return refmodel ? &refmodel->getDefaultModel() : nullptr;
}


const TimeDepthModel* SyntheticData::getTDModel( int imdl, int ioff ) const
{
    const ReflectivityModelBase* refmodel = getRefModel( imdl );
    return refmodel ? refmodel->get( ioff ) : nullptr;
}


ConstRefMan<SyntheticData> SyntheticData::get( const SynthGenParams& sgp,
					     Seis::RaySynthGenerator& synthgen )
{
    if ( !sgp.isRawOutput() )
	return nullptr;

    ConstRefMan<Seis::SynthGenDataPack> genres = synthgen.getAllResults();
    if ( !genres )
	return nullptr;

    ConstRefMan<SyntheticData> ret;
    if ( genres->isStack() )
    {
	auto* dptrcbuf = new SeisTrcBuf( true );
	synthgen.getStackedTraces( *dptrcbuf );
	auto* dp =
	    new SeisTrcBufDataPack( dptrcbuf, Seis::Line, SeisTrcInfo::TrcNr,
				PostStackSyntheticData::sDataPackCategory() );
	ret = new PostStackSyntheticData( sgp, *genres.ptr(), *dp );
    }
    else if ( genres->isPS() )
    {
	ObjectSet<SeisTrcBuf> tbufs;
	if ( !synthgen.getTraces(tbufs) )
	    return nullptr;

	const bool iscorrected = genres->isCorrected();
	ObjectSet<PreStack::Gather> gatherset;
	while ( tbufs.size() )
	{
	    PtrMan<SeisTrcBuf> tbuf = tbufs.removeSingle( 0 );
	    auto* gather = new PreStack::Gather();
	    if ( !gather->setFromTrcBuf(*tbuf,0) )
		{ delete gather; continue; }

	    gather->setCorrected( iscorrected );
	    gatherset += gather;
	}

	auto* dp = new PreStack::GatherSetDataPack( nullptr, gatherset );
	ret = new PreStackSyntheticData( sgp, *genres.ptr(), *dp );
    }

    return ret;
}



PostStackSyntheticData::PostStackSyntheticData( const SynthGenParams& sgp,
				    const Seis::SynthGenDataPack& synthdp,
				    SeisTrcBufDataPack& dp)
    : SyntheticData(sgp,synthdp,dp)
{
    DataPackMgr::ID pmid = DataPackMgr::FlatID();
    DPM( pmid ).addAndObtain( &dp );
    datapackid_ = DataPack::FullID( pmid, dp.id());
}


PostStackSyntheticData::~PostStackSyntheticData()
{
}


const SeisTrc* PostStackSyntheticData::getTrace( int seqnr ) const
{ return postStackPack().trcBuf().get( seqnr ); }


int PostStackSyntheticData::nrPositions() const
{
    return postStackPack().trcBuf().size();
}


SeisTrcBufDataPack& PostStackSyntheticData::postStackPack()
{
    return static_cast<SeisTrcBufDataPack&>( datapack_ );
}


const SeisTrcBufDataPack& PostStackSyntheticData::postStackPack() const
{
    return static_cast<const SeisTrcBufDataPack&>( datapack_ );
}


PreStackSyntheticData::PreStackSyntheticData( const SynthGenParams& sgp,
					const Seis::SynthGenDataPack& synthdp,
					PreStack::GatherSetDataPack& dp )
    : SyntheticData(sgp,synthdp,dp)
{
    DataPackMgr::ID pmid = DataPackMgr::SeisID();
    DPM( pmid ).addAndObtain( &dp );
    datapackid_ = DataPack::FullID( pmid, dp.id());
    ObjectSet<PreStack::Gather>& gathers = dp.getGathers();
    for ( int idx=0; idx<gathers.size(); idx++ )
	gathers[idx]->setName( name() );
}


PreStackSyntheticData::~PreStackSyntheticData()
{
    DPM( DataPackMgr::SeisID() ).release( datapack_.id() );
    if ( angledp_ )
	DPM( DataPackMgr::SeisID() ).release( angledp_->id() );
}


PreStack::GatherSetDataPack& PreStackSyntheticData::preStackPack()
{
    return static_cast<PreStack::GatherSetDataPack&>( datapack_ );
}


const PreStack::GatherSetDataPack& PreStackSyntheticData::preStackPack() const
{
    return static_cast<const PreStack::GatherSetDataPack&>( datapack_ );
}


int PreStackSyntheticData::nrPositions() const
{
    return preStackPack().getGathers().size();
}


void PreStackSyntheticData::convertAngleDataToDegrees(
					PreStack::Gather& ag ) const
{
    const auto& agdata = const_cast<const Array2D<float>&>( ag.data() );
    ArrayMath::getScaledArray<float>( agdata, nullptr, mRad2DegD, 0.,
				      false, true );
}


void PreStackSyntheticData::setAngleData(
					const ObjectSet<PreStack::Gather>& ags )
{
    if ( angledp_ )
	DPM( DataPackMgr::SeisID() ).release( angledp_->id() );

    angledp_ = new PreStack::GatherSetDataPack( nullptr, ags );
    const BufferString angledpnm( name().buf(), " (Angle Gather)" );
    angledp_->setName( angledpnm );
    DPM( DataPackMgr::SeisID() ).addAndObtain( angledp_ );
}


float PreStackSyntheticData::offsetRangeStep() const
{
    float offsetstep = mUdf(float);
    const ObjectSet<PreStack::Gather>& gathers = preStackPack().getGathers();
    if ( !gathers.isEmpty() )
    {
	const PreStack::Gather& gather = *gathers[0];
	offsetstep = gather.getOffset(1)-gather.getOffset(0);
    }

    return offsetstep;
}


const Interval<float> PreStackSyntheticData::offsetRange() const
{
    Interval<float> offrg( 0, 0 );
    const ObjectSet<PreStack::Gather>& gathers = preStackPack().getGathers();
    if ( !gathers.isEmpty() )
    {
	const PreStack::Gather& gather = *gathers[0];
	offrg.set(gather.getOffset(0),gather.getOffset( gather.size(true)-1));
    }
    return offrg;
}


bool PreStackSyntheticData::hasOffset() const
{ return offsetRange().width() > 0; }


bool PreStackSyntheticData::isNMOCorrected() const
{
    return synthgendp_->isCorrected();
}


const SeisTrc* PreStackSyntheticData::getTrace( int seqnr, int* offset ) const
{ return preStackPack().getTrace( seqnr, offset ? *offset : 0 ); }


SeisTrcBuf* PreStackSyntheticData::getTrcBuf( float offset,
					const Interval<float>* stackrg ) const
{
    SeisTrcBuf* tbuf = new SeisTrcBuf( true );
    Interval<float> offrg = stackrg ? *stackrg : Interval<float>(offset,offset);
    preStackPack().fill( *tbuf, offrg );
    return tbuf;
}


PostStackSyntheticDataWithInput::PostStackSyntheticDataWithInput(
					const SynthGenParams& sgp,
					const Seis::SynthGenDataPack& synthdp,
					SeisTrcBufDataPack& sdp )
    : PostStackSyntheticData(sgp,synthdp,sdp)
{
}


PostStackSyntheticDataWithInput::~PostStackSyntheticDataWithInput()
{}


InstAttributeSyntheticData::InstAttributeSyntheticData(
					const SynthGenParams& sgp,
					const Seis::SynthGenDataPack& synthdp,
					SeisTrcBufDataPack& sdp )
    : PostStackSyntheticDataWithInput(sgp,synthdp,sdp)
{
}



PSBasedPostStackSyntheticData::PSBasedPostStackSyntheticData(
					const SynthGenParams& sgp,
					const Seis::SynthGenDataPack& synthdp,
					SeisTrcBufDataPack& sdp )
    : PostStackSyntheticDataWithInput(sgp,synthdp,sdp)
{
}


PSBasedPostStackSyntheticData::~PSBasedPostStackSyntheticData()
{}


StratPropSyntheticData::StratPropSyntheticData( const SynthGenParams& sgp,
					const Seis::SynthGenDataPack& synthdp,
					SeisTrcBufDataPack& dp,
					const PropertyRef& pr )
    : PostStackSyntheticData(sgp,synthdp,dp)
    , prop_(pr)
{}
