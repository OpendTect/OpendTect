/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y. Liu
 * DATE     : January 2010
-*/

static const char* rcsID = "$Id: prestackanglemute.cc,v 1.1 2011-01-26 23:10:42 cvsyuancheng Exp $";

#include "prestackanglemute.h"
#include "prestackmute.h"

#include "arrayndslice.h"
#include "flatposdata.h"
#include "prestackgather.h"
#include "raytrace1d.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "muter.h"
#include "velocityfunctionvolume.h"


using namespace PreStack;

AngleMute::AngleMute()
    : Processor( sFactoryKeyword() )
    , velvolmid_( 0 )
    , muter_( 0 )
    , tail_( false )
    , taperlen_( 10 )
    , mutecutoff_( 0 )		    
{
    rtracer_ = new AngleRayTracer;
    velsource_ = new Vel::VolumeFunctionSource();
    velsource_->ref();
}


AngleMute::~AngleMute()
{ 
    delete rtracer_; 
    delete muter_;
    velsource_->unRef();
}


bool AngleMute::prepareWork()
{
    if ( !Processor::prepareWork() )
	return false;

    if ( muter_ ) delete muter_;
    muter_ = new Muter( taperlen_, tail_ );

    return true;
}


bool AngleMute::doPrepare( int nrthreads )
{
    raytraceparallel_ = nrthreads<Threads::getNrProcessors();
    return true;
}


bool AngleMute::setVelocityMid( const MultiID& mid )
{
    velvolmid_ = mid;

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
       return false;

    velsource_->setFrom( mid );
    return true;
}


void AngleMute::fillPar( IOPar& par ) const
{
    PtrMan <IOObj> ioobj = IOM().get( velvolmid_ );
    if ( ioobj ) par.set( sVelVolumeID(), velvolmid_ );

    par.set( sSourceDepth(), rtracer_->sourceDepth() );
    par.set( sReceiverDepth(), rtracer_->receiverDepth() );
    par.set( Mute::sTaperLength(), taperlen_ );
    par.setYN( Mute::sTailMute(), tail_ );
    par.set( sMuteCutoff(), mutecutoff_ );
}


bool AngleMute::usePar( const IOPar& par )
{
    float sourcedepth, receiverdepth;
    par.get( sSourceDepth(), sourcedepth );
    par.get( sReceiverDepth(), receiverdepth );
    rtracer_->setSourceDepth( sourcedepth );
    rtracer_->setReceiverDepth( receiverdepth );

    par.get( sVelVolumeID(), velvolmid_ );
    par.get( sMuteCutoff(), mutecutoff_ );
    par.get( Mute::sTaperLength(), taperlen_ );
    par.getYN( Mute::sTailMute(), tail_ );

    return true;
}


bool AngleMute::doWork( od_int64 start, od_int64 stop, int )
{
    for ( int idx=start; idx<=stop; idx++, addToNrDone(1) )
    {
	Gather* output = outputs_[idx];
	const Gather* input = inputs_[idx];
	if ( !output || !input )
	    continue;

	const BinID bid = input->getBinID();
	const int nroffsets = input->size( input->offsetDim()==0 );
	const int nrlayers = input->data().info().getSize( Gather::zDim() );

	TypeSet<float> offsets;
    	for ( int ioffset=0; ioffset<nroffsets; ioffset++ )
    	    offsets += input->getOffset( ioffset );
	rtracer_->setOffsets( offsets );

	SamplingData<float> sd;
	TypeSet<float> vels;
	velsource_->getVel( bid, sd, vels );
	const int velsz = vels.size();
	if ( nrlayers > velsz )
	{
	    Vel::VolumeFunction* velfun = velsource_->createFunction( bid );
    	    for ( int idy=velsz; idy<nrlayers; idy++ )
		vels += velfun->getVelocity( sd.atIndex(idy) );
	}

	ObjectSet<RayTracer1D::Layer> layers;
	for ( int il=0; il<nrlayers; il++ )
	{
	    RayTracer1D::Layer layer; 
	    layer.d0_ = sd.atIndex( il ); 
	    layer.Vint_ = vels[il];
	    layers += &layer;
	}

	rtracer_->setModel( true, layers, OD::CopyPtr );
	if ( !rtracer_->execute(raytraceparallel_) )
	    continue;

	Array1DSlice<float> slice( output->data() );
	slice.setDimMap( 0, Gather::zDim() );

	for ( int ioffs=0; ioffs<nroffsets; ioffs++ )
	{
	    slice.setPos( Gather::offsetDim(), ioffs );
	    if ( !slice.init() )
		continue;

	    bool found = false;
	    StepInterval<float> vrg;
	    for ( int il=0; il<nrlayers; il++ )
	    {
		const float sini = rtracer_->getSinAngle( il, ioffs );
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
	    SamplingData<float> vsd( vrg );

	    const int siniidx = tail_ ? 0 : nrlayers-1;
	    const float mutepos = Muter::mutePos( 
		    rtracer_->getSinAngle(siniidx,ioffs), vsd );
	    muter_->mute( slice, nrlayers, mutepos );

	    const int endidx = mutepos < 0 ? (int)mutepos - 1 : (int)mutepos;
	    if ( tail_ )
	    {
		for ( int idy=endidx+1; idy<nrlayers; idy++ )
		    slice.setValue( idy, mutecutoff_ );

	    }
	    else
	    {
		for ( int idy=0; idy<=endidx; idy++ )
		    slice.setValue( idy, mutecutoff_ );
	    }

	    //for ( int il=0; il<nrlayers; il++ )
		//output->data().set(ioffs,il,rtracer_->getSinAngle(il,ioffs));
	}
    }

    return true;
}
