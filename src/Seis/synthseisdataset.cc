/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno / Bert
Date:	       July 2011 / Aug 2018
________________________________________________________________________

-*/

#include "synthseisdataset.h"

#include "array1dinterpol.h"
#include "angles.h"
#include "atomic.h"
#include "arrayndimpl.h"
#include "binnedvalueset.h"
#include "datapackbase.h"
#include "elasticpropsel.h"
#include "envvars.h"
#include "fourier.h"
#include "fftfilter.h"
#include "flatview.h"
#include "mathfunc.h"
#include "propertyref.h"
#include "raytrace1d.h"
#include "raytracerrunner.h"
#include "separstr.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "survinfo.h"
#include "synthseisgenerator.h"
#include "unitofmeasure.h"
#include "timeser.h"
#include "velocitycalc.h"
#include "waveletmanager.h"

#define sDefaultAngleRange Interval<float>( 0.0f, 30.0f )
#define sDefaultOffsetRange StepInterval<float>( 0.f, 6000.f, 100.f )


static const char* sKeyDispPar()		{ return "Display Parameter"; }
static Threads::Atomic<int> curdatasetid_( 0 );
const char* SynthSeis::PostStackDataSet::sDataPackCategory()
{ return "Post-stack synthetics"; }


SynthSeis::DispPars::DispPars()
    : vdmapsetup_(new MapperSetup)
    , wvamapsetup_(new MapperSetup)
    , overlap_(1.f)
{
}


void SynthSeis::DispPars::fillPar( IOPar& par ) const
{
    IOPar disppar, vdmapperpar, wvamapperpar;
    using FlatView::DataDispPars;
    vdmapperpar.set( DataDispPars::sKeyColTab(), colseqname_ );
    wvamapperpar.set( DataDispPars::sKeyOverlap(), overlap_ );
    vdmapsetup_->fillPar( vdmapperpar );
    disppar.mergeComp( vdmapperpar, DataDispPars::sKeyVD() );
    wvamapsetup_->fillPar( wvamapperpar );
    disppar.mergeComp( wvamapperpar, DataDispPars::sKeyWVA() );
    par.mergeComp( disppar, sKeyDispPar() );
}


void SynthSeis::DispPars::usePar( const IOPar& par )
{
    PtrMan<IOPar> disppar = par.subselect( sKeyDispPar() );
    if ( !disppar )
	return;

    using FlatView::DataDispPars;
    disppar->get( DataDispPars::sKeyColTab(), colseqname_ );
    disppar->get( DataDispPars::sKeyOverlap(), overlap_ );
    PtrMan<IOPar> vdmapperpar =
	disppar->subselect( DataDispPars::sKeyVD() );
    if ( !vdmapperpar ) // Older par file
    {
	Interval<float> rg( vdmapsetup_->range() );
	disppar->get( sKey::Range(), rg );
	wvamapsetup_->setFixedRange( rg );
	vdmapsetup_->setFixedRange( rg );
    }
    else
    {
	 vdmapperpar->get( DataDispPars::sKeyColTab(), colseqname_ );
	 vdmapsetup_->usePar( *vdmapperpar );
	 PtrMan<IOPar> wvamapperpar =
	     disppar->subselect( DataDispPars::sKeyWVA() );
	 if ( wvamapperpar )
	 {
	     wvamapsetup_->usePar( *wvamapperpar );
	     wvamapperpar->get(DataDispPars::sKeyOverlap(),overlap_);
	 }
    }
}



SynthSeis::DataSet::DataSet( const GenParams& gp, DataPack& dp,
			     RayModelSet* rms )
    : NamedObject(gp.name_)
    , genpars_(gp)
    , raymodels_(rms)
    , datapack_(&dp)
{
    dp.setName( gp.name_ );
}


SynthSeis::DataSet::~DataSet()
{
    deepErase( finald2tmodels_ );
}


SynthSeis::DataSet::MgrID SynthSeis::DataSet::getNewID()
{
    return MgrID( curdatasetid_++ );
}


SynthSeis::DataSet::size_type SynthSeis::DataSet::size() const
{
    return finald2tmodels_.size();
}


const SeisTrc* SynthSeis::DataSet::getTrace( idx_type idx, float offs ) const
{
    return validIdx(idx) ? gtTrc( idx, offs ) : 0;
}


bool SynthSeis::DataSet::hasOffset() const
{
    return offsetDef().nrSteps() > 0;
}


void SynthSeis::DataSet::getD2TFrom( const DataSet& oth )
{
    if ( this != &oth )
	deepCopy( finald2tmodels_, oth.finald2tmodels_ );
}


ZSampling SynthSeis::DataSet::zRange() const
{
    const SeisTrc* trc0 = getTrace( 0 );
    if ( !trc0 )
	return StepInterval<float>( 0.f, 1.f, 1.f );

    return trc0->zRange();
}


float SynthSeis::DataSet::getTime( float dpt, int seqnr ) const
{
    return !validIdx( seqnr ) ? mUdf( float )
	 : finald2tmodels_[seqnr]->getTime( dpt );
}


float SynthSeis::DataSet::getDepth( float time, int seqnr ) const
{
    return !validIdx( seqnr ) ? mUdf( float )
	 : finald2tmodels_[seqnr]->getDepth( time );
}


