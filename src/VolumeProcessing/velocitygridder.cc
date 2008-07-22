/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : October 2006
-*/

static const char* rcsID = "$Id: velocitygridder.cc,v 1.1 2008-07-22 19:44:22 cvskris Exp $";

#include "velocitygridder.h"

#include "attribdatacubes.h"
#include "iopar.h"
#include "velocityfunction.h"
#include "velocityfunctiongrid.h"

namespace VolProc
{


void VelGriddingStep::initClass()
{
    VolProc::PS().addCreator( create, VelGriddingStep::sType() );
}


VolProc::Step* VelGriddingStep::create( VolProc::Chain& vr )
{
    mDeclareAndTryAlloc( VolProc::Step*, res, VelGriddingStep( vr ) );
    return res;
}


VelGriddingStep::VelGriddingStep( VolProc::Chain& vr )
    :VolProc::Step( vr )
{
    mTryAlloc( velfuncsource_, Vel::GriddedSource );
    if ( velfuncsource_ )
	velfuncsource_->ref();
}


VelGriddingStep::~VelGriddingStep()
{
    velfuncsource_->unRef();
    deepUnRef( velfuncs_ );
}


const VelocityDesc& VelGriddingStep::outputVelocityType() const
{
    return velfuncsource_->getDesc();
}


void VelGriddingStep::setPicks( ObjectSet<Vel::FunctionSource>& nvfs )
{ velfuncsource_->setSource( nvfs ); }


void VelGriddingStep::setPicks( const TypeSet<MultiID>& mids )
{ velfuncsource_->setSource( mids ); }


void VelGriddingStep::getPicks(TypeSet<MultiID>& mids ) const
{
    velfuncsource_->getSources( mids );
}


void VelGriddingStep::setGridder( Gridder2D* gridder )
{ velfuncsource_->setGridder( gridder ); }


const Gridder2D* VelGriddingStep::getGridder() const
{ return velfuncsource_->getGridder(); }


const ObjectSet<Vel::FunctionSource>& VelGriddingStep::getPicks() const
{
    return velfuncsource_->getSources();
}


bool VelGriddingStep::needsInput(const HorSampling&) const
{ return false; }


bool  VelGriddingStep::prepareComp( int nrthreads )
{
    if ( !nrthreads )
	return false;

   deepUnRef( velfuncs_ );
   if ( !velfuncsource_ )
   {
       mTryAlloc( velfuncsource_, Vel::GriddedSource );
       velfuncsource_->ref();
   }

   for ( int idx=0; idx<nrthreads; idx++ )
   {
       RefMan<Vel::Function> velfunc = velfuncsource_->createFunction();
       velfunc->ref();
       velfuncs_ +=velfunc;
   }

   return true;
}


bool VelGriddingStep::computeBinID( const BinID& bid, int threadid )
{
    RefMan<Vel::Function> velfunc = velfuncs_[threadid];
    if ( !velfunc->moveTo( bid ) )
	velfunc = 0;

    const int inlidx = output_->inlsampling.nearestIndex( bid.inl );
    const int crlidx = output_->crlsampling.nearestIndex( bid.crl );
    
    const bool zit = chain_.zIsT();
    const int zsz = output_->getZSz();
    StepInterval<float> zrg( chain_.getZSampling().start,
	    		     chain_.getZSampling().atIndex(zsz-1),
		 	     chain_.getZSampling().step );

    for ( int idx=0; idx<zsz; idx++ )
    {
	if ( velfunc ) velfunc->setDesiredZRange( zrg );
	const float z = (output_->z0+idx) * output_->zstep;
	const float vel = velfunc ? velfunc->getVelocity( z ) : mUdf(float);

	output_->setValue( 0, inlidx, crlidx, idx, vel );
    }

    return true;
}


void VelGriddingStep::fillPar( IOPar& par ) const
{
    const ObjectSet<Vel::FunctionSource>& sources =
					velfuncsource_->getSources();

    par.set( sKeyNrSources(), sources.size() );
    for ( int idx=0; idx<sources.size(); idx++ )
    {
	IOPar sourcepar;
	sourcepar.set( sKeyType(), sources[idx]->type() );
	sourcepar.set( sKeyID(), sources[idx]->multiID() );
	sources[idx]->fillPar(sourcepar);

	const BufferString idxstr = idx;
	par.mergeComp( sourcepar, idxstr.buf() );
    }

    velfuncsource_->fillPar( par );
}


bool VelGriddingStep::usePar( const IOPar& par )
{
    int nrsources;
    if ( !par.get( sKeyNrSources(), nrsources ) )
	return false;

    ObjectSet<Vel::FunctionSource> sources;
    for ( int idx=0; idx<nrsources; idx++ )
    {
	const BufferString idxstr = idx;
	PtrMan<IOPar> sourcepar = par.subselect( idxstr.buf() );
	if ( !sourcepar )
	    continue;

	BufferString sourcetype;
	if ( !sourcepar->get( sKeyType(), sourcetype ) )
	    continue;

	MultiID mid;
	if ( !sourcepar->get( sKeyID(), mid ) )
	    continue;

	Vel::FunctionSource* source =
	    Vel::FunctionSource::factory().create( sourcetype.buf(), mid );
	if ( !source )
	    continue;

	source->ref();

	if ( !source->usePar( *sourcepar ) )
	{
	    source->unRef();
	    continue;
	}

	sources += source;
    }

    if ( !sources.size() )
	return false;

    velfuncsource_->setSource( sources );

    if ( !velfuncsource_->usePar( par ) ) 
	return false;

    return true;
}


}; //Namespace
