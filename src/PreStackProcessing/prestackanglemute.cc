/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y. Liu
 * DATE     : January 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "prestackanglemute.h"

#include "ailayer.h"
#include "arrayndslice.h"
#include "flatposdata.h"
#include "ioman.h"
#include "ioobj.h"
#include "math.h"
#include "muter.h"
#include "prestackgather.h"
#include "prestackmute.h"
#include "raytrace1d.h"
#include "raytracerrunner.h"
#include "timedepthconv.h"
#include "velocityfunctionvolume.h"


using namespace PreStack;

AngleMuteBase::AngleMuteBase()
    : params_(0)
{
    velsource_ = new Vel::VolumeFunctionSource();
    velsource_->ref();
}


AngleMuteBase::~AngleMuteBase()
{
    delete params_;
    deepErase( rtrunners_ ); 
    velsource_->unRef();
}


void AngleMuteBase::fillPar( IOPar& par ) const
{
    PtrMan <IOObj> ioobj = IOM().get( params_->velvolmid_ );
    if ( ioobj ) par.set( sKeyVelVolumeID(), params_->velvolmid_ );

    IOPar rtracepar;
    par.merge( params_->raypar_ );
    par.set( sKeyMuteCutoff(), params_->mutecutoff_ );
}


bool AngleMuteBase::usePar( const IOPar& par  ) 
{
    params_->raypar_.merge( par );
    par.get( sKeyVelVolumeID(), params_->velvolmid_ );
    par.get( sKeyMuteCutoff(), params_->mutecutoff_ );

    return true;
}


bool AngleMuteBase::setVelocityFunction()
{
    const MultiID& mid = params_->velvolmid_;

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
	if ( !t2dstretcher->setVelData(params_->velvolmid_) )
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

    bool doblock = false; float blockratiothreshold;
    params_->raypar_.getYN( RayTracer1D::sKeyBlock(), doblock );
    params_->raypar_.get( RayTracer1D::sKeyBlockRatio(), blockratiothreshold );
/*    if ( doblock )
    {
	BendPointVelBlock( depths, vels, blockrelthreshold );
	nrlayers = vels.size();
    }
    Should call BlockElasticModel as discussed with Aneesh */

    int il = 1;
    for ( il=1; il<nrlayers; il++ )
	layers += ElasticLayer(depths[il]-depths[il-1], 
			vels[il], mUdf(float), mUdf(float) );
    layers += ElasticLayer(depths[il-1]-depths[il-2], 
			vels[il-1], mUdf(float), mUdf(float) );

    sd = zrg;
    return !layers.isEmpty();
}


