/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocityfunction.h"

#include "attribdataholder.h"
#include "binidvalset.h"
#include "interpol1d.h"
#include "ioman.h"
#include "ioobj.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "unitofmeasure.h"
#include "veldescimpl.h"
#include "zvalseriesimpl.h"

#include "velocityfunctioninterval.h"
#include "velocityfunctiongrid.h"
#include "hiddenparam.h"

namespace Vel
{

static HiddenParam<Function,VelocityDesc*> velfunctiondescmgr_(nullptr);
static HiddenParam<Function,const ZDomain::Info*> velfunctionzinfomgr_(nullptr);
static HiddenParam<Function,Pos::GeomID*> velfunctiongeomidmgr_(nullptr);


Function::Function( FunctionSource& vfs )
    : source_(vfs)
    , bid_(mUdf(int),mUdf(int))
    , desiredrg_(SI().zRange(true))
{
    velfunctiondescmgr_.setParam( this, new VelocityDesc );
    velfunctionzinfomgr_.setParam( this,
				   new ZDomain::Info(SI().zDomainInfo()) );
    velfunctiongeomidmgr_.setParam( this,
				   new Pos::GeomID(Survey::default3DGeomID()) );
    Worker::setUnit( UnitOfMeasure::surveyDefVelUnit(), desc_() );
    source_.ref();
}


Function::~Function()
{
    delete cache_;
    source_.removeFunction( this );
    source_.unRef();
    velfunctiondescmgr_.removeAndDeleteParam ( this );
    velfunctionzinfomgr_.removeAndDeleteParam ( this );
    velfunctiongeomidmgr_.removeAndDeleteParam( this );
}


VelocityDesc& Function::desc_()
{
    return *velfunctiondescmgr_.getParam( this );
}


const ZDomain::Info* Function::zdomaininfo_()
{
    return velfunctionzinfomgr_.getParam( this );
}


Pos::GeomID Function::geomid_()
{
    return *velfunctiongeomidmgr_.getParam( this );
}


const VelocityDesc& Function::getDesc() const
{
    return mSelf().desc_();
}


const ZDomain::Info& Function::zDomain() const
{
    mDynamicCastGet(const IntervalFunction*,intfunction,this);
    if ( intfunction )
	return intfunction->zDomain_();
    return *mSelf().zdomaininfo_();
}


bool Function::zIsTime() const
{
    return zDomain().isTime();
}


bool Function::zInMeter() const
{
    return zDomain().isDepthMeter();
}


bool Function::zInFeet() const
{
    return zDomain().isDepthFeet();
}


const UnitOfMeasure* Function::velUnit() const
{
    return Worker::getUnit( getDesc() );
}


const StepInterval<float>& Function::getDesiredZ() const
{
    return desiredrg_;
}


void Function::setDesiredZRange( const StepInterval<float>& zsamp )
{
    desiredrg_ = zsamp;
}


void Function::setGeomID( const Pos::GeomID& geomid )
{
    *velfunctiongeomidmgr_.getParam( this ) = geomid;
}


Pos::GeomID Function::getGeomID() const
{
    return mSelf().geomid_();
}


Function& Function::setZDomain( const ZDomain::Info& zinfo )
{
    if ( (!zinfo.isTime() && !zinfo.isDepth()) || zinfo == zDomain() )
	return *this;

    mDynamicCastGet(IntervalFunction*,intfunction,this);
    if ( intfunction )
	return intfunction->setZDomain_( zinfo );

    velfunctionzinfomgr_.deleteAndNullPtrParam( this );
    auto* zdomaininfo = new ZDomain::Info( zinfo );
    velfunctionzinfomgr_.setParam( this, zdomaininfo );
    desiredrg_.setUdf();

    return *this;
}


Function& Function::copyDescFrom( const FunctionSource& src )
{
    if ( src.getDesc().isUdf() )
	return *this;

    const UnitOfMeasure* veluom = Worker::getUnit( getDesc() );
    desc_() = src.getDesc();
    Worker::setUnit( veluom, desc_() );

    return *this;
}


#define cDefSampleSnapDist 1e-3f

float Function::getVelocity( float z ) const
{
    Threads::Locker cachelckr( cachelock_ );
    if ( !cache_ )
    {
	const ZSampling sampling( getDesiredZ() );
	cachesd_ = RegularZValues::getDoubleSamplingData(
					SamplingData<float>( sampling ) );
	const int zstart = mNINT32( cachesd_.start / cachesd_.step );
	const int zstop = mNINT32( mCast(double,sampling.stop)/cachesd_.step );
	mTryAlloc( cache_, TypeSet<float>( zstop-zstart+1, mUdf(float) ) );
	if ( !cache_ )
	    return mUdf(float);

	if ( !computeVelocity((float)cachesd_.start,(float)cachesd_.step,
			      cache_->size(),cache_->arr()) )
	{
	    deleteAndNullPtr( cache_ );
	    return mUdf( float );
	}
    }
    cachelckr.unlockNow();

    const int sz = cache_->size();
    const int sampidx = cachesd_.nearestIndex( z );
    if ( sampidx<0 || sampidx>=sz )
	return mUdf(float);
    else if ( sampidx<0 )
	return (*cache_)[0];
    else if ( sampidx>=sz-1 )
	return (*cache_)[sz-1];

    const float pos = mCast(float,( z - cachesd_.start ) / cachesd_.step);
    if ( sampidx-pos > -cDefSampleSnapDist && sampidx-pos < cDefSampleSnapDist )
	return (*cache_)[sampidx];

    return Interpolate::linearReg1DWithUdf( (*cache_)[sampidx], sampidx<(sz-1)
					  ? (*cache_)[sampidx+1]
					  : mUdf(float), pos );
}


const BinID& Function::getBinID() const
{ return bid_; }


bool Function::moveTo( const BinID& bid )
{
    bid_ = bid;
    removeCache();

    return true;
}


void Function::removeCache()
{
    Threads::Locker lckr( cachelock_ );
    deleteAndNullPtr( cache_ );
}


// FunctionSource

static HiddenParam<FunctionSource,const ZDomain::Info*>
					velfunctionsourcezinfomgr_(nullptr);

mImplFactory1Param( FunctionSource, const MultiID&, FunctionSource::factory );


FunctionSource::FunctionSource()
{
    velfunctionsourcezinfomgr_.setParam( this,
			new ZDomain::Info(SI().zDomainInfo()) );
}


FunctionSource::~FunctionSource()
{
    velfunctionsourcezinfomgr_.removeAndDeleteParam( this );
}


BufferString FunctionSource::userName() const
{
    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    if ( ioobj )
	return ioobj->name();

    return BufferString( factoryKeyword() );
}


const ZDomain::Info* FunctionSource::zdomaininfo_()
{
    return velfunctionsourcezinfomgr_.getParam( this );
}


const ZDomain::Info& FunctionSource::zDomain() const
{
    mDynamicCastGet(const IntervalSource*,intsource,this);
    if ( intsource )
	return intsource->zDomain_();
    mDynamicCastGet(const GriddedSource*,griddedsource,this);
    if ( griddedsource )
	return griddedsource->zDomain_();

    return *mSelf().zdomaininfo_();
}


bool FunctionSource::zIsTime() const
{
    return zDomain().isTime();
}


bool FunctionSource::zInMeter() const
{
    return zDomain().isDepthMeter();
}


bool FunctionSource::zInFeet() const
{
    return zDomain().isDepthFeet();
}


const UnitOfMeasure* FunctionSource::velUnit() const
{
    mDynamicCastGet(const IntervalSource*,intsource,this);
    if ( intsource )
	return intsource->getVelUnit_();

    return Worker::getUnit( getDesc() );
}


FunctionSource& FunctionSource::setZDomain( const ZDomain::Info& zinfo )
{
    if ( (!zinfo.isTime() && !zinfo.isDepth()) || zinfo == zDomain() )
	return *this;

    mDynamicCastGet(GriddedSource*,griddedsource,this);
    if ( griddedsource )
	return griddedsource->setZDomain_( zinfo );

    velfunctionsourcezinfomgr_.deleteAndNullPtrParam( this );
    auto* zdomaininfo = new ZDomain::Info( zinfo );
    velfunctionsourcezinfomgr_.setParam( this, zdomaininfo );

    return *this;
}


void FunctionSource::removeFunction( const Function* func )
{
    Threads::Locker lckr( lock_ );

    int idx = functions_.indexOf( func );

    if ( idx!=-1 )
	functions_.removeSingle( idx );
}


const char* FunctionSource::errMsg() const
{
    return errmsg_.buf();
}


void FunctionSource::getSurroundingPositions( const BinID& bid,
					      BinIDValueSet& bids) const
{
    BinIDValueSet mybids( 0, false );
    getAvailablePositions( mybids );
    if ( !mybids.isEmpty() )
	bids.append( mybids ); //Filter?
}


int FunctionSource::findFunction( const BinID& bid ) const
{
    for ( int idx=functions_.size()-1; idx>=0; idx-- )
    {
	if ( functions_[idx]->getBinID()==bid )
	    return idx;
    }

    return -1;
}


ConstRefMan<Function> FunctionSource::getFunction( const BinID& bid )
{
    if ( mIsUdf(bid.inl()) || mIsUdf(bid.crl()) )
	return nullptr;

    Threads::Locker lckr( lock_ );
    RefMan<Function> tmpfunc;
    int idx = findFunction( bid );
    if ( idx==-1 )
    {
	tmpfunc = createFunction( bid );
	if ( !tmpfunc )
	    return nullptr;

	functions_ += tmpfunc.ptr();
    }
    else
	tmpfunc = functions_[idx];

    lckr.unlockNow();

    return ConstRefMan<Function>( tmpfunc.ptr() );
}

} // namespace Vel
