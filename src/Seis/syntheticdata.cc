/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:	       July 2011
________________________________________________________________________

-*/

#include "syntheticdata.h"
#include "syntheticdataimpl.h"

#include "datapackbase.h"
#include "stratsynthgenparams.h"
#include "velocitycalc.h"

#include "array1dinterpol.h"
#include "angles.h"
#include "arrayndimpl.h"
#include "binidvalset.h"
#include "datapackbase.h"
#include "elasticpropsel.h"
#include "envvars.h"
#include "fourier.h"
#include "fftfilter.h"
#include "flatposdata.h"
#include "mathfunc.h"
#include "propertyref.h"
#include "raytracerrunner.h"
#include "separstr.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "timeser.h"
#include "waveletmanager.h"

#define sDefaultAngleRange Interval<float>( 0.0f, 30.0f )
#define sDefaultOffsetRange StepInterval<float>( 0.f, 6000.f, 100.f )

#include "raytrace1d.h"

static const char* sKeyDispPar()		{ return "Display Parameter"; }

//SynthFVSpecificDispPars
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


//SyntheticData
SyntheticData::SyntheticData( const SynthGenParams& sgp, DataPack& dp )
    : NamedObject(sgp.name_)
    , datapack_(&dp)
    , id_(-1)
    , raymodels_(new ObjectSet<SyntheticData::RayModel>())
    , reflectivitymodels_(new ReflectivityModelSet)
{
}


SyntheticData::~SyntheticData()
{
    deepErase( d2tmodels_ );
    deepErase( zerooffsd2tmodels_ );
    removePack();
}


void SyntheticData::setName( const char* nm )
{
    NamedObject::setName( nm );
    datapack_->setName( nm );
}


void SyntheticData::removePack()
{
    datapack_ = 0;
}


float SyntheticData::getTime( float dpt, int seqnr ) const
{
    return zerooffsd2tmodels_.validIdx( seqnr )
	? zerooffsd2tmodels_[seqnr]->getTime( dpt ) : mUdf( float );
}


float SyntheticData::getDepth( float time, int seqnr ) const
{
    return zerooffsd2tmodels_.validIdx( seqnr )
	? zerooffsd2tmodels_[seqnr]->getDepth( time ) : mUdf( float );
}


bool SyntheticData::isAngleStack() const
{
    TypeSet<float> offsets;
    raypars_.get( RayTracer1D::sKeyOffset(), offsets );
    return !isPS() && offsets.size()>1;
}


void SyntheticData::fillGenParams( SynthGenParams& sgp ) const
{
    sgp.inpsynthnm_.setEmpty();
    sgp.raypars_ = raypars_;
    sgp.wvltnm_ = wvltnm_;
    sgp.name_ = name();
    sgp.synthtype_ = synthType();
}


void SyntheticData::useGenParams( const SynthGenParams& sgp )
{
    raypars_ = sgp.raypars_;
    wvltnm_ = sgp.wvltnm_;
    setName( sgp.name_ );
}


void SyntheticData::fillDispPar( IOPar& par ) const
{
    disppars_.fillPar( par );
}


void SyntheticData::useDispPar( const IOPar& par )
{
    disppars_.usePar( par );
}


void SyntheticData::setRayModels( ObjectSet<RayModel>& raymodels )
{
    raymodels = *raymodels_;
}


bool SyntheticData::haveSameRM( const IOPar& par1, const IOPar& par2 ) const
{
    uiString msg;
    PtrMan<RayTracer1D> rt1d1 = RayTracer1D::createInstance( par1, msg );
    PtrMan<RayTracer1D> rt1d2 = RayTracer1D::createInstance( par2, msg );
    if ( !rt1d1 || !rt1d2 )
	return false;

    return rt1d1->hasSameParams( *rt1d2 );
}


void SyntheticData::adjustD2TModels( ObjectSet<TimeDepthModel>& d2tmodels )
{
    for ( int idx=0; idx<d2tmodels.size(); idx++ )
    {
	TimeDepthModel* d2tmodel = d2tmodels[idx];
	if ( !d2tmodel ) continue;
	const int d2tmsz = d2tmodel->size();
	TypeSet<float> depths;
	depths.setSize( d2tmsz );
	TypeSet<float> times;
	times.setSize( d2tmsz );
	for ( int isamp=0; isamp<d2tmsz; isamp++ )
	{
	    depths[isamp] = d2tmodel->getDepth( isamp ) -
				mCast(float,SI().seismicReferenceDatum());
	    times[isamp] = d2tmodel->getTime( isamp );
	}

	d2tmodel->setModel( depths.arr(), times.arr(), d2tmsz );
    }
}


void SyntheticData::updateD2TModels()
{
    deepErase( d2tmodels_ );
    deepErase( zerooffsd2tmodels_ );
    ObjectSet<TimeDepthModel> zeroofsetd2tms;
    if ( raymodels_->isEmpty() )
	return;

    for ( int idx=0; idx<raymodels_->size(); idx++ )
    {
	if ( !(*raymodels_)[idx] )
	    continue;

	TimeDepthModel* zeroofsetd2tm = new TimeDepthModel();
	SyntheticData::RayModel* rm = (*raymodels_)[idx];
	rm->getZeroOffsetD2T( *zeroofsetd2tm );
	zeroofsetd2tms += zeroofsetd2tm;

	ObjectSet<TimeDepthModel> d2tmodel;
	rm->getD2T( d2tmodel, false );
	adjustD2TModels( d2tmodel );
	while( d2tmodel.size() )
	{
	    TimeDepthModel* d2tm =
			new TimeDepthModel(*d2tmodel.removeSingle( 0 ) );
	    d2tmodels_ += d2tm;
	}

	deepErase( d2tmodel );
    }

    adjustD2TModels( zeroofsetd2tms );
    while( !zeroofsetd2tms.isEmpty() )
	zerooffsd2tmodels_ += zeroofsetd2tms.removeSingle( 0 );
}


