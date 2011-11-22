/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: raytracerrunner.cc,v 1.8 2011-11-22 10:27:45 cvsbruno Exp $";


#include "raytracerrunner.h"

RayTracerRunner::RayTracerRunner( const TypeSet<ElasticModel>& aims, 
				    const IOPar& raypars ) 
    : Executor("Ray tracer runner") 
    , aimodels_(aims)
    , nrdone_(0)
    , raypar_(raypars)		
{}


RayTracerRunner::~RayTracerRunner() 
{
    deepErase( raytracers_ );
}


#define mErrRet(msg) { errmsg_ = msg; return ErrorOccurred(); }

int RayTracerRunner::nextStep() 
{
    if ( nrdone_ == 0 )
    {
	if ( aimodels_.isEmpty() )
	    mErrRet( "No AI model set" );

	deepErase( raytracers_ );
    }
    if ( nrdone_ == totalNr() )
	return Finished();

    const ElasticModel& aim = aimodels_[nrdone_];
    if ( aim.isEmpty() )
	{ nrdone_ ++; return Executor::MoreToDo(); }

    if ( RayTracer1D::factory().getNames(false).isEmpty() )
	return false;

    BufferString errmsg;
    RayTracer1D* rt1d = RayTracer1D::createInstance( raypar_, errmsg );
    if ( !rt1d )
    {
	rt1d = RayTracer1D::factory().create( 
		*RayTracer1D::factory().getNames(false)[0] );
	rt1d->usePar( raypar_ );
    }
    if ( !rt1d && !errmsg.isEmpty() ) 
	mErrRet( errmsg.buf() );

    rt1d->setModel( aim );
    raytracers_ += rt1d;

    if ( !rt1d->execute() )
	mErrRet( rt1d->errMsg() );

    nrdone_ ++;
    return MoreToDo();
}

