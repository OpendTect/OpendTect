/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocityfunctionvolume.h"

#include "binidvalset.h"
#include "idxable.h"
#include "ioman.h"
#include "posinfo.h"
#include "seisread.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "seistrctr.h"
#include "survinfo.h"


namespace Vel
{


void VolumeFunctionSource::initClass()
{ FunctionSource::factory().addCreator( create, sFactoryKeyword() ); }



VolumeFunction::VolumeFunction( VolumeFunctionSource& source )
    : Function( source )
    , extrapolate_( false )
    , statics_( 0 )
    , staticsvel_( 0 )
{}


VolumeFunction::~VolumeFunction()
{}


bool VolumeFunction::moveTo( const BinID& bid )
{
    if ( !Function::moveTo( bid ) )
	return false;

    mDynamicCastGet( VolumeFunctionSource&, source, source_ );
    if ( !source.getVel( bid, velsampling_, vel_ ) )
    {
	vel_.erase();
	return false;
    }

    return true;
}


void VolumeFunction::enableExtrapolation( bool yn )
{
    extrapolate_ = yn;
    removeCache();
}


void VolumeFunction::setStatics( float time, float vel )
{
    statics_ = time;
    staticsvel_ = vel;
    removeCache();
}


StepInterval<float> VolumeFunction::getAvailableZ() const
{
    if ( extrapolate_ )
	return SI().zRange(true);

    return getLoadedZ();
}


StepInterval<float> VolumeFunction::getLoadedZ() const
{
    return StepInterval<float>( velsampling_.start,
				velsampling_.atIndex(vel_.size()-1),
			        velsampling_.step );
}


bool VolumeFunction::computeVelocity( float z0, float dz, int nr,
				      float* res ) const
{
    const int velsz = vel_.size();

    if ( !velsz )
	return false;

    mDynamicCastGet( VolumeFunctionSource&, source, source_ );
    const SamplingData<double> velsampling =
					getDoubleSamplingData( velsampling_ );
    const SamplingData<double> sd = getDoubleSamplingData(
					SamplingData<float>(z0,dz) );

    if ( mIsEqual(z0,velsampling_.start,1e-5) &&
	 mIsEqual(velsampling_.step,dz,1e-5) &&
	 velsz==nr )
	OD::sysMemCopy( res, vel_.arr(), sizeof(float)*velsz );
    else if ( source.getDesc().type_!=VelocityDesc::RMS ||
	      !extrapolate_ ||
	      velsampling.atIndex(velsz-1) > sd.atIndex(nr-1) )
    {
	for ( int idx=0; idx<nr; idx++ )
	{
	    const float z = mCast(float,sd.atIndex( idx ));
	    const float sample = velsampling.getfIndex( z );
	    if ( sample<0 )
	    {
		res[idx] = extrapolate_ ? vel_[0] : mUdf(float);
		continue;
	    }

	    if ( sample<velsz )
	    {
		res[idx] = IdxAble::interpolateReg<const float*>( vel_.arr(),
			velsz, sample, false );
		continue;
	    }

	    //sample>=vel_.size()
	    if ( !extrapolate_ )
	    {
		res[idx] = mUdf(float);
		continue;
	    }

	    if ( source.getDesc().type_!=VelocityDesc::RMS )
	    {
		res[idx] = vel_[velsz-1];
		continue;
	    }

	    pErrMsg( "Should not happen" );
	}
    }
    else //RMS vel && extrapolate_ && extrapolation needed at the end
    {
	mAllocVarLenArr( double, times, velsz );
	if ( !mIsVarLenArrOK(times) ) return false;
	for ( int idx=0; idx<velsz; idx++ )
	    times[idx] = velsampling.atIndex( idx );

	return sampleVrms( vel_.arr(), (double)statics_, staticsvel_, times,
			   velsz, sd, res, nr );
    }

    return true;
}


VolumeFunctionSource::VolumeFunctionSource()
    : zit_( SI().zIsTime() )
{}


VolumeFunctionSource::~VolumeFunctionSource()
{
    deepErase( velreader_ );
}


bool VolumeFunctionSource::zIsTime() const
{ return zit_; }


bool VolumeFunctionSource::setFrom( const MultiID& velid )
{
    deepErase( velreader_ );
    threads_.erase();

    PtrMan<IOObj> velioobj = IOM().get( velid );
    if ( !velioobj )
    {
	errmsg_ = "Velocity volume with id: ";
	errmsg_.add( velid ).add(" is not found." );
	return false;
    }

    if ( !desc_.usePar( velioobj->pars() ) )
        return false;

    zit_ = SI().zIsTime();
    velioobj->pars().getYN( sKeyZIsTime(), zit_ );

    mid_ = velid;

    return true;
}


SeisTrcReader* VolumeFunctionSource::getReader()
{
    Threads::Locker lckr( readerlock_ );
    const void* thread = Threads::currentThread();

    const int idx = threads_.indexOf( thread );
    if ( threads_.validIdx(idx) )
	return velreader_[idx];

    PtrMan<IOObj> velioobj = IOM().get( mid_ );
    if ( !velioobj )
	return nullptr;

    auto* velreader = new SeisTrcReader( *velioobj );
    if ( !velreader->prepareWork() )
    {
	delete velreader;
	return nullptr;
    }

    velreader_ += velreader;
    threads_ += thread;

    return velreader;
}


void VolumeFunctionSource::getAvailablePositions( BinIDValueSet& bids ) const
{
    VolumeFunctionSource* myself = const_cast<VolumeFunctionSource*>(this);
    SeisTrcReader* velreader = myself->getReader();

    if ( !velreader || !velreader->seisTranslator() )
	return;

    const SeisPacketInfo& packetinfo =
	velreader->seisTranslator()->packetInfo();

    if ( packetinfo.cubedata )
	bids.add( *packetinfo.cubedata );
    else
    {
	const StepInterval<int>& inlrg = packetinfo.inlrg;
	const StepInterval<int>& crlrg = packetinfo.crlrg;
	for ( int inl=inlrg.start; inl<=inlrg.stop; inl +=inlrg.step )
	{
	    for ( int crl=crlrg.start; crl<=crlrg.stop; crl +=crlrg.step )
	    {
		bids.add( BinID(inl,crl) );
	    }
	}
    }
}


bool VolumeFunctionSource::getVel( const BinID& bid,
			    SamplingData<float>& sd, TypeSet<float>& trcdata )
{
    SeisTrcReader* velreader = getReader();
    if ( !velreader )
    {
	pErrMsg("No reader available");
	return false;
    }

    mDynamicCastGet( SeisTrcTranslator*, veltranslator,
		     velreader->translator() );

    if ( !veltranslator || !veltranslator->supportsGoTo() )
    {
	pErrMsg("Velocity translator not capable enough");
	return false;
    }

    if ( !veltranslator->goTo(bid) )
	return false;

    SeisTrc velocitytrc;
    if ( !velreader->get(velocitytrc) )
	return false;

    trcdata.setSize( velocitytrc.size(), mUdf(float) );
    for ( int idx=0; idx<velocitytrc.size(); idx++ )
	trcdata[idx] = velocitytrc.get( idx, 0 );

    sd = velocitytrc.info().sampling;

    return true;
}


VolumeFunction*
VolumeFunctionSource::createFunction(const BinID& binid)
{
    VolumeFunction* res = new VolumeFunction( *this );
    if ( !res->moveTo(binid) )
    {
	delete res;
	return 0;
    }

    return res;
}


FunctionSource* VolumeFunctionSource::create(const MultiID& mid)
{
    VolumeFunctionSource* res = new VolumeFunctionSource;
    if ( !res->setFrom( mid ) )
    {
	FunctionSource::factory().errMsg() = res->errMsg();
	delete res;
	return 0;
    }

    return res;
}


} // namespace Vel
