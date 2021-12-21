/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/


#include "raytracerrunner.h"

#include "ailayer.h"
#include "iopar.h"


RayTracerRunner::RayTracerRunner( const char* rt1dfactkeywd )
    : ParallelTask("Raytracing")
    , raypar_(*new IOPar())
{
    const Factory<RayTracer1D>& rt1dfact = RayTracer1D::factory();
    if ( rt1dfact.isEmpty() )
	return;

    if ( rt1dfact.hasName(rt1dfactkeywd) )
    {
	raypar_.set( sKey::Type(), rt1dfactkeywd );
	return;
    }

    const BufferStringSet& factnms = rt1dfact.getNames();
    if ( !factnms.isEmpty() )
    {
	const FixedString defnm( rt1dfact.getDefaultName() );
	if ( !defnm.isEmpty() )
	    raypar_.set( sKey::Type(), defnm.str() );
    }
}


RayTracerRunner::RayTracerRunner( const IOPar& raypars )
    : ParallelTask("Raytracing")
    , raypar_(*new IOPar(raypars))
{
}


RayTracerRunner::RayTracerRunner( const TypeSet<ElasticModel>& aims,
				  const IOPar& raypars )
    : RayTracerRunner(raypars)
{
    setModel( aims );
}


RayTracerRunner::~RayTracerRunner()
{
    deepErase( raytracers_ );
    delete &raypar_;
}


od_int64 RayTracerRunner::nrIterations() const
{
    return totalnr_;
}


uiString RayTracerRunner::uiNrDoneText() const
{
    return tr("Layers done");
}


od_int64 RayTracerRunner::nrDone() const
{
    od_int64 nrdone = 0;
    for ( const auto* rt1d : raytracers_ )
	nrdone += rt1d->nrDone();
    return nrdone;
}


void RayTracerRunner::setOffsets( const TypeSet<float>& offsets )
{
    raypar_.set( RayTracer1D::sKeyOffset(), offsets );
}


void RayTracerRunner::setModel( const TypeSet<ElasticModel>& aimodels )
{
    aimodels_ = &aimodels;
    computeTotalNr();
}


void RayTracerRunner::computeTotalNr()
{
    if ( !aimodels_ )
	return;

    totalnr_ = 0;
    for ( const auto& aimodel : *aimodels_ )
	totalnr_ += aimodel.size();
}


#define mErrRet(msg) { msg_ = msg; return false; }

bool RayTracerRunner::doPrepare( int /* nrthreads */ )
{
    msg_ = tr("Preparing Reflectivity Model");
    deepErase( raytracers_ );

    if ( !aimodels_ || aimodels_->isEmpty() )
	mErrRet( tr("No Elastic models set") );

    if ( RayTracer1D::factory().getNames().isEmpty() )
	mErrRet( tr("RayTracer factory is empty") )

    computeTotalNr();
    uiString errmsg;
    for ( const auto& aimodel : *aimodels_ )
    {
	RayTracer1D* rt1d = RayTracer1D::createInstance( raypar_, &aimodel,
							 errmsg );
	if ( !rt1d )
	{
	    uiString msg = tr( "Wrong input for raytracing on model: %1" )
				.arg(aimodels_->indexOf(aimodel)+1);
	    msg.append( errmsg, true );
	    mErrRet( msg );
	}

	raytracers_ += rt1d;
    }

    return true;
}


bool RayTracerRunner::doWork( od_int64 start, od_int64 stop, int threadidx )
{
    const bool parallel = start == 0 && threadidx == 0 &&
		(stop == nrIterations()-1);

    const TypeSet<ElasticModel>& models = *aimodels_;

    bool startlayer = false;
    int startmdlidx = modelIdx( start, startlayer );
    if ( !startlayer ) startmdlidx++;
    const int stopmdlidx = modelIdx( stop, startlayer );
    for ( int idx=startmdlidx; idx<=stopmdlidx; idx++ )
    {
	if ( models[idx].isEmpty() )
	    continue;

	ParallelTask* rt1d = raytracers_[idx];
	if ( !rt1d->executeParallel(!parallel) )
	    mErrRet( rt1d->uiMessage() );
    }
    return true;
}


bool RayTracerRunner::doFinish( bool success )
{
    if ( !success )
	deepErase( raytracers_ );

    return success;
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

