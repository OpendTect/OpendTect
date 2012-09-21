/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";


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
{ return aimodels_.size(); }


void RayTracerRunner::setOffsets( TypeSet<float> offsets )
{ raypar_.set( RayTracer1D::sKeyOffset(), offsets ); }


void RayTracerRunner::addModel( const ElasticModel& aim, bool dosingle )
{
    if ( dosingle )
	aimodels_.erase();

    aimodels_ += aim; 
}


#define mErrRet(msg) { errmsg_ = msg; return false; }
bool RayTracerRunner::doPrepare( int nrthreads )
{
    deepErase( raytracers_ );

    if ( aimodels_.isEmpty() )
	mErrRet( "No AI model set" );

    if ( RayTracer1D::factory().getNames(false).isEmpty() )
	return false;

    BufferString errmsg;
    for ( int idx=0; idx<aimodels_.size(); idx++ )
    {
	RayTracer1D* rt1d = RayTracer1D::createInstance( raypar_, errmsg );
	if ( !rt1d )
	{
	    rt1d = RayTracer1D::factory().create( 
		    *RayTracer1D::factory().getNames(false)[0] );
	    rt1d->usePar( raypar_ );
	}
	raytracers_ += rt1d;
    }

    if ( raytracers_.isEmpty() && !errmsg.isEmpty() )
	mErrRet( errmsg.buf() );

    return true;
}


bool RayTracerRunner::doWork( od_int64 start, od_int64 stop, int thread )
{
    for ( int idx=start; idx<=stop; idx++, addToNrDone(1) )
    {
	const ElasticModel& aim = aimodels_[idx];
	if ( aim.isEmpty() ) 
	    continue;

	RayTracer1D* rt1d = raytracers_[idx];
	rt1d->setModel( aim );
	const bool parallel = maxNrThreads() > 1;
	if ( !rt1d->execute( parallel ) )
	    mErrRet( rt1d->errMsg() );
    }
    return true;
}

