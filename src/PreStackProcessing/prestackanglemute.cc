/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y. Liu
 * DATE     : January 2010
-*/

static const char* rcsID = "$Id: prestackanglemute.cc,v 1.7 2011-02-01 20:55:46 cvsyuancheng Exp $";

#include "prestackanglemute.h"

#include "ailayer.h"
#include "arrayndslice.h"
#include "flatposdata.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "math.h"
#include "muter.h"
#include "prestackgather.h"
#include "prestackmute.h"
#include "raytrace1d.h"
#include "timedepthconv.h"
#include "velocityfunctionvolume.h"


using namespace PreStack;

AngleMute::AngleMute()
    : Processor( sFactoryKeyword() )
    , velvolmid_( 0 )
    , muter_( 0 )
    , tail_( false )
    , taperlen_( 10 )
    , mutecutoff_( 0 )		   
    , setup_( RayTracer1D::Setup() )				   
{
    velsource_ = new Vel::VolumeFunctionSource();
    velsource_->ref();
}


AngleMute::~AngleMute()
{ 
    deepErase( rtracers_ ); 

    delete muter_;
    velsource_->unRef();
}


bool AngleMute::doPrepare( int nrthreads )
{
    raytraceparallel_ = nrthreads < Threads::getNrProcessors();

    if ( !muter_ ) 
	muter_ = new Muter( taperlen_, tail_ );

    for ( int idx=0; idx<nrthreads; idx++ )
    {
	rtracers_ += new RayTracer1D( RayTracer1D::Setup() );
	rtracers_[idx]->setSetup( setup_ );
    }

    return true;
}


void AngleMute::setTailMute( bool yn )
{ tail_ = yn; delete muter_; muter_ = 0; }


void AngleMute::setTaperLength( float l )
{ taperlen_ = l; delete muter_; muter_ = 0; }


bool AngleMute::setVelocityMid( const MultiID& mid )
{
    velvolmid_ = mid;

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
       return false;

    velsource_->setFrom( mid );
    return true;
}


void AngleMute::setSetup( const RayTracer1D::Setup& ns )
{
    setup_ = ns;
    for ( int idx=0; idx<rtracers_.size(); idx++ )
	rtracers_[idx]->setSetup( ns );
}


void AngleMute::fillPar( IOPar& par ) const
{
    PtrMan <IOObj> ioobj = IOM().get( velvolmid_ );
    if ( ioobj ) par.set( sKeyVelVolumeID(), velvolmid_ );

    IOPar rtracepar;
    setup_.fillPar( rtracepar );
    par.mergeComp( rtracepar, sKeyRayTracer() );

    par.set( Mute::sTaperLength(), taperlen_ );
    par.setYN( Mute::sTailMute(), tail_ );
    par.set( sKeyMuteCutoff(), mutecutoff_ );
}


bool AngleMute::usePar( const IOPar& par )
{
    PtrMan<IOPar> rtracepar = par.subselect( sKeyRayTracer() );
    if ( !rtracepar )
	return false;

    if ( !setup_.usePar( *rtracepar ) )
	return false;

    for ( int idx=0; idx<rtracers_.size(); idx++ )
	rtracers_[idx]->setSetup( setup_ );

    par.get( sKeyVelVolumeID(), velvolmid_ );
    par.get( sKeyMuteCutoff(), mutecutoff_ );
    par.get( Mute::sTaperLength(), taperlen_ );
    par.getYN( Mute::sTailMute(), tail_ );

    return true;
}


bool AngleMute::doWork( od_int64 start, od_int64 stop, int thread )
{
    for ( int idx=start; idx<=stop; idx++, addToNrDone(1) )
    {
	Gather* output = outputs_[idx];
	const Gather* input = inputs_[idx];
	if ( !output || !input )
	    continue;
	
	const int nrlayers = input->data().info().getSize( Gather::zDim() );
	
	const BinID bid = input->getBinID();
	RefMan<Vel::VolumeFunction> velfun = velsource_->createFunction( bid );
	if ( !velfun )
	    continue;

	SamplingData<float> sd;
	TypeSet<float> vels;
	velsource_->getVel( bid, sd, vels );
	const int velsz = vels.size();
	if ( nrlayers > velsz )
	{
    	    for ( int idy=velsz; idy<nrlayers; idy++ )
		vels += velfun->getVelocity( sd.atIndex(idy) );
	}
	
	const StepInterval<float> zrg = velfun->getAvailableZ();
	TypeSet<float> depths;
	depths.setSize( nrlayers, 0 );
	
	if ( velsource_->zIsTime() )
	{
	    RefMan<Time2DepthStretcher> t2dstretcher= new Time2DepthStretcher();
	    if ( !t2dstretcher->setVelData(velvolmid_) )
		continue;

	    CubeSampling cs;
	    cs.hrg.start = cs.hrg.stop = bid; 
	    cs.zrg = zrg;
	    t2dstretcher->addVolumeOfInterest( cs, false);
	    t2dstretcher->loadDataIfMissing( 0, 0 );
	    t2dstretcher->transform( bid, sd, nrlayers, depths.arr() );
	}
	else
	{
	    for ( int il=0; il<nrlayers; il++ )
		depths[il] = zrg.atIndex( il );
	}

	TypeSet<float> offsets;
	const int nroffsets = input->size( input->offsetDim()==0 );
    	for ( int ioffset=0; ioffset<nroffsets; ioffset++ )
    	    offsets += input->getOffset( ioffset );
	rtracers_[thread]->setOffsets( offsets );
	
	TypeSet<AILayer> layers;
	for ( int il=0; il<nrlayers; il++ )
	    layers += AILayer(  depths[il], vels[il], mUdf(float) );

	rtracers_[thread]->setModel( true, layers );
	if ( !rtracers_[thread]->execute(raytraceparallel_) )
	    continue;

	Array1DSlice<float> slice( output->data() );
	slice.setDimMap( 0, Gather::zDim() );

	const float cutoffsin = sin( mutecutoff_*M_PI/180);
	for ( int ioffs=0; ioffs<nroffsets; ioffs++ )
	{
	    slice.setPos( Gather::offsetDim(), ioffs );
	    if ( !slice.init() )
		continue;

	    bool found = false;
	    StepInterval<float> vrg;
	    for ( int il=0; il<nrlayers; il++ )
	    {
		const float sini = rtracers_[thread]->getSinAngle( il, ioffs );
		if ( mIsUdf(sini) )
		    continue;

		if ( !found )
		{
		    found = true;
		    vrg.start = vrg.stop = sini;
		}
		else
		    vrg.include( sini );
	    }

	    if ( !found || vrg.width()<1e-8 )
		continue;

	    vrg.step = vrg.width() / nrlayers;

	    const float mutepos = 
		Muter::mutePos( cutoffsin, SamplingData<float>(vrg) );
	    muter_->mute( slice, nrlayers, mutepos );
	}
    }

    return true;
}