void SynthSeis::DataSet::fillDispPars( IOPar& par ) const
{
    disppars_.fillPar( par );
}


void SynthSeis::DataSet::useDispPars( const IOPar& par )
{
    disppars_.usePar( par );
}



void SynthSeis::DataSet::adjustD2TModelsToSRD( D2TModelSet& d2tmdls )
{
    const double shft = -SI().seismicReferenceDatum();
    for ( auto d2tmdl : d2tmdls )
	d2tmdl->shiftDepths( shft );
}


void SynthSeis::DataSet::updateD2TModels()
{
    deepErase( finald2tmodels_ );
    for ( auto rm : rayModels() )
    {
	TimeDepthModel* zeroofsetd2tm = new TimeDepthModel();
	*zeroofsetd2tm = rm->zeroOffsetD2T();
	finald2tmodels_ += zeroofsetd2tm;
    }

    adjustD2TModelsToSRD( finald2tmodels_ );
}


DataPack::FullID SynthSeis::DataSet::dataPackID() const
{
    return DataPack::FullID( dpMgrID(), datapack_->id() );
}


const DataPack* SynthSeis::DataSet::gtTrcBufDP( float offs ) const
{
    auto* retdp = new SeisTrcBufDataPack( "Stacked Synthetics" );
    auto* dptrcbuf = new SeisTrcBuf( true );
    const auto nrtrcsc = size();

    for ( int itrc=0; itrc<nrtrcsc; itrc++ )
    {
	const auto* trc = getTrace( itrc, offs );
	if ( trc )
	    dptrcbuf->add( new SeisTrc(*trc) );
	else
	{
	    pErrMsg("null trc");
	    dptrcbuf->add( new SeisTrc(zRange().nrSteps()+1) );
	}
    }

    retdp->setBuffer( dptrcbuf, Seis::Line, SeisTrcInfo::TrcNr, 0, true );
    return retdp;
}


ConstRefMan<DataPack> SynthSeis::DataSet::getTrcDPAtOffset( float offs ) const
{
    return ConstRefMan<DataPack>( gtTrcBufDP(offs) );
}


ConstRefMan<DataPack> SynthSeis::DataSet::getFlattenedTrcDP(
	const ZValueSet& zvals, bool istime, float offs ) const
{
    if ( zvals.isEmpty() )
	return datapack_;
    if ( zvals.size() != size() )
	{ pErrMsg("wrong size"); return datapack_; }

    const ZValueSet* tvals = &zvals;
    PtrMan<ZValueSet> tconvsdeleter;
    if ( !istime )
    {
	const auto nrtrcs = zvals.size();
	ZValueSet* tconvs = new ZValueSet( nrtrcs, 0.f );
	for ( int idx=0; idx<nrtrcs; idx++ )
	    tconvs->get(idx) = getTime( zvals.get(idx), idx );
	tvals = tconvs;
	tconvsdeleter = tconvs;
    }

    ConstRefMan<DataPack> inpdp = gtTrcBufDP( offs );
    if ( !inpdp )
	return datapack_;
    mDynamicCastGet( const SeisTrcBufDataPack*, tbdp, inpdp.ptr() );
    if ( !tbdp )
	{ pErrMsg("Bad DP type"); return datapack_; }

    const auto& tbuf = tbdp->trcBuf();
    ZGate zrg = tbuf.getZGate4Shifts( *tvals, true );
    if ( mIsUdf(zrg.start) )
	return datapack_;

    auto* retdp = new SeisTrcBufDataPack( "Flattened Synthetics" );
    auto* dptrcbuf = new SeisTrcBuf( true );
    tbuf.getShifted( zrg, *tvals, true, mUdf(float), *dptrcbuf );
    retdp->setBuffer( dptrcbuf, Seis::Line, SeisTrcInfo::TrcNr, 0, true );

    return ConstRefMan<DataPack>( retdp );
}


SynthSeis::PostStackDataSet::PostStackDataSet( const GenParams& gp, DPType& dp,
					       RayModelSet* rms )
    : DataSet(gp,dp,rms)
{
}


SynthSeis::PostStackDataSet::~PostStackDataSet()
{
}


DataPackMgr::ID	SynthSeis::PostStackDataSet::dpMgrID() const
{
    return DataPackMgr::FlatID();
}


const SeisTrc* SynthSeis::PostStackDataSet::gtTrc( idx_type idx, float ) const
{
    return postStackPack().trcBuf().get( idx );
}


SeisTrcBufDataPack& SynthSeis::PostStackDataSet::postStackPack()
{
    return static_cast<SeisTrcBufDataPack&>( *datapack_ );
}


const DataPack* SynthSeis::PostStackDataSet::gtTrcBufDP( float ) const
{
    return datapack_;
}


const SeisTrcBufDataPack& SynthSeis::PostStackDataSet::postStackPack() const
{
    return static_cast<const SeisTrcBufDataPack&>( *datapack_ );
}


SynthSeis::PSBasedPostStackDataSet::PSBasedPostStackDataSet(
					    const GenParams& gp, DPType& dp )
    : PostStackDataSet(gp,dp,0)
{
}


SynthSeis::StratPropDataSet::StratPropDataSet( const GenParams& sgp,
					       SeisTrcBufDataPack& dp,
					       const PropertyRef& pr )
    : PostStackDataSet(sgp,dp,0)
    , prop_(pr)
{
}
