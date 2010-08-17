/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: velocityfunctionvolume.cc,v 1.9 2010-08-17 20:32:46 cvskris Exp $";

#include "velocityfunctionvolume.h"

#include "cubesampling.h"
#include "idxable.h"
#include "ioman.h"
#include "seisread.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "seistrctr.h"
#include "survinfo.h"

namespace Vel
{


void VolumeFunctionSource::initClass()
{ FunctionSource::factory().addCreator( create, sType() ); }



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
    {
	const int msize = mMIN(velsz,nr);
	memcpy( res, vel_.arr(), sizeof(float)*velsz );
    }
    else if ( source.getDesc().type_!=VelocityDesc::RMS ||
	      !extrapolate_ ||
	      velsampling_.atIndex(velsz-1)>z0+dz*(nr-1) )
    {
	for ( int idx=0; idx<nr; idx++ )
	{
	    const float z = z0+dz*idx;
	    const float sample = velsampling_.getIndex( z );
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
    : velreader_( 0 )
    , zit_( SI().zIsTime() )
{}


VolumeFunctionSource::~VolumeFunctionSource()
{ delete velreader_; }


bool VolumeFunctionSource::zIsTime() const
{ return zit_; }


bool VolumeFunctionSource::setFrom( const MultiID& velid )
{
    delete velreader_;
    velreader_ = 0;

    PtrMan<IOObj> velioobj = IOM().get( velid );
    if ( !velioobj )
	return false;

    if ( !desc_.usePar( velioobj->pars() ) )
        return false;

    velreader_ = new SeisTrcReader( velioobj );
    if ( !velreader_->prepareWork() )
    {
	delete velreader_;
	velreader_ = 0;
	return false;
    }

    zit_ = SI().zIsTime();
    velioobj->pars().getYN( sKeyZIsTime(), zit_ );

    mid_ = velid;

    return true;
}


void VolumeFunctionSource::getAvailablePositions( BinIDValueSet& bids ) const
{
    if ( !velreader_ || !velreader_->seisTranslator() )
	return;

    const SeisPacketInfo& packetinfo =
	velreader_->seisTranslator()->packetInfo();

    if ( !packetinfo.cubedata )
	return;

    const PosInfo::CubeData& cubedata = *packetinfo.cubedata;

    for ( int idx=0; idx<cubedata.size(); idx++ )
    {
	const PosInfo::LineData& line = *cubedata[idx];
	const int inl = line.linenr_;
	for ( int idy=0; idy<line.segments_.size(); idy++ )
	{
	    const StepInterval<int> crls = line.segments_[idy];
	    for ( int crl=crls.start; crl<=crls.stop; crl+=crls.step )
	    {
		bids.add( BinID(inl,crl) );
	    }
	}
    }
}


void VolumeFunctionSource::getAvailablePositions(
	HorSampling& hrg ) const
{
    if ( !velreader_ || !velreader_->seisTranslator() )
	return;

    const SeisPacketInfo& packetinfo =
	velreader_->seisTranslator()->packetInfo();

    if ( packetinfo.cubedata )
	return;

    hrg.set( packetinfo.inlrg, packetinfo.crlrg );
}


bool VolumeFunctionSource::getVel( const BinID& bid,
			    SamplingData<float>& sd, TypeSet<float>& trcdata )
{
    Threads::MutexLocker lock( readerlock_ );
    if ( !velreader_ )
    {
	pErrMsg("No reader available");
	return false;
    }

    mDynamicCastGet( SeisTrcTranslator*, veltranslator,
		     velreader_->translator() );

    if ( !veltranslator || !veltranslator->supportsGoTo() )
    {
	pErrMsg("Velocity translator not capable enough");
	return false;
    }

    if ( !veltranslator->goTo(bid) )
	return false;

    SeisTrc velocitytrc;
    if ( !velreader_->get(velocitytrc) )
	return false;

    lock.unLock();

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
	delete res;
	return 0;
    }

    return res;
}


}; //namespace
