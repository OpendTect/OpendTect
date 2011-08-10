/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y. Liu
 * DATE     : January 2010
-*/

static const char* rcsID = "$Id: prestackanglemute.cc,v 1.14 2011-08-10 15:03:51 cvsbruno Exp $";

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
#include "timedepthconv.h"
#include "velocityfunctionvolume.h"


using namespace PreStack;

AngleMuteBase::AngleMuteBase()
    : pars_(0)
{
    velsource_ = new Vel::VolumeFunctionSource();
    velsource_->ref();
}


AngleMuteBase::~AngleMuteBase()
{
    delete pars_;
    velsource_->unRef();
}


void AngleMuteBase::fillPar( IOPar& par ) const
{
    PtrMan <IOObj> ioobj = IOM().get( pars_->velvolmid_ );
    if ( ioobj ) par.set( sKeyVelVolumeID(), pars_->velvolmid_ );

    IOPar rtracepar;
    pars_->raysetup_.fillPar( rtracepar );
    par.mergeComp( rtracepar, sKeyRayTracer() );
    par.set( sKeyMuteCutoff(), pars_->mutecutoff_ );
    par.setYN( sKeyVelBlock(), pars_->dovelblock_ );
}


bool AngleMuteBase::usePar( const IOPar& par  ) 
{
    PtrMan<IOPar> rtracepar = par.subselect( sKeyRayTracer() );
    if ( !rtracepar || !pars_->raysetup_.usePar( *rtracepar ) )
	return false;

    par.get( sKeyVelVolumeID(), pars_->velvolmid_ );
    par.get( sKeyMuteCutoff(), pars_->mutecutoff_ );
    par.getYN( sKeyVelBlock(), pars_->dovelblock_ );

    return true;
}


bool AngleMuteBase::setVelocityFunction()
{
    const MultiID& mid = pars_->velvolmid_;

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
       return false;

    velsource_->setFrom( mid );
    return true;
}