float AngleMuteBase::getOffsetMuteLayer( const RayTracer1D& rt, int nrlayers, 
					int ioff, bool tail, int startlayer, 
					bool belowcutoff ) const 
{
    float mutelayer = mUdf(float);
    const float cutoffsin = (float) sin( params_->mutecutoff_ * M_PI / 180 );
    if ( tail )
    {
	float prevsin = mUdf(float);
	int previdx = -1;
	for ( int il=startlayer; il<nrlayers; il++ )
	{
	    const float sini = rt.getSinAngle(il,ioff);
	    if ( mIsUdf(sini) || (mIsZero(sini,1e-8) && il<nrlayers/2) )
		continue; //Ordered down, get rid of edge 0.

	    bool ismuted = ( sini < cutoffsin && belowcutoff ) || 
				( sini > cutoffsin && !belowcutoff );
	    if ( ismuted )
	    {
		if ( previdx != -1 && !mIsZero(sini-prevsin,1e-5) )
		{
		    mutelayer = previdx + (il-previdx)*
			(cutoffsin-prevsin)/(sini-prevsin);
		}
		else
		    mutelayer = (float)il;
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
	for ( int il=nrlayers-1; il>=startlayer; il-- )
	{
	    const float sini = rt.getSinAngle(il,ioff);
	    if ( mIsUdf(sini) )
		continue;

	    bool ismuted = ( sini > cutoffsin && belowcutoff ) || 
				( sini < cutoffsin && !belowcutoff );
	    if ( ismuted ) 
	    {
		if ( previdx!=-1 && !mIsZero(sini-prevsin,1e-5) )
		{
		    mutelayer = previdx + (il-previdx) *
			(cutoffsin-prevsin)/(sini-prevsin);
		}
		else
		    mutelayer = (float)il;
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
{
    params_ = new AngleMutePars();
}


AngleMute::~AngleMute()
{ 
    deepErase( muters_ );
}


bool AngleMute::doPrepare( int nrthreads )
{
    deepErase( rtrunners_ );
    deepErase( muters_ );

    if ( !setVelocityFunction() )
	return false;

    raytraceparallel_ = nrthreads < Threads::getNrProcessors();

    for ( int idx=0; idx<nrthreads; idx++ )
    {
	muters_ += new Muter( params().taperlen_, params().tail_ );
	rtrunners_ += new RayTracerRunner( params().raypar_ );
    }

    return true;
}


bool AngleMute::usePar( const IOPar& par )
{
    if ( !AngleMuteBase::usePar( par ) )
	return false;

    par.get( Mute::sTaperLength(), params().taperlen_ );
    par.getYN( Mute::sTailMute(), params().tail_ );

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
    for ( int idx=mCast(int,start); idx<=stop; idx++, addToNrDone(1) )
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

	const int nrblockedlayers = layers.size();	
	TypeSet<float> offsets;
	const int nroffsets = input->size( input->offsetDim()==0 );
	for ( int ioffset=0; ioffset<nroffsets; ioffset++ )
	    offsets += input->getOffset( ioffset );

	rtrunners_[thread]->setOffsets( offsets );
	rtrunners_[thread]->addModel( layers, true );
	if ( !rtrunners_[thread]->execute(raytraceparallel_) )
	    { errmsg_ = rtrunners_[thread]->errMsg(); continue; }

	Array1DSlice<float> trace( output->data() );
	trace.setDimMap( 0, Gather::zDim() );

	for ( int ioffs=0; ioffs<nroffsets; ioffs++ )
	{
	    trace.setPos( Gather::offsetDim(), ioffs );
	    if ( !trace.init() )
		continue;

	    float mutelayer = 
		    getOffsetMuteLayer( *rtrunners_[thread]->rayTracers()[0],
	    nrblockedlayers, ioffs, params().tail_ );
	    if ( mIsUdf( mutelayer ) )
		continue;

	    if ( nrlayers != nrblockedlayers )
	    {
		const int muteintlayer = (int)mutelayer;
		if ( input->zIsTime() )
		{
		    float mtime = 0;
		    for ( int il=0; il<=muteintlayer; il++ )
		    {
			mtime += layers[il].thickness_/layers[il].vel_;
			if ( il==muteintlayer )
			{
			    const float diff = mutelayer-muteintlayer;
			    if ( diff>0 )
				mtime += diff*
				    layers[il+1].thickness_/layers[il+1].vel_;
			}
		    }
		    mutelayer = sd.getfIndex( mtime );
		}
		else
		{
		    float depth = 0;
		    for ( int il=0; il<muteintlayer+2; il++ )
		    {
			if ( il >= nrblockedlayers ) break;
			float thk = layers[il].thickness_;
			if ( il == muteintlayer+1 )
			    thk *= ( mutelayer - muteintlayer);
			
			depth += thk;
		    }
		    mutelayer = sd.getfIndex( depth );
		}
	    }
	    muters_[thread]->mute( trace, nrlayers, mutelayer );
	}
    }

    return true;
}


AngleMute::AngleMutePars& AngleMute::params()
{ return static_cast<AngleMute::AngleMutePars&>(*params_); }


const AngleMute::AngleMutePars& AngleMute::params() const
{ return static_cast<AngleMute::AngleMutePars&>(*params_); }

