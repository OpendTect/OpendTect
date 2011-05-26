/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: raytracerrunner.cc,v 1.3 2011-05-26 15:42:47 cvsbruno Exp $";


#include "raytracerrunner.h"

RayTracerRunner::RayTracerRunner( const TypeSet<AIModel>& aims, 
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

    const AIModel& aim = aimodels_[nrdone_];
    if ( aim.isEmpty() )
	{ nrdone_ ++; return Executor::MoreToDo(); }

    RayTracer1D* rt1d = new RayTracer1D( raysetup_ );
    rt1d->setModel( true, aim );
    rt1d->setOffsets( offsets_ );
    if ( !rt1d->execute() )
	mErrRet( rt1d->errMsg() );

    raytracers_ += rt1d;

    nrdone_ ++;
    return MoreToDo();
}

