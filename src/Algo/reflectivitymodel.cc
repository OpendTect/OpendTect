/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A. Huck
 * DATE     : Dec 2021
-*/


#include "reflectivitymodel.h"

#include "ptrman.h"
#include "typeset.h"


// ReflectivityModelTrace

ReflectivityModelTrace::ReflectivityModelTrace( int nrspikes )
{
    mTryAlloc( reflectivities_, float_complex[nrspikes] );
}


ReflectivityModelTrace::~ReflectivityModelTrace()
{
    delete [] reflectivities_;
}


bool ReflectivityModelTrace::isOK() const
{
    return reflectivities_;
}


// ReflectivityModelSet::Setup

void ReflectivityModelSet::Setup::fillPar( IOPar& iop ) const
{
    TimeDepthModelSet::Setup::fillPar( iop );
    //TODO
}


bool ReflectivityModelSet::Setup::usePar( const IOPar& iop )
{
    if ( !TimeDepthModelSet::Setup::usePar(iop) )
	return false;
    //TODO
    return true;
}


// ReflectivityModelSet

ReflectivityModelSet::ReflectivityModelSet( const ElasticModel& emodel,
				const ReflectivityModelSet::Setup& rmsu,
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

    const int nrmodels = rmsu.offsetdomain_ || !axisvals || axisvals->isEmpty()
	|| (axisvals->size()==1 && mIsZero(axisvals->first(),1e-4f))
		       ? nrModels()
		       : axisvals->size();
    for ( int idx=0; idx<nrmodels; idx++ )
    {
	auto* refmodel = new ReflectivityModelTrace( modelSize() - 2 );
	if ( !refmodel || !refmodel->isOK() )
	{
	    delete refmodel;
	    isbad_ = true;
	    break;
	}

	reflectivities_.add( refmodel );
    }

    if ( isbad_ )
	deepErase( reflectivities_ );
}


ReflectivityModelSet::ReflectivityModelSet( const ElasticModel& emodel,
					    const TypeSet<float>& anglevals,
				    const ReflectivityModelSet::Setup& rmsu )
    : ReflectivityModelSet(emodel,rmsu,&anglevals)
{
}


ReflectivityModelSet::~ReflectivityModelSet()
{
    deepErase( reflectivities_ );
}


int ReflectivityModelSet::nrRefModels() const
{
    return reflectivities_.size();
}


float_complex* ReflectivityModelSet::getRefs( int imodel )
{
    return reflectivities_.get( imodel )->getReflectivities();
}


bool ReflectivityModelSet::isDefined( int imdl, int idz ) const
{
    const ReflectivityModelTrace* rm = reflectivities_.get( imdl );
    const TimeDepthModel& defd2t = getDefaultModel();
    const TimeDepthModel* t2d = get( imdl );
    return !mIsUdf(rm->getReflectivities()[idz]) &&
	   !mIsUdf(defd2t.getTimes()[idz+1]) &&
	   !mIsUdf(t2d->getTimes()[idz+1]) &&
	   !mIsUdf(t2d->getDepths()[idz+1] );
}


// OffsetReflectivityModelSet

OffsetReflectivityModelSet::OffsetReflectivityModelSet(
				const ElasticModel& emodel,
				const OffsetReflectivityModelSet::Setup& rmsu,
				const TypeSet<float>* axisvals, float* velmax )
    : ReflectivityModelSet( emodel, rmsu, axisvals, velmax )
{
}


// AngleReflectivityModelSet::Setup

void AngleReflectivityModelSet::Setup::fillPar( IOPar& iop ) const
{
    ReflectivityModelSet::Setup::fillPar( iop );
    //TODO
}


bool AngleReflectivityModelSet::Setup::usePar( const IOPar& iop )
{
    if ( !ReflectivityModelSet::Setup::usePar(iop) )
	return false;
    //TODO
    return true;
}


// AngleReflectivityModelSet

AngleReflectivityModelSet::AngleReflectivityModelSet(
				const ElasticModel& emodel,
				const TypeSet<float>& anglevals,
				const AngleReflectivityModelSet::Setup& rmsu )
    : ReflectivityModelSet( emodel, anglevals, rmsu )
    , azimuth_(rmsu.azimuth_)
    , a0_(rmsu.a0_)
    , d0_(rmsu.d0_)
    , b0_(rmsu.b0_)
{
}


AngleReflectivityModelSet::AngleReflectivityModelSet(
				const ElasticModel& emodel,
				const TypeSet<float>& anglevals,
				double azimuth )
    : AngleReflectivityModelSet( emodel, anglevals,
				 AngleReflectivityModelSet::Setup(azimuth) )
{
}
