/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: velocityfunctionvolume.cc,v 1.1 2008-07-22 17:39:21 cvskris Exp $";

#include "velocityfunctionvolume.h"

#include "cubesampling.h"
#include "idxable.h"
#include "ioman.h"
#include "seisread.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "survinfo.h"

namespace Vel
{


void VolumeFunctionSource::initClass()
{ FunctionSource::factory().addCreator( create, sType() ); }



VolumeFunction::VolumeFunction( VolumeFunctionSource& source )
    : Function( source )
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


StepInterval<float> VolumeFunction::getAvailableZ() const
{
    return StepInterval<float>( velsampling_.start,
				velsampling_.atIndex(vel_.size()-1),
			        velsampling_.step );
}


bool VolumeFunction::computeVelocity( float z0, float dz, int nr,
				      float* res ) const
{
    if ( vel_.isEmpty() )
	return false;

    mDynamicCastGet( VolumeFunctionSource&, source, source_ );

    if ( mIsEqual(z0,velsampling_.start,1e-5) &&
	 mIsEqual(velsampling_.step,dz,1e-5) )
    {
	const int msize = mMIN(vel_.size(),nr);
	memcpy( res, vel_.arr(), sizeof(float)*msize );
	for ( int idx=msize; idx<nr; idx++ )
	    res[idx] = mUdf(float);
    }
    else
    {
	for ( int idx=0; idx<nr; idx++ )
	{
	    const float z = z0+dz*idx;
	    const float sample = velsampling_.getIndex( z );
	    if ( sample<0 || sample>=vel_.size() )
		res[idx] = mUdf(float);
	    else
		res[idx] = IdxAble::interpolateReg<const float*>( vel_.arr(),
			vel_.size(), sample, false );
	}
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


void VolumeFunctionSource::getAvailablePositions(
	HorSampling& hrg ) const
{
    if ( !velreader_ || !velreader_->seisTranslator() )
	return;


    const SeisPacketInfo& packetinfo =
	velreader_->seisTranslator()->packetInfo();

    hrg.set( packetinfo.inlrg, packetinfo.crlrg );
}


bool VolumeFunctionSource::getVel( const BinID& bid,
			    SamplingData<float>& sd, TypeSet<float>& trcdata )
{
    Threads::MutexLocker lock( readerlock_ );

    mDynamicCastGet( SeisTrcTranslator*, veltranslator,
		     velreader_->translator() );

    if ( !veltranslator->supportsGoTo() ||
	 !veltranslator->goTo(bid) )
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
