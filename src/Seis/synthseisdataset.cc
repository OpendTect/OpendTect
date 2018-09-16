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
#include "binidvalset.h"
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
#include "seistrcprop.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "timeser.h"
#include "velocitycalc.h"
#include "waveletmanager.h"

#define sDefaultAngleRange Interval<float>( 0.0f, 30.0f )
#define sDefaultOffsetRange StepInterval<float>( 0.f, 6000.f, 100.f )


static const char* sKeyDispPar()		{ return "Display Parameter"; }
static Threads::Atomic<int> curdatasetid_( 0 );


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



SynthSeis::RayModel::RayModel( const RayTracer1D& rt1d, int nroffsets )
    : zerooffsett2dmodel_(0)
    , reflmodels_(new ReflectivityModelSet)
    , sampledreflmodels_(new ReflectivityModelSet)
{
    for ( int idx=0; idx<nroffsets; idx++ )
    {
	ReflectivityModel* refmodel = new ReflectivityModel();
	rt1d.getReflectivity( idx, *refmodel );

	TimeDepthModel* t2dm = new TimeDepthModel();
	rt1d.getTDModel( idx, *t2dm );

	reflmodels_->add( refmodel );
	t2dmodels_ += t2dm;
	if ( !idx )
	{
	    zerooffsett2dmodel_ = new TimeDepthModel();
	    rt1d.getZeroOffsTDModel( *zerooffsett2dmodel_ );
	}
    }
}


SynthSeis::RayModel::RayModel( const RayModel& oth )
    : zerooffsett2dmodel_(0)
    , reflmodels_(oth.reflmodels_)
    , sampledreflmodels_(oth.sampledreflmodels_)
{
    if ( oth.zerooffsett2dmodel_ )
	zerooffsett2dmodel_ = new TimeDepthModel( *oth.zerooffsett2dmodel_ );
    for ( auto t2dmdl : oth.t2dmodels_ )
	t2dmodels_ += new TimeDepthModel( *t2dmdl );
    for ( auto trc : oth.outtrcs_ )
	outtrcs_ += new SeisTrc( *trc );
}


SynthSeis::RayModel::~RayModel()
{
    deepErase( outtrcs_ );
    deepErase( t2dmodels_ );
    delete zerooffsett2dmodel_;
}


void SynthSeis::RayModel::forceReflTimes(const StepInterval<float>& si )
{
    for ( int idx=0; idx<reflmodels_->size(); idx++ )
    {
	ReflectivityModel& refmodel =
			const_cast<ReflectivityModel&>(*reflmodels_->get(idx));
	for ( int iref=0; iref<refmodel.size(); iref++ )
	{
	    refmodel[iref].time_ = si.atIndex(iref);
	    refmodel[iref].correctedtime_ = si.atIndex(iref);
	}
    }
}


#define mGet( inpset, outpset, steal )\
{\
    outpset.copy( inpset );\
    if ( steal )\
	inpset.erase();\
}

void SynthSeis::RayModel::getTraces(
		    ObjectSet<SeisTrc>& trcs, bool steal )
{
    mGet( outtrcs_, trcs, steal );
}


SynthSeis::RayModel::RflMdlSetRef
SynthSeis::RayModel::reflModels( bool sampled )
{ return sampled ? sampledreflmodels_ : reflmodels_; }
SynthSeis::RayModel::ConstRflMdlSetRef
SynthSeis::RayModel::reflModels( bool sampled ) const
{ return sampled ? sampledreflmodels_ : reflmodels_; }


const TimeDepthModel& SynthSeis::RayModel::zeroOffsetD2T() const
{
    return *zerooffsett2dmodel_;
}


void SynthSeis::RayModel::getD2T(
			ObjectSet<TimeDepthModel>& tdmodels, bool steal )
{
    mGet( t2dmodels_, tdmodels, steal );
}


