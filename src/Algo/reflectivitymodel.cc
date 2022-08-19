/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "reflectivitymodel.h"

#include "iopar.h"
#include "manobjectset.h"
#include "odmemory.h"
#include "raytrace1d.h"
#include "typeset.h"


const char* AngleReflectivityModel::sKeyMeanRhob() 
{ return "Mean density"; }
const char* AngleReflectivityModel::sKeyMeanVp()
{ return "Mean Vp"; }
const char* AngleReflectivityModel::sKeyMeanVs()
{ return "Mean Vs"; }


// ReflectivityModelTrace

ReflectivityModelTrace::ReflectivityModelTrace( int nrspikes )
{
    setSize( nrspikes );
}


ReflectivityModelTrace::~ReflectivityModelTrace()
{
    delete [] reflectivities_;
}


bool ReflectivityModelTrace::isOK() const
{
    return reflectivities_ || sz_ < 1;
}


bool ReflectivityModelTrace::setSize( int sz, bool settonull )
{
    deleteAndZeroArrPtr( reflectivities_ );
    if ( sz > 0 )
    {
	mTryAlloc( reflectivities_, float_complex[sz] );
	if ( reflectivities_ && settonull )
	    OD::sysMemZero( reflectivities_, sz*sizeof(float_complex) );
    }

    sz_ = sz;
    return isOK();
}


// ReflectivityModelBase::Setup

void ReflectivityModelBase::Setup::fillPar( IOPar& iop ) const
{
    TimeDepthModelSet::Setup::fillPar( iop );
    //TODO
}


bool ReflectivityModelBase::Setup::usePar( const IOPar& iop )
{
    if ( !TimeDepthModelSet::Setup::usePar(iop) )
	return false;
    //TODO
    return true;
}


// ReflectivityModelBase

ReflectivityModelBase::ReflectivityModelBase( const ElasticModel& emodel,
				const ReflectivityModelBase::Setup& rmsu,
				const TypeSet<float>* axisvals,
				float* velmax )
    : TimeDepthModelSet(emodel,rmsu,rmsu.offsetdomain_ ? axisvals : nullptr,
		        velmax)
{
    if ( !rmsu.offsetdomain_ && !axisvals )
    {
	isbad_ = false;
	pErrMsg( "No angles provided for angle-based model" );
    }

    nroffs_ = rmsu.offsetdomain_ || !axisvals || axisvals->isEmpty()
	|| (axisvals->size()==1 && mIsZero(axisvals->first(),1e-4f))
		       ? nrModels()
		       : axisvals->size();

    const int layersize = nrLayers();
    if ( rmsu.withangles_ )
    {
	mTryAlloc( sini_, float[nroffs_*layersize] );
	if ( !sini_ )
	{
	    isbad_ = true;
	    return;
	}

	mTryAlloc( sinarr_, float*[nroffs_] );
	if ( !sini_ )
	{
	    deleteAndZeroArrPtr( sini_ );
	    isbad_ = true;
	    return;
	}
    }

    if ( rmsu.withreflectivity_ )
	reflectivities_ = new RefObjectSet<ReflectivityModelTrace>;

    float* sinarr = sini_;
    for ( int idx=0; idx<nroffs_; idx++ )
    {
	if ( sinarr_ )
	{
	    sinarr_[idx] = sinarr;
	    sinarr += layersize;
	}

	if ( reflectivities_ )
	{
	    auto* refmodel = new ReflectivityModelTrace( nrSpikes() );
	    if ( !refmodel || !refmodel->isOK() )
	    {
		delete refmodel;
		isbad_ = true;
		break;
	    }

	    reflectivities_->add( refmodel );
	}
    }

    if ( isbad_ && reflectivities_ )
	reflectivities_->setEmpty();
}


ReflectivityModelBase::ReflectivityModelBase( const ElasticModel& emodel,
					      const TypeSet<float>& anglevals,
				    const ReflectivityModelBase::Setup& rmsu )
    : ReflectivityModelBase(emodel,rmsu,&anglevals)
{
}


ReflectivityModelBase::~ReflectivityModelBase()
{
    delete reflectivities_;
    delete [] sinarr_;
    delete [] sini_;
}


