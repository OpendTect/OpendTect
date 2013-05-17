/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "raytracerrunner.h"

RayTracerRunner::RayTracerRunner( const TypeSet<ElasticModel>& aims, 
				    const IOPar& raypars ) 
    : aimodels_(aims)
    , raypar_(raypars)		
{}


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
	mErrRet( "No AI model set" );

    if ( RayTracer1D::factory().getNames(false).isEmpty() )
	return false;

    BufferString errmsg;
    totalnr_ = 0;
    for ( int idx=0; idx<aimodels_.size(); idx++ )
    {
	RayTracer1D* rt1d = RayTracer1D::createInstance( raypar_, errmsg );
	if ( !rt1d )
	{
	    rt1d = RayTracer1D::factory().create( 
		    *RayTracer1D::factory().getNames(false)[0] );
	    rt1d->usePar( raypar_ );
	}

	rt1d->setModel( aimodels_[idx] );
	totalnr_ += rt1d->totalNr();
	raytracers_ += rt1d;
    }

    if ( raytracers_.isEmpty() && !errmsg.isEmpty() )
	mErrRet( errmsg.buf() );

    return true;
}


od_int64 RayTracerRunner::nrDone() const
{
    od_int64 nrdone = 0;
    for ( int modelidx=0; modelidx<raytracers_.size(); modelidx++ )
	nrdone += raytracers_[modelidx]->nrDone();
    return nrdone;
}


int RayTracerRunner::curModelIdx( od_int64 idx ) const
{
    od_int64 stopidx = 0;
    for ( int modelidx=0; modelidx<raytracers_.size(); modelidx++ )
    {
	stopidx += raytracers_[modelidx]->totalNr();
	if ( stopidx>idx )
	    return modelidx;
    }

    return -1;
}


bool RayTracerRunner::doWork( od_int64 start, od_int64 stop, int thread )
{
    int startmdlidx = curModelIdx(start);
    if ( start ) startmdlidx++;
    const int stopmdlidx = curModelIdx(stop);
    for ( int idx=startmdlidx; idx<=stopmdlidx; idx++ )
    {
	const ElasticModel& aim = aimodels_[idx];
	if ( aim.isEmpty() ) 
	    continue;

	RayTracer1D* rt1d = raytracers_[idx];
	const bool parallel = maxNrThreads() > 1;
	if ( !rt1d->execute( parallel ) )
	    mErrRet( rt1d->errMsg() );
    }
    return true;
}

