/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/


#include "raytracerrunner.h"

#include "raytrace1d.h"



RayTracerRunner::RayTracerRunner( const ElasticModelSet& elmdls,
				  const IOPar& raypars )
    : TaskGroup()
    , raypar_(raypars)
    , elasticmodels_(&elmdls)
{
    showCumulativeCount( true );
}


RayTracerRunner::RayTracerRunner( const IOPar& raypars )
    : TaskGroup()
    , raypar_(raypars)
    , elasticmodels_(0)
{
    showCumulativeCount( true );
}


RayTracerRunner::~RayTracerRunner()
{}


void RayTracerRunner::setOffsets( const TypeSet<float>& offsets )
{
    raypar_.set( RayTracer1D::sKeyOffset(), offsets );
}


void RayTracerRunner::setModel( const ElasticModelSet& aim )
{
    elasticmodels_ = &aim;
}


#define mErrRet(msg) { errmsg_ = msg; return false; }

bool RayTracerRunner::doPrepare()
{
    raytracers_.setEmpty();
    results_.setEmpty();
    if ( !elasticmodels_ || elasticmodels_->isEmpty() )
	mErrRet( tr("No AI model set") );

    if ( RayTracer1D::factory().isEmpty() )
	return false;

    uiString errmsg;
    for ( int idx=0; idx<elasticmodels_->size(); idx++ )
    {
	RayTracer1D* rt1d = RayTracer1D::createInstance( raypar_, errmsg );
	if ( !rt1d )
	{
	    deepErase( raytracers_ );
	    mErrRet( errmsg );
	}

	rt1d->usePar( raypar_ );
	rt1d->doParallel( parallel_ );

	if ( !rt1d->setModel(*elasticmodels_->get(idx)) )
	{
	    errmsg = tr("Wrong input for raytracing on model %1").arg( idx+1 );
	    errmsg.appendPhrase( rt1d->errMsg() );

	    delete rt1d;
	    mErrRet( errmsg );
	}

	addTask( rt1d );
	raytracers_ += rt1d;
    }

    return true;
}


bool RayTracerRunner::execute()
{
    setEmpty();

    return doPrepare() && doFinish( TaskGroup::execute() );
}


bool RayTracerRunner::doFinish( bool success )
{
    if ( success )
    {
	for ( int idx=0; idx<raytracers_.size(); idx++ )
	{
	    ConstRefMan<RayTracerData> res = raytracers_[idx]->results();
	    results_ += const_cast<RayTracerData*>( res.ptr() );
	}
    }

    raytracers_.setEmpty();

    return success;
}