RefMan<ReflectivityModelSet> SyntheticData::getRefModels(int modelid,
							 bool sampled )
{
    if ( !raymodels_->validIdx(modelid) )
	return 0;
    return sampled ? (*raymodels_)[modelid]->sampledrefmodels_
		   : (*raymodels_)[modelid]->refmodels_;
}


SyntheticData::RayModel::RayModel( const RayTracer1D& rt1d, int nroffsets )
    : zerooffset2dmodel_(0)
    , refmodels_(new ReflectivityModelSet)
    , sampledrefmodels_(new ReflectivityModelSet)
{
    for ( int idx=0; idx<nroffsets; idx++ )
    {
	ReflectivityModel* refmodel = new ReflectivityModel();
	rt1d.getReflectivity( idx, *refmodel );

	TimeDepthModel* t2dm = new TimeDepthModel();
	rt1d.getTDModel( idx, *t2dm );

	refmodels_->add( refmodel );
	t2dmodels_ += t2dm;
	if ( !idx )
	{
	    zerooffset2dmodel_ = new TimeDepthModel();
	    rt1d.getZeroOffsTDModel( *zerooffset2dmodel_ );
	}
    }
}


SyntheticData::RayModel::~RayModel()
{
    deepErase( outtrcs_ );
    deepErase( t2dmodels_ );
    delete zerooffset2dmodel_;
}


void SyntheticData::RayModel::forceReflTimes(const StepInterval<float>& si )
{
    for ( int idx=0; idx<refmodels_->size(); idx++ )
    {
	ReflectivityModel& refmodel =
			const_cast<ReflectivityModel&>(*refmodels_->get(idx));
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

void SyntheticData::RayModel::getTraces(
		    ObjectSet<SeisTrc>& trcs, bool steal )
{
    mGet( outtrcs_, trcs, steal );
}


RefMan<ReflectivityModelSet>& SyntheticData::RayModel::getRefs( bool sampled )
{
    return sampled ? sampledrefmodels_ : refmodels_;
}


void SyntheticData::RayModel::getZeroOffsetD2T( TimeDepthModel& tdms )
{
    tdms = *zerooffset2dmodel_;
}


void SyntheticData::RayModel::getD2T(
			ObjectSet<TimeDepthModel>& tdmodels, bool steal )
{
    mGet( t2dmodels_, tdmodels, steal );
}

const SeisTrc* SyntheticData::RayModel::stackedTrc() const
{
    if ( outtrcs_.isEmpty() )
    return 0;

    SeisTrc* trc = new SeisTrc( *outtrcs_[0] );
    SeisTrcPropChg stckr( *trc );
    for ( int idx=1; idx<outtrcs_.size(); idx++ )
    stckr.stack( *outtrcs_[idx], false, mCast(float,idx) );

    return trc;
}


//PostStackSyntheticData
PostStackSyntheticData::PostStackSyntheticData( const SynthGenParams& sgp,
						SeisTrcBufDataPack& dp)
    : SyntheticData(sgp,dp)
{
    useGenParams( sgp );
    DataPackMgr::ID pmid = DataPackMgr::FlatID();
    DPM( pmid ).add( &dp );
    datapackid_ = DataPack::FullID( pmid, dp.id());
}


PostStackSyntheticData::~PostStackSyntheticData()
{
}


const SeisTrc* PostStackSyntheticData::getTrace( int seqnr ) const
{ return postStackPack().trcBuf().get( seqnr ); }


SeisTrcBufDataPack& PostStackSyntheticData::postStackPack()
{
    return static_cast<SeisTrcBufDataPack&>( *datapack_ );
}


const SeisTrcBufDataPack& PostStackSyntheticData::postStackPack() const
{
    return static_cast<const SeisTrcBufDataPack&>( *datapack_ );
}


//PSBasedPostStackSyntheticData
PSBasedPostStackSyntheticData::PSBasedPostStackSyntheticData(
	const SynthGenParams& sgp, SeisTrcBufDataPack& sdp )
    : PostStackSyntheticData(sgp,sdp)
{
    useGenParams( sgp );
}


PSBasedPostStackSyntheticData::~PSBasedPostStackSyntheticData()
{}


void PSBasedPostStackSyntheticData::fillGenParams( SynthGenParams& sgp ) const
{
    SyntheticData::fillGenParams( sgp );
    sgp.inpsynthnm_ = inpsynthnm_;
    sgp.anglerg_ = anglerg_;
}


void PSBasedPostStackSyntheticData::useGenParams( const SynthGenParams& sgp )
{
    SyntheticData::useGenParams( sgp );
    inpsynthnm_ = sgp.inpsynthnm_;
    anglerg_ = sgp.anglerg_;
}


//StratPropSyntheticData
StratPropSyntheticData::StratPropSyntheticData( const SynthGenParams& sgp,
						    SeisTrcBufDataPack& dp,
						    const PropertyRef& pr )
    : PostStackSyntheticData( sgp, dp )
    , prop_(pr)
{}