SeisTrc* SynthSeis::RayModel::stackedTrc() const
{
    if ( outtrcs_.isEmpty() )
	return 0;

    SeisTrc* trc = new SeisTrc( *outtrcs_[0] );
    SeisTrcPropChg stckr( *trc );
    for ( int idx=1; idx<outtrcs_.size(); idx++ )
	stckr.stack( *outtrcs_[idx], false, mCast(float,idx) );

    return trc;
}



SynthSeis::DataSet::DataSet( const GenParams& gp, DataPack& dp )
    : NamedObject(gp.name_)
    , genpars_(gp)
    , datapack_(&dp)
{
}


SynthSeis::DataSet::~DataSet()
{
    deepErase( finald2tmodels_ );
    deepErase( raymodels_ );
}


SynthSeis::DataSet::MgrID SynthSeis::DataSet::getNewID()
{
    return MgrID( curdatasetid_++ );
}


void SynthSeis::DataSet::setName( const char* nm )
{
    NamedObject::setName( nm );
    datapack_->setName( nm );
}


SynthSeis::DataSet::size_type SynthSeis::DataSet::size() const
{
    return finald2tmodels_.size();
}


const SeisTrc* SynthSeis::DataSet::getTrace( idx_type idx ) const
{
    return validIdx(idx) ? gtTrace( idx ) : 0;
}


void SynthSeis::DataSet::setRayModels( const RayModelSet& rms )
{
    if ( &rms == &raymodels_ )
	return;

    deepErase( raymodels_ );
    for ( auto rm : rms )
	raymodels_ += new RayModel( *rm );
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
    for ( int idx=0; idx<d2tmdls.size(); idx++ )
	d2tmdls[idx]->shiftDepths( shft );
}


void SynthSeis::DataSet::updateD2TModels()
{
    deepErase( finald2tmodels_ );
    if ( raymodels_.isEmpty() )
	return;

    for ( int idx=0; idx<raymodels_.size(); idx++ )
    {
	const RayModel& rm = *raymodels_[idx];
	TimeDepthModel* zeroofsetd2tm = new TimeDepthModel();
	*zeroofsetd2tm = rm.zeroOffsetD2T();
	finald2tmodels_ += zeroofsetd2tm;
    }

    adjustD2TModelsToSRD( finald2tmodels_ );
}


ConstRefMan<ReflectivityModelSet> SynthSeis::DataSet::reflModels( int modelid,
							  bool sampled ) const
{
    if ( !raymodels_.validIdx(modelid) )
	return 0;
    return sampled ? raymodels_[modelid]->sampledreflmodels_
		   : raymodels_[modelid]->reflmodels_;
}


DataPack::FullID SynthSeis::DataSet::dataPackID() const
{
    return DataPack::FullID( dpMgrID(), dataPack().id() );
}



SynthSeis::PostStackDataSet::PostStackDataSet( const GenParams& gp, DPType& dp )
    : DataSet(gp,dp)
{
}


SynthSeis::PostStackDataSet::~PostStackDataSet()
{
}


DataPackMgr::ID	SynthSeis::PostStackDataSet::dpMgrID() const
{
    return DataPackMgr::FlatID();
}


const SeisTrc* SynthSeis::PostStackDataSet::gtTrace( idx_type idx ) const
{
    return postStackPack().trcBuf().get( idx );
}


SeisTrcBufDataPack& SynthSeis::PostStackDataSet::postStackPack()
{
    return static_cast<SeisTrcBufDataPack&>( *datapack_ );
}


const SeisTrcBufDataPack& SynthSeis::PostStackDataSet::postStackPack() const
{
    return static_cast<const SeisTrcBufDataPack&>( *datapack_ );
}


SynthSeis::PSBasedPostStackDataSet::PSBasedPostStackDataSet(
			const GenParams& gp, DPType& dp )
    : PostStackDataSet(gp,dp)
{
}


SynthSeis::StratPropDataSet::StratPropDataSet( const GenParams& sgp,
						SeisTrcBufDataPack& dp,
						const PropertyRef& pr )
    : PostStackDataSet( sgp, dp )
    , prop_(pr)
{
}