bool AngleMuteBase::getLayers(const BinID& bid, 
				TypeSet<ElasticLayer>& layers, 
				SamplingData<float>& sd, int resamplesz )
{ 
    TypeSet<float> vels;
    RefMan<Vel::VolumeFunction> velfun = velsource_->createFunction( bid );
    if ( !velfun || !velsource_->getVel(bid,sd,vels) )
	return false;

    const int velsz = vels.size();
    if ( resamplesz > velsz )
    {
	for ( int idy=velsz; idy<resamplesz; idy++ )
	{
	    const float vel = velfun->getVelocity( sd.atIndex(idy) );
	    if ( mIsUdf(vel) )
		continue;

	    vels += vel;
	}
    }
    int nrlayers = vels.size();

    const StepInterval<float> zrg = velfun->getAvailableZ();
    TypeSet<float> depths;
    depths.setSize( nrlayers, 0 );
    
    if ( velsource_->zIsTime() )
    {
	RefMan<Time2DepthStretcher> t2dstretcher= new Time2DepthStretcher();
	if ( !t2dstretcher->setVelData(pars_->velvolmid_) )
	    return false;

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

    if ( pars_->dovelblock_ )
    {
	BendPointVelBlock( depths, vels );
	nrlayers = vels.size();
    }

    for ( int il=1; il<nrlayers; il++ )
	layers += ElasticLayer(depths[il]-depths[il-1], 
			vels[il-1], mUdf(float), mUdf(float) );

    return !layers.isEmpty();
}


float AngleMuteBase::getOffsetMuteLayer( const RayTracer1D& rt, int nrlayers, 
					    int ioff, bool tail ) const 
{
    float mutelayer = mUdf(float);
    const float cutoffsin = sin( pars_->mutecutoff_ * M_PI / 180 );
    if ( tail )
    {
	float prevsin = mUdf(float);
	int previdx = -1;
	for ( int il=0; il<nrlayers; il++ )
	{
	    const float sini = rt.getSinAngle(il,ioff);
	    if ( mIsUdf(sini) || (mIsZero(sini,1e-8) && il<nrlayers/2) )
		continue; //Ordered down, get rid of edge 0.

	    if ( sini<cutoffsin )
	    {
		if ( previdx != -1 && !mIsZero(sini-prevsin,1e-5) )
		{
		    mutelayer = previdx + (il-previdx)*
			(cutoffsin-prevsin)/(sini-prevsin);
		}
		else
		    mutelayer = il;
		break;
	    }

	    prevsin = sini;
	    previdx = il;
	}
    }
    else
    {
	float prevsin = mUdf(float);
	int previdx = -1;
	for ( int il=nrlayers-1; il>=0; il-- )
	{
	    const float sini = rt.getSinAngle(il,ioff);
	    if ( mIsUdf(sini) )
		continue;

	    if ( sini>cutoffsin ) 
	    {
		if ( previdx!=-1 && !mIsZero(sini-prevsin,1e-5) )
		{
		    mutelayer = previdx + (il-previdx) *
			(cutoffsin-prevsin)/(sini-prevsin);
		}
		else
		    mutelayer = il;
		break;
	    }

	    prevsin = sini;
	    previdx = il;
	}
    }
    return mutelayer;
}




AngleMute::AngleMute()
    : Processor( sFactoryKeyword() )
    , muter_( 0 )
{
    pars_ = new AngleMutePars();
}


AngleMute::~AngleMute()
{ 
    deepErase( rtracers_ ); 
    delete muter_;
}


bool AngleMute::doPrepare( int nrthreads )
{
    if ( !setVelocityFunction() )
	return false;

    raytraceparallel_ = nrthreads < Threads::getNrProcessors();

    if ( !muter_ ) 
	muter_ = new Muter( params().taperlen_, params().tail_ );

    for ( int idx=0; idx<nrthreads; idx++ )
    {
	rtracers_ += new RayTracer1D( RayTracer1D::Setup() );
	rtracers_[idx]->setSetup( pars_->raysetup_ );
    }
    return true;
}


bool AngleMute::usePar( const IOPar& par )
{
    if ( !AngleMuteBase::usePar( par ) )
	return false;

    par.get( Mute::sTaperLength(), params().taperlen_ );
    par.getYN( Mute::sTailMute(), params().tail_ );

    for ( int idx=0; idx<rtracers_.size(); idx++ )
	rtracers_[idx]->setSetup( pars_->raysetup_  );

    return true;
}


void AngleMute::fillPar( IOPar& par ) const
{
    AngleMuteBase::fillPar( par );
    par.set( Mute::sTaperLength(), params().taperlen_ );
    par.setYN( Mute::sTailMute(), params().tail_ );
}


bool AngleMute::doWork( od_int64 start, od_int64 stop, int thread )
{
    for ( int idx=start; idx<=stop; idx++, addToNrDone(1) )
    {
	Gather* output = outputs_[idx];
	const Gather* input = inputs_[idx];
	if ( !output || !input )
	    continue;


	const BinID bid = input->getBinID();

	int nrlayers = input->data().info().getSize( Gather::zDim() );
	TypeSet<ElasticLayer> layers; SamplingData<float> sd;
	if ( !getLayers( bid, layers, sd, nrlayers ) )
	    continue;
	
	TypeSet<float> offsets;
	const int nroffsets = input->size( input->offsetDim()==0 );
	for ( int ioffset=0; ioffset<nroffsets; ioffset++ )
	    offsets += input->getOffset( ioffset );
	rtracers_[thread]->setOffsets( offsets );
	rtracers_[thread]->setModel( layers );
	if ( !rtracers_[thread]->execute(raytraceparallel_) )
	    continue;

	Array1DSlice<float> trace( output->data() );
	trace.setDimMap( 0, Gather::zDim() );

	for ( int ioffs=0; ioffs<nroffsets; ioffs++ )
	{
	    trace.setPos( Gather::offsetDim(), ioffs );
	    if ( !trace.init() )
		continue;

	    const float mutelayer = getOffsetMuteLayer( *rtracers_[thread],
					    nrlayers, ioffs, params().tail_ );
	    if ( !mIsUdf( mutelayer ) )
		muter_->mute( trace, nrlayers, mutelayer );
	}
    }

    return true;
}


AngleMute::AngleMutePars& AngleMute::params()
{ return static_cast<AngleMute::AngleMutePars&>(*pars_); }


const AngleMute::AngleMutePars& AngleMute::params() const
{ return static_cast<AngleMute::AngleMutePars&>(*pars_); }

