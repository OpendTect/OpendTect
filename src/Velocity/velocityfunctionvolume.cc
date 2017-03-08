/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "velocityfunctionvolume.h"

#include "binidvalset.h"
#include "trckeyzsampling.h"
#include "idxable.h"
#include "dbman.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "seisprovider.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "uistrings.h"


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

    if ( mIsEqual(z0,velsampling_.start,1e-5) &&
	 mIsEqual(velsampling_.step,dz,1e-5) &&
	 velsz==nr )
	OD::memCopy( res, vel_.arr(), sizeof(float)*velsz );
    else if ( source.getDesc().type_!=VelocityDesc::RMS ||
	      !extrapolate_ ||
	      velsampling_.atIndex(velsz-1)>z0+dz*(nr-1) )
    {
	for ( int idx=0; idx<nr; idx++ )
	{
	    const float z = z0+dz*idx;
	    const float sample = velsampling_.getfIndex( z );
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
	TypeSet<float> times;
	for ( int idx=0; idx<velsz; idx++ )
	{
	    float t = velsampling_.atIndex( idx );
	    times += t;
	}

	return sampleVrms( vel_.arr(), statics_, staticsvel_, times.arr(),
			   velsz, SamplingData<double>( z0, dz ), res, nr );
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

    PtrMan<IOObj> velioobj = DBM().get( velid );
    if ( !velioobj )
    {
	errmsg_ = uiStrings::phrCannotFindDBEntry(
			    tr("for Velocity volume with id: %1.").arg(velid));
	return false;
    }

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


void VolumeFunctionSource::getAvailablePositions( BinIDValueSet& bids ) const
{
    uiRetVal uirv;
    VolumeFunctionSource* myself = const_cast<VolumeFunctionSource*>(this);
    Seis::Provider* velprovider = myself->getProvider( uirv );
    if ( !velprovider )
	{ pErrMsg( uirv.getText() ); return; }

    if ( !velprovider->is2D() )
    {
	mDynamicCastGet(const Seis::Provider3D&,prov3d,*velprovider);
	PosInfo::CubeData cubedata;
	prov3d.getGeometryInfo( cubedata );
	bids.add( cubedata );
    }
    else
    {
	mDynamicCastGet(const Seis::Provider2D&,prov2d,*velprovider);
	PosInfo::Line2DData line2ddata;
	prov2d.getGeometryInfo( prov2d.curLineIdx(), line2ddata );
	for ( int idx=0; idx<line2ddata.positions().size(); idx++ )
	{
	    const TrcKey trckey = Survey::GM().traceKey( prov2d.curGeomID(),
					line2ddata.positions()[idx].nr_ );
	    bids.add( trckey.binID() );
	}
    }
}


bool VolumeFunctionSource::getVel( const BinID& bid,
			   SamplingData<float>& sd, TypeSet<float>& trcdata )
{
    uiRetVal uirv;
    Seis::Provider* velprovider = getProvider( uirv );
    if ( !velprovider )
	{ pErrMsg( uirv.getText() ); return false; }

    if ( !velprovider->isPresent(TrcKey(bid)) )
	return false;

    SeisTrc velocitytrc;
    const uiRetVal retval = velprovider->get( TrcKey(bid), velocitytrc );
    if ( !retval.isOK() )
	return false;

    trcdata.setSize( velocitytrc.size(), mUdf(float) );
    for ( int idx=0; idx<velocitytrc.size(); idx++ )
	trcdata[idx] = velocitytrc.get( idx, 0 );

    sd = velocitytrc.info().sampling_;

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


FunctionSource* VolumeFunctionSource::create(const DBKey& mid)
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
