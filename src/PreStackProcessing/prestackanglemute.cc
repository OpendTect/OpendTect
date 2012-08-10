/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y. Liu
 * DATE     : January 2010
-*/

static const char* rcsID mUnusedVar = "$Id: prestackanglemute.cc,v 1.24 2012-08-10 04:11:24 cvssalil Exp $";

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

    bool dovelblock = false; float blockthreshold;
    params_->raypar_.getYN( RayTracer1D::sKeyVelBlock(), dovelblock );
    params_->raypar_.get( RayTracer1D::sKeyVelBlockVal(), blockthreshold );
    if ( dovelblock )
    {
	BendPointVelBlock( depths, vels, blockthreshold );
	nrlayers = vels.size();
    }

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
		    mutelayer = il;
		break;
	    }

	    prevsin = sini;
	    previdx = il;
	}
    }
    return mutelayer;
}


float AngleMuteBase::getOffsetMuteLayer( const RayTracer1D& rt, int nrlayers, 
					    int ioff, bool tail ) const 
{
    return getOffsetMuteLayer( rt, nrlayers, ioff, tail, 0, true );
}


void AngleMuteBase::getOffsetMuteLayers( const RayTracer1D& rt, int nrlayers, 
					int ioff, bool tail, 
					TypeSet< Interval<float> >& res ) const 
{
    int lid = 0;
    while ( true )
    {
	Interval<float> ires( mUdf(float), mUdf(float) );
	ires.start = getOffsetMuteLayer( rt, nrlayers, ioff, tail, lid, true );
        if ( mIsUdf( ires.start ) )
	    break;

	res += ires;

	lid = (int)ires.start;
	if ( lid <= 0 || lid >= nrlayers )
	    break;

	lid ++;
	ires.stop = getOffsetMuteLayer( rt, nrlayers, ioff, tail, lid, false );
        if ( mIsUdf( ires.stop ) )
	   break;
    }
}



AngleMute::AngleMute()
    : Processor( sFactoryKeyword() )
    , muter_( 0 )
{
    params_ = new AngleMutePars();
}


AngleMute::~AngleMute()
{ 
    delete muter_;
}


bool AngleMute::doPrepare( int nrthreads )
{
    deepErase( rtrunners_ );

    if ( !setVelocityFunction() )
	return false;

    raytraceparallel_ = nrthreads < Threads::getNrProcessors();

    if ( !muter_ ) 
	muter_ = new Muter( params().taperlen_, params().tail_ );

    for ( int idx=0; idx<nrthreads; idx++ )
	rtrunners_ += new RayTracerRunner( params().raypar_ );

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

	    TypeSet< Interval<float> > mutelayeritvs;
	    getOffsetMuteLayers( *rtrunners_[thread]->rayTracers()[0],
				nrlayers, ioffs, params().tail_, mutelayeritvs);
	    if ( nrlayers != nrblockedlayers )
	    {
		float depth = 0;
		for ( int iml=0; iml<mutelayeritvs.size(); iml ++ )
		{
		    Interval<float>& itvml = mutelayeritvs[iml];
		    float startdpt; float stopdpt;
		    const float startml = itvml.start;
		    const float stopml = itvml.stop;
		    for ( int il=0; il<mMAX((int)start+2,(int)stopml+2); il++ )
		    {
			if ( il >= layers.size() ) break;
			float thk = layers[il].thickness_;
			if ( !mIsUdf(startml) && il == (int)startml+1 )
			{
			    const float dlayerstart = startml - (int)startml;
			    startdpt =depth + thk*dlayerstart;
			}
			if ( !mIsUdf(stopml) && il == (int)stopml+1 )
			{
			    const float dlayerstop = stopml - (int)stopml;
			    stopdpt = depth + thk*dlayerstop;
			}

			depth += thk;
		    }
		    itvml.start = sd.getfIndex( startdpt );
		    itvml.stop = sd.getfIndex( stopdpt );
		}
	    }

	    muter_->muteIntervals( trace, nrlayers, mutelayeritvs );
	}
    }

    return true;
}


AngleMute::AngleMutePars& AngleMute::params()
{ return static_cast<AngleMute::AngleMutePars&>(*params_); }


const AngleMute::AngleMutePars& AngleMute::params() const
{ return static_cast<AngleMute::AngleMutePars&>(*params_); }

