/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/


#include "raytracerrunner.h"

RayTracerRunner::RayTracerRunner( const TypeSet<ElasticModel>& aims,
				  const IOPar& raypars )
    : aimodels_(aims)
    , raypar_(raypars)
{
}


RayTracerRunner::RayTracerRunner( const IOPar& raypars )
    : raypar_(raypars)
{}


RayTracerRunner::~RayTracerRunner()
{ deepErase( raytracers_ );}


od_int64 RayTracerRunner::nrIterations() const
{
    return totalnr_;
}


void RayTracerRunner::setOffsets( TypeSet<float> offsets )
{ raypar_.set( RayTracer1D::sKeyOffset(), offsets ); }


void RayTracerRunner::addModel( const ElasticModel& aim, bool dosingle )
{
    if ( dosingle )
	aimodels_.erase();

    aimodels_ += aim;
}

#define mErrRet(msg) { errmsg_ = msg; return false; }

bool RayTracerRunner::prepareRayTracers()
{
    deepErase( raytracers_ );

    if ( aimodels_.isEmpty() )
	mErrRet( toUiString("No AI model set") );

    if ( RayTracer1D::factory().getNames().isEmpty() )
	return false;

    totalnr_ = 0;
    uiString errmsg;
    for ( int idx=0; idx<aimodels_.size(); idx++ )
    {
	RayTracer1D* rt1d = RayTracer1D::createInstance( raypar_, errmsg );
	if ( !rt1d )
	{
	    deepErase( raytracers_ );
	    mErrRet( errmsg );
	}

	rt1d->usePar( raypar_ );

	if ( !rt1d->setModel(aimodels_[idx]) )
	{
	    errmsg = tr( "Wrong input for raytracing on model: %1" ).arg(idx+1);
	    errmsg.append( rt1d->errMsg(), true );

	    deepErase( raytracers_ );
	    delete rt1d;
	    mErrRet( errmsg );
	}

	totalnr_ += rt1d->totalNr();
	raytracers_ += rt1d;
    }

    return true;
}


od_int64 RayTracerRunner::nrDone() const
{
    od_int64 nrdone = 0;
    for ( int modelidx=0; modelidx<raytracers_.size(); modelidx++ )
	nrdone += raytracers_[modelidx]->nrDone();
    return nrdone;
}


int RayTracerRunner::modelIdx( od_int64 idx, bool& startlayer ) const
{
    od_int64 stopidx = -1;
    startlayer = false;
    for ( int modelidx=0; modelidx<raytracers_.size(); modelidx++ )
    {
	if ( idx == (stopidx + 1) )
	    startlayer = true;

	stopidx += raytracers_[modelidx]->totalNr();
	if ( stopidx>=idx )
	    return modelidx;
    }

    return -1;
}


bool RayTracerRunner::executeParallel( bool parallel )
{
    if ( !prepareRayTracers() )
	return false;
    return ParallelTask::executeParallel( parallel );
}


bool RayTracerRunner::doWork( od_int64 start, od_int64 stop, int thread )
{
    bool startlayer = false;
    int startmdlidx = modelIdx( start, startlayer );
    if ( !startlayer ) startmdlidx++;
    const int stopmdlidx = modelIdx( stop, startlayer );
    for ( int idx=startmdlidx; idx<=stopmdlidx; idx++ )
    {
	const ElasticModel& aim = aimodels_[idx];
	if ( aim.isEmpty() )
	    continue;

	RayTracer1D* rt1d = raytracers_[idx];
	const bool parallel = maxNrThreads() > 1;
	if ( !rt1d->executeParallel( parallel ) )
	    mErrRet( rt1d->errMsg() );
    }
    return true;
}
