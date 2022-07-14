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
#include "seisbufadapters.h"
#include "seistrc.h"

static Threads::Atomic<int> curdatasetid_( 0 );


SyntheticData::SyntheticData( const SynthGenParams& sgp,
			      const Seis::SynthGenDataPack& synthgendp,
			      DataPack& dp )
    : SharedObject(sgp.name_)
    , sgp_(sgp)
    , datapack_(&dp)
    , synthgendp_(&synthgendp)
{
    datapack_->setName( sgp.name_ );
}


SyntheticData::~SyntheticData()
{
}


SyntheticData::SynthID SyntheticData::getNewID()
{
    return SynthID( ++curdatasetid_ );
}


void SyntheticData::setName( const char* nm )
{
    SharedObject::setName( nm );
    datapack_->setName( nm );
    if ( sgp_.name_ != nm )
	sgp_.name_.set( nm );
}


bool SyntheticData::isOK() const
{
    const int nrpos = nrPositions();
    return nrpos > 0 && getRefModels().nrModels() == nrpos;
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


const char* SyntheticData::waveletName() const
{
    return sgp_.getWaveletNm();
}


float SyntheticData::getTime( float dpt, int trcnr ) const
{
    const TimeDepthModel* tdmodel = getTDModel( trcnr );
    return tdmodel ? tdmodel->getTime( dpt ) : mUdf( float );
}


float SyntheticData::getDepth( float time, int trcnr ) const
{
    const TimeDepthModel* tdmodel = getTDModel( trcnr );
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


const ReflectivityModelBase* SyntheticData::getRefModel( int itrc ) const
{
    const ReflectivityModelSet& refmodels = getRefModels();
    return refmodels.validIdx(itrc) ? refmodels.get( itrc ) : nullptr;
}


const TimeDepthModel* SyntheticData::getTDModel( int itrc ) const
{
    const ReflectivityModelBase* refmodel = getRefModel( itrc );
    return refmodel ? &refmodel->getDefaultModel() : nullptr;
}


const TimeDepthModel* SyntheticData::getTDModel( int itrc, int ioff ) const
{
    const ReflectivityModelBase* refmodel = getRefModel( itrc );
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

    const int nrrefmodels = genres->getModels().nrModels();

    ConstRefMan<SyntheticData> ret;
    if ( genres->isStack() )
    {
	auto* dptrcbuf = new SeisTrcBuf( true );
	synthgen.getStackedTraces( *dptrcbuf );
	if ( dptrcbuf->size() != nrrefmodels )
	{
	    delete dptrcbuf;
	    return nullptr;
	}

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

	const bool iscorrected = sgp.isCorrected();
	RefObjectSet<PreStack::Gather> gatherset;
	while ( tbufs.size() )
	{
	    PtrMan<SeisTrcBuf> tbuf = tbufs.removeSingle( 0 );
	    RefMan<PreStack::Gather> gather = new PreStack::Gather();
	    if ( !gather->setFromTrcBuf(*tbuf,0) )
		continue;

	    gather->setName( sgp.name_ );
	    gather->setCorrected( iscorrected );
	    gatherset += gather;
	}

	if ( gatherset.size() != nrrefmodels )
	    return nullptr;

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
}


PostStackSyntheticData::~PostStackSyntheticData()
{
}


DataPack::MgrID PostStackSyntheticData::groupID()
{
    return DataPackMgr::FlatID();
}


DataPack::FullID PostStackSyntheticData::fullID() const
{
    return DataPack::FullID( groupID(), datapack_->id() );
}


const char* PostStackSyntheticData::sDataPackCategory()
{
    return "Post-stack synthetics";
}


const SeisTrc* PostStackSyntheticData::getTrace( int trcnr ) const
{ return postStackPack().trcBuf().get( trcnr ); }


int PostStackSyntheticData::nrPositions() const
{
    return postStackPack().trcBuf().size();
}


TrcKey PostStackSyntheticData::getTrcKey( int trcnr ) const
{
    const SeisTrcBuf& tbuf = postStackPack().trcBuf();
    return tbuf.validIdx( trcnr ) ? tbuf.get( trcnr )->info().trcKey()
				  : TrcKey::udf();

}


ZSampling PostStackSyntheticData::zRange() const
{
    const SeisTrcBuf& tbuf = postStackPack().trcBuf();
    return tbuf.isEmpty() ? tbuf.zRange() : tbuf.first()->zRange();
}


SeisTrcBufDataPack& PostStackSyntheticData::postStackPack()
{
    return static_cast<SeisTrcBufDataPack&>( *datapack_ );
}


const SeisTrcBufDataPack& PostStackSyntheticData::postStackPack() const
{
    return static_cast<const SeisTrcBufDataPack&>( *datapack_ );
}


ConstRefMan<FlatDataPack> PostStackSyntheticData::getTrcDP() const
{
    return &postStackPack();
}


ConstRefMan<FlatDataPack> PostStackSyntheticData::getFlattenedTrcDP(
			    const TypeSet<float>& zvals, bool istime ) const
{
    if ( zvals.isEmpty() )
	return getTrcDP();
    if ( zvals.size() != nrPositions() )
	{ pErrMsg("wrong size"); return getTrcDP(); }

    const TypeSet<float>* tvals = &zvals;
    PtrMan<TypeSet<float> > tconvsdeleter;
    if ( !istime )
    {
	const int nrtrcs = zvals.size();
	auto* tconvs = new TypeSet<float>( nrtrcs, 0.f );
	for ( int itrc=0; itrc<nrtrcs; itrc++ )
	    tconvs->get(itrc) = getTime( zvals[itrc], itrc );
	tvals = tconvs;
	tconvsdeleter = tconvs;
    }

    const SeisTrcBufDataPack& tbdp = postStackPack();
    const SeisTrcBuf& tbuf = tbdp.trcBuf();
    const Interval<float> zrg = tbuf.getZRange4Shifts( *tvals, true );
    if ( mIsUdf(zrg.start) )
	return getTrcDP();

    auto* dptrcbuf = new SeisTrcBuf( true );
    tbuf.getShifted( zrg, *tvals, true, mUdf(float), *dptrcbuf );
    ConstRefMan<FlatDataPack> dp =
	new SeisTrcBufDataPack( dptrcbuf, Seis::Line, SeisTrcInfo::TrcNr,
				PostStackSyntheticData::sDataPackCategory() );
    dp.getNonConstPtr()->setName( name() );

    return dp;
}


PreStackSyntheticData::PreStackSyntheticData( const SynthGenParams& sgp,
					const Seis::SynthGenDataPack& synthdp,
					PreStack::GatherSetDataPack& dp )
    : SyntheticData(sgp,synthdp,dp)
{
    preStackPack().setName( name() );
}


PreStackSyntheticData::~PreStackSyntheticData()
{
}


DataPack::MgrID PreStackSyntheticData::groupID()
{
    return DataPackMgr::SeisID();
}


DataPack::FullID PreStackSyntheticData::fullID() const
{
    return DataPack::FullID( groupID(), datapack_->id() );
}


void PreStackSyntheticData::setName( const char* nm )
{
    SyntheticData::setName( nm );
    preStackPack().setName( nm );
    if ( angledp_ )
    {
	const BufferString anglenm( nm, " (Angle Gather)" );
	angledp_.getNonConstPtr()->setName( anglenm );
    }
}


PreStack::GatherSetDataPack& PreStackSyntheticData::preStackPack()
{
    return static_cast<PreStack::GatherSetDataPack&>( *datapack_ );
}


const PreStack::GatherSetDataPack& PreStackSyntheticData::preStackPack() const
{
    return static_cast<const PreStack::GatherSetDataPack&>( *datapack_ );
}


void PreStackSyntheticData::obtainGathers()
{
    preStackPack().obtainGathers();
    if ( angledp_ )
	angledp_.getNonConstPtr()->obtainGathers();
}


int PreStackSyntheticData::nrPositions() const
{
    return preStackPack().nrGathers();
}


TrcKey PreStackSyntheticData::getTrcKey( int trcnr ) const
{
    return preStackPack().getTrcKeyByIdx( trcnr );
}


ConstRefMan<PreStack::Gather> PreStackSyntheticData::getGather( int trcnr,
							    bool angles ) const
{
    return angles ? ( angledp_ ? angledp_->getGather( trcnr ) : nullptr )
		  : preStackPack().getGather( trcnr );
}


DataPack::ID PreStackSyntheticData::getGatherIDByIdx( int trcnr,
						      bool angles ) const
{
    ConstRefMan<PreStack::Gather> gather = getGather( trcnr, angles );
    return gather ? gather->id() : DataPack::cNoID();
}


ZSampling PreStackSyntheticData::zRange() const
{
    return preStackPack().zRange();
}


void PreStackSyntheticData::convertAngleDataToDegrees(
					PreStack::Gather& ag ) const
{
    const auto& agdata = const_cast<const Array2D<float>&>( ag.data() );
    ArrayMath::getScaledArray<float>( agdata, nullptr, mRad2DegD, 0.,
				      false, true );
}


void PreStackSyntheticData::setAngleData(
					const PreStack::GatherSetDataPack* ags )
{
    angledp_ = ags;
}


Interval<float> PreStackSyntheticData::offsetRange() const
{
    return preStackPack().offsetRange();
}


float PreStackSyntheticData::offsetRangeStep() const
{
    return preStackPack().offsetRangeStep();
}


bool PreStackSyntheticData::hasOffset() const
{ return offsetRange().width() > 0; }


bool PreStackSyntheticData::isNMOCorrected() const
{
    return getGenParams().isCorrected();
}


const SeisTrc* PreStackSyntheticData::getTrace( int trcnr, int* offset ) const
{ return preStackPack().getTrace( trcnr, offset ? *offset : 0 ); }


SeisTrcBuf* PreStackSyntheticData::getTrcBuf( float offset,
					const Interval<float>* stackrg ) const
{
    auto* tbuf = new SeisTrcBuf( true );
    Interval<float> offrg = stackrg ? *stackrg : Interval<float>(offset,offset);
    preStackPack().fill( *tbuf, offrg );
    return tbuf;
}


ConstRefMan<FlatDataPack> PreStackSyntheticData::getTrcDPAtOffset( int offsidx )
									   const
{
    auto* dptrcbuf = new SeisTrcBuf( true );
    const int nrtrcsc = nrPositions();

    for ( int itrc=0; itrc<nrtrcsc; itrc++ )
    {
	const SeisTrc* trc = getTrace( itrc, &offsidx );
	if ( trc )
	    dptrcbuf->add( new SeisTrc(*trc) );
	else
	{
	    pErrMsg("null trc");
	    dptrcbuf->add( new SeisTrc(preStackPack().zRange().nrSteps()+1) );
	}
    }

    ConstRefMan<FlatDataPack> dp =
	new SeisTrcBufDataPack( dptrcbuf, Seis::Line, SeisTrcInfo::TrcNr,
				PostStackSyntheticData::sDataPackCategory() );
    dp.getNonConstPtr()->setName( name() );
    return dp;
}


ConstRefMan<FlatDataPack> PreStackSyntheticData::getFlattenedTrcDP(
						const TypeSet<float>& zvals,
						bool istime, int offsidx ) const
{
    if ( zvals.isEmpty() )
	return getTrcDPAtOffset( offsidx );
    if ( zvals.size() != nrPositions() )
	{ pErrMsg("wrong size"); return getTrcDPAtOffset( offsidx ); }

    const TypeSet<float>* tvals = &zvals;
    PtrMan<TypeSet<float> > tconvsdeleter;
    const bool iscorrected = isNMOCorrected();
    if ( !istime )
    {
	const int nrtrcs = zvals.size();
	auto* tconvs = new TypeSet<float>( nrtrcs, 0.f );
	for ( int itrc=0; itrc<nrtrcs; itrc++ )
	{
	    tconvs->get(itrc) = iscorrected
		? getTDModel( itrc )->getTime( zvals[itrc] )
		: getTDModel( itrc, offsidx )->getTime( zvals[itrc] );
	}
	tvals = tconvs;
	tconvsdeleter = tconvs;
    }

    const PreStack::GatherSetDataPack& tbdp = preStackPack();
    PtrMan<SeisTrcBuf> tbuf = new SeisTrcBuf( true );
    tbdp.fill( *tbuf.ptr(), offsidx );
    const Interval<float> zrg = tbuf->getZRange4Shifts( *tvals, true );
    if ( mIsUdf(zrg.start) )
	return getTrcDPAtOffset( offsidx );

    auto* dptrcbuf = new SeisTrcBuf( true );
    tbuf->getShifted( zrg, *tvals, true, mUdf(float), *dptrcbuf );
    tbuf = nullptr;

    ConstRefMan<FlatDataPack> dp =
	new SeisTrcBufDataPack( dptrcbuf, Seis::Line, SeisTrcInfo::TrcNr,
				PostStackSyntheticData::sDataPackCategory() );
    dp.getNonConstPtr()->setName( name() );

    return dp;
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
