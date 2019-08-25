/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "velocityfunctionvolume.h"

#include "binnedvalueset.h"
#include "idxable.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "seisprovider.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "trckey.h"
#include "uistrings.h"


namespace Vel
{


void VolumeFunctionSource::initClass()
{
    FunctionSource::factory().addCreator( create, sFactoryKeyword(),
						    sFactoryDisplayName() );
}



VolumeFunction::VolumeFunction( VolumeFunctionSource& source )
    : Function( source )
    , extrapolate_( false )
    , statics_( 0 )
    , staticsvel_( 0 )
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
	return SI().zRange();

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
    deepErase( velprovider_ );
}


bool VolumeFunctionSource::zIsTime() const
{ return zit_; }


bool VolumeFunctionSource::setFrom( const DBKey& velid )
{
    deepErase( velprovider_ );
    threads_.erase();

    PtrMan<IOObj> velioobj = getIOObj( velid );
    if ( !velioobj )
	{ errmsg_ = uiStrings::phrCannotFindDBEntry( velid ); return false; }

    if ( !desc_.usePar( velioobj->pars() ) )
        return false;

    zit_ = SI().zIsTime();
    velioobj->pars().getYN( sKeyZIsTime(), zit_ );

    mid_ = velid;
    return true;
}


Seis::Provider* VolumeFunctionSource::getProvider( uiRetVal& uirv )
{
    Threads::Locker lckr( providerlock_ );
    const Threads::ThreadID thread = Threads::currentThread();

    const int idx = threads_.indexOf( thread );
    if ( threads_.validIdx(idx) )
	return velprovider_[idx];

    Seis::Provider* velprovider = Seis::Provider::create( mid_, &uirv );
    if ( !velprovider ) return 0;

    velprovider_ += velprovider;
    threads_ += thread;

    return velprovider;
}


void VolumeFunctionSource::getAvailablePositions( BinnedValueSet& bids ) const
{
    uiRetVal uirv;
    Seis::Provider* velprovider = mSelf().getProvider( uirv );
    if ( !velprovider )
	{ ErrMsg( uirv.getText() ); return; }

    bids.add( velprovider->as3D()->possiblePositions() );
}


bool VolumeFunctionSource::getVel( const BinID& bid,
			   SamplingData<float>& sd, TypeSet<float>& trcdata )
{
    uiRetVal uirv;
    Seis::Provider* velprovider = getProvider( uirv );
    if ( !velprovider )
	{ ErrMsg( uirv.getText() ); return false; }

    if ( !velprovider->isPresent(TrcKey(bid)) )
	return false;

    SeisTrc velocitytrc;
    const uiRetVal retval = velprovider->getAt( TrcKey(bid), velocitytrc );
    if ( !retval.isOK() )
	return false;

    trcdata.setSize( velocitytrc.size(), mUdf(float) );
    for ( int idx=0; idx<velocitytrc.size(); idx++ )
	trcdata[idx] = velocitytrc.get( idx, 0 );

    sd = velocitytrc.info().sampling_;

    return true;
}


VolumeFunction*
VolumeFunctionSource::createFunction( const BinID& binid )
{
    VolumeFunction* res = new VolumeFunction( *this );
    if ( !res->moveTo(binid) )
	{ delete res; return 0; }

    return res;
}


FunctionSource* VolumeFunctionSource::create( const DBKey& dbky )
{
    VolumeFunctionSource* res = new VolumeFunctionSource;
    if ( !res->setFrom( dbky ) )
	{ ErrMsg(res->errMsg()); delete res; return 0; }

    return res;
}

} // namespace Vel
