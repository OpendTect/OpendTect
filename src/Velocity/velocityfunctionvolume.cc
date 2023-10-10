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
#include "seisioobjinfo.h"
#include "seispacketinfo.h"
#include "seisread.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "veldesc.h"
#include "zvalseriesimpl.h"


namespace Vel
{

// VolumeFunction

VolumeFunction::VolumeFunction( VolumeFunctionSource& source )
    : Function(source)
{}


VolumeFunction::~VolumeFunction()
{
}


bool VolumeFunction::moveTo( const BinID& bid )
{
    if ( !Function::moveTo(bid) )
	return false;

    mDynamicCastGet( VolumeFunctionSource&, source, source_ );
    if ( !source.getVel(bid,velsampling_,vel_) )
    {
	vel_.erase();
	return false;
    }

    if ( getDesiredZ().isUdf() && zDomain() == source.zDomain() )
	setDesiredZRange( getLoadedZ() );

    if ( getDesc().isUdf() )
	copyDescFrom( getSource() );

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


ZSampling VolumeFunction::getAvailableZ() const
{
    if ( extrapolate_ && zDomain() == SI().zDomainInfo() )
	return SI().zRange( true );

    return getLoadedZ();
}


ZSampling VolumeFunction::getLoadedZ() const
{
    return ZSampling( float (velsampling_.start),
		      float (velsampling_.atIndex(vel_.size()-1)),
		      float (velsampling_.step) );
}


bool VolumeFunction::computeVelocity( float z0, float dz, int sz,
				      float* res ) const
{
    const double t0 = mIsUdf(statics_) ? 0. : statics_;
    const ArrayValueSeries<double,double> vels_in( (double*)(vel_.arr()), false,
						   vel_.size() );
    const RegularZValues zvals_in( velsampling_, vel_.size(),
				   source_.zDomain() );
    const SamplingData<float> sd_out( z0, dz );
    const RegularZValues zvals_out( sd_out, sz, zDomain() );
    ArrayValueSeries<double,float> vels_out( res, false, sz );
    const Vel::Worker worker( source_.getDesc(), SI().seismicReferenceDatum(),
			      UnitOfMeasure::surveyDefSRDStorageUnit() );
    if ( !worker.sampleVelocities(vels_in,zvals_in,zvals_out,vels_out,t0) )
	return false;

    const UnitOfMeasure* funcveluom = getVelUnit();
    const UnitOfMeasure* funcsrcveluom = source_.getVelUnit();
    if ( funcveluom != funcsrcveluom )
    {
	for ( int idx=0; idx<sz; idx++ )
	    convValue( res[idx], funcsrcveluom, funcveluom );
    }

    return true;
}


// VolumeFunctionSource

void VolumeFunctionSource::initClass()
{ FunctionSource::factory().addCreator( create, sFactoryKeyword() ); }


VolumeFunctionSource::VolumeFunctionSource()
    : FunctionSource()
    , desc_(*new VelocityDesc)
{
    desc_.setUnit( UnitOfMeasure::surveyDefVelStorageUnit() );
}


VolumeFunctionSource::~VolumeFunctionSource()
{
    deepErase( velreader_ );
    delete &desc_;
}


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

    const IOPar& par = velioobj->pars();
    if ( !desc_.usePar(par) || desc_.isUdf() )
        return false;

    const SeisIOObjInfo info( velid );
    if ( info.isOK() )
	setZDomain( info.zDomain() );

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
	errmsg_ = toString( velreader->errMsg() );
	delete velreader;
	return nullptr;
    }

    velreader_ += velreader;
    threads_ += thread;

    return velreader;
}


void VolumeFunctionSource::getAvailablePositions( BinIDValueSet& bids ) const
{
    SeisTrcReader* velreader = mSelf().getReader();
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
	    for ( int crl=crlrg.start; crl<=crlrg.stop; crl +=crlrg.step )
		bids.add( BinID(inl,crl) );
    }
}


bool VolumeFunctionSource::getVel( const BinID& bid, SamplingData<double>& sd,
				   TypeSet<double>& trcdata )
{
    SeisTrcReader* velreader = getReader();
    if ( !velreader )
    {
	if ( errmsg_.isEmpty() )
	    errmsg_ = "No reader available";

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

    const int icomp = 0;
    trcdata.setSize( velocitytrc.size(), mUdf(double) );
    for ( int idx=0; idx<velocitytrc.size(); idx++ )
	trcdata[idx] = velocitytrc.get( idx, icomp );

    sd = RegularZValues::getDoubleSamplingData( velocitytrc.info().sampling );

    return true;
}


VolumeFunction* VolumeFunctionSource::createFunction( const BinID& binid )
{
    auto* res = new VolumeFunction( *this );
    if ( !res->moveTo(binid) )
    {
	delete res;
	return nullptr;
    }

    return res;
}


FunctionSource* VolumeFunctionSource::create( const MultiID& mid )
{
    if ( mid.groupID() !=
	    IOObjContext::getStdDirData( IOObjContext::Seis )->groupID() )
	return nullptr;

    auto* res = new VolumeFunctionSource;
    if ( !res->setFrom(mid) )
    {
	FunctionSource::factory().errMsg() = res->errMsg();
	delete res;
	return nullptr;
    }

    return res;
}

} // namespace Vel
