/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: raytracerrunner.cc,v 1.6 2011-10-06 14:17:33 cvsbruno Exp $";


#include "raytracerrunner.h"

RayTracerRunner::RayTracerRunner( const TypeSet<ElasticModel>& aims, 
				    const TypeSet<float>& offs, 
				    const RayTracer1D::Setup& su )
    : Executor("Ray tracer runner") 
    , aimodels_(aims)
    , raysetup_(su)
    , offsets_(offs)
    , nrdone_(0)
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

	if ( offsets_.isEmpty() )
	    offsets_ += 0;

	deepErase( raytracers_ );
    }
    if ( nrdone_ == totalNr() )
	return Finished();

    const ElasticModel& aim = aimodels_[nrdone_];
    if ( aim.isEmpty() )
	{ nrdone_ ++; return Executor::MoreToDo(); }

    if ( RayTracer1D::factory().getNames(false).isEmpty() )
	return false;

    BufferString type = RayTracer1D::factory().getDefaultName();
    if ( type.isEmpty() )
	type = *RayTracer1D::factory().getNames(false)[0];

    RayTracer1D* rt1d = RayTracer1D::factory().create( type );
    rt1d->setup() = raysetup_;
    rt1d->setModel( aim );
    rt1d->setOffsets( offsets_ );
    raytracers_ += rt1d;

    if ( !rt1d->execute() )
	mErrRet( rt1d->errMsg() );

    nrdone_ ++;
    return MoreToDo();
}