int ReflectivityModelBase::nrRefModels() const
{
    return nroffs_;
}


int ReflectivityModelBase::nrLayers() const
{
    return modelSize()-1;
}


int ReflectivityModelBase::nrSpikes() const
{
    return nrLayers()-1;
}


bool ReflectivityModelBase::hasAngles() const
{
    return sinarr_;
}


bool ReflectivityModelBase::hasReflectivities() const
{
    return reflectivities_;
}


float* ReflectivityModelBase::getAngles( int ioff )
{
#ifdef __debug__
    if ( !hasAngles() || ioff < 0 || ioff >= nroffs_ )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    return sinarr_[ioff];
}


float ReflectivityModelBase::getSinAngle( int ioff, int idz ) const
{
#ifdef __debug__
    if ( !hasAngles() || ioff < 0 || ioff >= nroffs_ ||
	 idz < 0 || idz>=nrLayers() )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    return sinarr_[ioff][idz];
}


const ReflectivityModelTrace*
ReflectivityModelBase::getReflectivities( int ioff ) const
{
    return mSelf().getReflectivities( ioff );
}


ReflectivityModelTrace* ReflectivityModelBase::getReflectivities( int ioff )
{
#ifdef __debug__
    if ( !hasReflectivities() || !reflectivities_->validIdx(ioff) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    return reflectivities_->get( ioff );
}


const float* ReflectivityModelBase::getReflTimes( int ioff ) const
{
#ifdef __debug__
    if ( ioff >=0 && !get(ioff) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    const TimeDepthModel& tdmodel = ioff < 0 ? getDefaultModel()
					      : *get( ioff );
    return tdmodel.getTimes()+1;
}


const float* ReflectivityModelBase::getReflDepths() const
{
    return getDefaultModel().getDepths()+1;
}


float_complex* ReflectivityModelBase::getRefs( int ioff )
{
    return getReflectivities( ioff )->arr();
}


bool ReflectivityModelBase::isSpikeDefined( int ioff, int idz ) const
{
#ifdef __debug__
    if ( !hasReflectivities() || !reflectivities_->validIdx(ioff) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    const ReflectivityModelTrace* rm = reflectivities_->get( ioff );
    return !mIsUdf(rm->arr()[idz]) &&
	   !mIsUdf(getReflDepths()[idz]) &&
	   !mIsUdf(getReflTimes()[idz]) &&
	   !mIsUdf(getReflTimes(ioff)[idz]);
}


// OffsetReflectivityModel

OffsetReflectivityModel::OffsetReflectivityModel(
				const ElasticModel& emodel,
				const OffsetReflectivityModel::Setup& rmsu,
				const TypeSet<float>* axisvals, float* velmax )
    : ReflectivityModelBase( emodel, rmsu, axisvals, velmax )
{
}


// AngleReflectivityModel::Setup

void AngleReflectivityModel::Setup::fillPar( IOPar& iop ) const
{
    ReflectivityModelBase::Setup::fillPar( iop );
    iop.set( sKey::Azimuth(), azimuth_ );
    iop.set( sKeyMeanRhob(), d0_ );
    iop.set( sKeyMeanVp(), a0_ );
    iop.set( sKeyMeanVs(), b0_ );
}


bool AngleReflectivityModel::Setup::usePar( const IOPar& iop )
{
    if ( !ReflectivityModelBase::Setup::usePar(iop) )
	return false;

    iop.get(sKey::Azimuth(), azimuth_);
    iop.get( sKeyMeanRhob(), d0_ );
    iop.get( sKeyMeanVp(), a0_ );
    iop.get( sKeyMeanVs(), b0_ );

    return true;
}


// AngleReflectivityModel

AngleReflectivityModel::AngleReflectivityModel(
				const ElasticModel& emodel,
				const TypeSet<float>& anglevals,
				const AngleReflectivityModel::Setup& rmsu )
    : ReflectivityModelBase( emodel, anglevals, rmsu )
    , azimuth_(rmsu.azimuth_)
    , a0_(rmsu.a0_)
    , d0_(rmsu.d0_)
    , b0_(rmsu.b0_)
{
}


AngleReflectivityModel::AngleReflectivityModel(
				const ElasticModel& emodel,
				const TypeSet<float>& anglevals,
				double azimuth )
    : AngleReflectivityModel( emodel, anglevals,
			      AngleReflectivityModel::Setup(azimuth) )
{
}


// ReflectivityModelSet

ReflectivityModelSet::ReflectivityModelSet( const IOPar& iop )
    : createpars_(*new IOPar(iop))
    , refmodels_(*new RefObjectSet<const ReflectivityModelBase>)
{
}


ReflectivityModelSet::~ReflectivityModelSet()
{
    delete &createpars_;
    delete &refmodels_;
}


bool ReflectivityModelSet::hasSameParams( const ReflectivityModelSet& oth) const
{
    return hasSameParams( oth.createpars_ );
}


bool ReflectivityModelSet::hasSameParams( const IOPar& othcreatepars ) const
{
    const IOPar& iop = createpars_;
    const IOPar& othiop = othcreatepars;
    BufferString type1str, type2str;
    if ( !iop.get(sKey::Type(),type1str) ||
	 !othiop.get(sKey::Type(),type2str) || type1str != type2str )
	return false;

    if ( RayTracer1D::factory().hasName(type1str) )
    {
	uiString msg1, msg2;
	ConstPtrMan<RayTracer1D> rt = RayTracer1D::createInstance( iop, msg1 );
	ConstPtrMan<RayTracer1D> othrt =
				     RayTracer1D::createInstance( othiop, msg2);
	if ( !rt || !othrt || !msg1.isEmpty() || !msg2.isEmpty() )
	    return false;

	return rt->hasSameParams( *othrt.ptr() );
    }

    return false;

}


bool ReflectivityModelSet::validIdx( int modlidx ) const
{
    return refmodels_.validIdx( modlidx );
}


int ReflectivityModelSet::nrModels() const
{
    return refmodels_.size();
}


const ReflectivityModelBase* ReflectivityModelSet::get( int idx ) const
{
    return refmodels_.validIdx( idx ) ? refmodels_.get( idx ) : nullptr;
}


void ReflectivityModelSet::add( const ReflectivityModelBase& refmodel )
{
    refmodels_.add( &refmodel );
}


void ReflectivityModelSet::getOffsets( TypeSet<float>& offsets ) const
{
    if ( !createpars_.get(RayTracer1D::sKeyOffset(),offsets) )
    {
	pErrMsg("Should not happen");

	offsets.setEmpty();
	const float step = 100.f;
	for ( int idx=0; idx<nrModels(); idx++ )
	    offsets += step * idx;
    }
}


void ReflectivityModelSet::getTWTrange( Interval<float>& twtrg,
					bool zeroff ) const
{
    twtrg.setUdf();
    for ( const auto* refmodel : refmodels_ )
    {
	if ( refmodel->modelSize() < 2 )
	    continue;

	if ( zeroff )
	{
	    twtrg.include( refmodel->getDefaultModel().getTimes()[1] );
	    twtrg.include( refmodel->getDefaultModel().getLastTime() );
	    continue;
	}

	for ( int idx=0; idx<refmodel->size(); idx++ )
	{
	    const TimeDepthModel* tdmodel = refmodel->get( idx );
	    twtrg.include( tdmodel->getTimes()[1] );
	    twtrg.include( tdmodel->getLastTime() );
	}
    }
}


void ReflectivityModelSet::use( const ObjectSet<const TimeDepthModel>& tdmodels,
				bool defonly )
{
    int imdl=0;
    for ( const auto* tdmodel : tdmodels )
	use( *tdmodel, imdl++, defonly );
}


void ReflectivityModelSet::use( const TimeDepthModel& tdmodel, int imdl,
				bool defonly )
{
    if ( !refmodels_.validIdx(imdl) )
	{ pErrMsg("Invalid idx"); DBG::forceCrash(true); }

    auto& tdmodelset =
		const_cast<ReflectivityModelBase&>( *refmodels_.get(imdl) );
    tdmodelset.forceTimes( tdmodel, defonly );
}
