/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: raytracerrunner.cc,v 1.2 2011-05-25 15:49:02 cvsbruno Exp $";


#include "raytracerrunner.h"

RayTracerRunner::RayTracerRunner( const ObjectSet<AIModel>& aims, 
				    const TypeSet<float>& offs, 
				    const RayTracer1D::Setup& su )
    : Executor("Ray tracer runner") 
    , aimodels_(aims)
    , raysetup_(su)
    , offsets_(offs)
{}


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
    const AIModel& aim = *aimodels_[nrdone_];
    if ( aim.isEmpty() )
	{ nrdone_ ++; return Executor::MoreToDo(); }

    RayTracer1D* rt1d = new RayTracer1D( raysetup_ );
    rt1d->setModel( true, aim );
    rt1d->setOffsets( offsets_ );
    if ( !rt1d->execute() )
	mErrRet( rt1d->errMsg() );

    raytracers_ += rt1d;

    return MoreToDo();
}


const RayTracer1D* RayTracerRunner::rayTracer( int idx ) const
{ return raytracers_.validIdx(idx) ? raytracers_[idx] : 0; }
