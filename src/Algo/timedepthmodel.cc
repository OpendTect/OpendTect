/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "timedepthmodel.h"

#include "ailayer.h"
#include "idxable.h"
#include "odmemory.h"
#include "uistrings.h"


TimeDepthModel::TimeDepthModel()
{}


TimeDepthModel::TimeDepthModel( const TimeDepthModel& oth )
{
    *this = oth;
}


TimeDepthModel::~TimeDepthModel()
{
    setEmpty();
}


TimeDepthModel& TimeDepthModel::operator=( const TimeDepthModel& oth )
{
    errmsg_.setEmpty();
    setEmpty();

    const int sz = oth.sz_;
    const int nrbytes = sz*sizeof(float);

    mTryAlloc( times_, float[sz] );
    if ( times_ )
	OD::sysMemCopy( times_, oth.times_, nrbytes );
    else
    {
	errmsg_ = uiStrings::phrCannotAllocateMemory( nrbytes );
	return *this;
    }

    if ( oth.owndepths_ )
    {
	mTryAlloc( depths_, float[sz] );
	if ( depths_ )
	    OD::sysMemCopy( depths_, oth.depths_, nrbytes );
	else
	{
	    deleteAndNullArrPtr( times_ );
	    errmsg_ = uiStrings::phrCannotAllocateMemory( nrbytes );
	    return *this;
	}
    }
    else
	depths_ = oth.depths_;

    sz_ = sz;
    owndepths_ = oth.owndepths_;

    return *this;
}


void TimeDepthModel::setEmpty()
{
    sz_ = 0;
    deleteAndNullArrPtr( times_ );
    if ( owndepths_ )
	delete [] depths_;
    depths_ = nullptr;
}


bool TimeDepthModel::isOK() const
{ return times_ && depths_ && sz_ > 0; }


#ifdef __debug__
# define mChkIdx \
    if ( idx >= sz_ ) \
    { \
	BufferString msg("sz_=",sz_, " asked "); msg.add( idx ); \
	pErrMsg(msg); idx = sz_ - 1; \
    }
#else
# define mChkIdx \
    if ( idx >= sz_ ) \
	idx = sz_ - 1;
#endif


float TimeDepthModel::getDepth( float time ) const
{ return isOK() ? convertTo( depths_, times_, sz_, time, false ) : mUdf(float);}


float TimeDepthModel::getDepth( int idx ) const
{ mChkIdx; return isOK() ? depths_[idx] : mUdf(float); }


float TimeDepthModel::getTime( float dpt ) const
{ return isOK() ? convertTo( depths_, times_, sz_, dpt, true ) : mUdf(float); }


float TimeDepthModel::getTime( int idx ) const
{ mChkIdx; return isOK() ? times_[idx] : mUdf(float); }


float TimeDepthModel::getFirstTime() const
{ return isOK() ? getTime(0) : mUdf(float); }


float TimeDepthModel::getLastTime() const
{ return isOK() ? getTime(sz_-1) : mUdf(float); }


float TimeDepthModel::getVelocity( float dpt ) const
{ return isOK() ? getVelocity( depths_, times_, sz_, dpt ) : mUdf(float); }


float TimeDepthModel::getDepth( const float* dpths, const float* times,
					int sz, float time)
{ return convertTo( dpths, times, sz, time, false ); }


float TimeDepthModel::getTime( const float* dpths, const float* times,
					int sz, float dpt)
{ return convertTo( dpths, times, sz, dpt, true ); }


float TimeDepthModel::getVelocity( const float* dpths, const float* times,
				   int sz, float depth )
{
    if ( sz < 2 )
	return mUdf(float);

    int idx1;
    IdxAble::findFPPos( dpths, sz, depth, -1, idx1 );
    if ( idx1 < 1 )
	idx1 = 1;
    else if ( idx1 > sz-1 )
	idx1 = sz - 1;

    int idx0 = idx1 - 1;
    const float ddt = times[idx1] - times[idx0];
    const float ddh = dpths[idx1] - dpths[idx0];
    return ddt ? ddh / ddt : mUdf(float);
}


float TimeDepthModel::convertTo( const float* dpths, const float* times,
				 int sz, float z, bool targetistime )
{
    const float* zinvals = targetistime ? dpths : times;
    const float* zoutvals = targetistime ? times : dpths;

    int idx1;
    if ( IdxAble::findFPPos( zinvals, sz, z, -1, idx1 ) )
	return zoutvals[idx1];
    else if ( sz < 2 )
	return mUdf(float);
    else if ( idx1 < 0 || idx1 == sz-1 )
    {
	int idx0 = idx1 < 0 ? 1 : idx1;
	const float ddh = dpths[idx0] - dpths[idx0-1];
	const float ddt = times[idx0] - times[idx0-1];
	const float v = ddt ? ddh / ddt : mUdf(float);
	idx0 = idx1 < 0 ? 0 : idx1;
	const float zshift = z - zinvals[idx0];
	const float zout = zoutvals[idx0];
	return targetistime ? ( v ? zout + zshift / v : mUdf(float) )
			    : zout + zshift * v;
    }

    const int idx2 = idx1 + 1;
    const float z1 = z - zinvals[idx1];
    const float z2 = zinvals[idx2] - z;
    return z1+z2 ? (z1 * zoutvals[idx2] + z2 * zoutvals[idx1]) / (z1 + z2)
		 : mUdf(float);
}


bool TimeDepthModel::setModel( const float* dpths, const float* times, int sz )
{
    if ( !owndepths_ )
	return false;

    setEmpty();

    PointBasedMathFunction func;
    for ( int idx=0; idx<sz; idx++ )
	func.add( times[idx], dpths[idx] );

    mTryAlloc( depths_, float[func.size()] );
    if ( !depths_ )
	{ errmsg_ = tr("Out of memory"); return false; }

    mTryAlloc( times_, float[func.size()] );
    if ( !times_ )
    {
	errmsg_ = tr("Out of memory");
	deleteAndNullArrPtr( depths_ ) ;
	return false;
    }

    for ( int idx=0; idx<func.size(); idx++ )
    {
	times_[idx] = func.xVals()[idx];
	depths_[idx] = func.yVals()[idx];
    }

    sz_ = func.size();

    return true;
}


void TimeDepthModel::setAllVals( float* dpths, float* times, int sz )
{
    setVals( dpths, true );
    setVals( times, false );
    setSize( sz );
}


void TimeDepthModel::setVals( float* arr, bool isdepth, bool becomesmine )
{
    if ( isdepth )
    {
	if ( owndepths_ )
	    delete [] depths_;
	depths_ = arr;
	owndepths_ = becomesmine;
    }
    else
    {
	delete [] times_;
	times_ = arr;
    }
}


void TimeDepthModel::forceTimes( const TimeDepthModel& oth )
{
    if ( oth.sz_ == sz_ )
    {
	for ( int idx=0; idx<sz_; idx++ )
	    times_[idx] = oth.getTime( depths_[idx] );
    }
    else
	*this = oth;
}


// TimeDepthModelSet::Setup

TimeDepthModelSet::Setup::Setup()
    : pdown_(true)
    , pup_(true)
    , starttime_(0.f)
    , startdepth_(0.f)
    , depthtype_(ZDomain::DepthType::Meter)
{
}


TimeDepthModelSet::Setup::~Setup()
{
}


void TimeDepthModelSet::Setup::fillPar( IOPar& iop ) const
{
}


bool TimeDepthModelSet::Setup::usePar( const IOPar& iop )
{
    return true;
}


bool TimeDepthModelSet::Setup::areDepthsInFeet() const
{
    return depthtype_ == ZDomain::DepthType::Feet;
}


// TimeDepthModelSet

TimeDepthModelSet::TimeDepthModelSet( const ElasticModel& emodel,
				      const Setup& tdmsu,
				      const TypeSet<float>* axisvals,
				      float* velmax )
    : TimeDepthModelSet(emodel.size()+1,axisvals)
{
    if ( !isOK() )
	return;

    setFrom( emodel, tdmsu, velmax );
}


TimeDepthModelSet::TimeDepthModelSet( const TimeDepthModel& tdmodel,
				      const TypeSet<float>* axisvals )
    : TimeDepthModelSet(tdmodel.size(),axisvals)
{
    if ( !isOK() || !tdmodel.isOK() )
	return;

    const int nrbytes = defmodel_->size() * sizeof(float);
    const float* twtvals = tdmodel.getTimes();
    OD::sysMemCopy( defmodel_->getDepths(), tdmodel.getDepths(), nrbytes );
    OD::sysMemCopy( defmodel_->getTimes(), twtvals, nrbytes );
    for ( auto* model : tdmodels_ )
    {
	if ( model != defmodel_ )
	    OD::sysMemCopy( model->getTimes(), twtvals, nrbytes );
    }
}


TimeDepthModelSet::TimeDepthModelSet( int modelsz,
				      const TypeSet<float>* axisvals )
{
    if ( axisvals &&
	(axisvals->size() > 1 ||
	(axisvals->size() == 1 && (!mIsZero( axisvals->first(), 1e-4f )))) )
	singleton_ = false;

    init( modelsz, axisvals );
}


TimeDepthModelSet::~TimeDepthModelSet()
{
    if ( singleton_ || !tdmodels_.isPresent(defmodel_) )
	delete defmodel_;

    deepErase( tdmodels_ );
}


bool TimeDepthModelSet::isOK() const
{
    return !isbad_ && defmodel_ && defmodel_->isOK();
}


int TimeDepthModelSet::nrModels() const
{
    return singleton_ ? 1 : tdmodels_.size();
}


int TimeDepthModelSet::modelSize() const
{
    return defmodel_->size();
}


const TimeDepthModel& TimeDepthModelSet::getDefaultModel() const
{
    return *defmodel_;
}


TimeDepthModel& TimeDepthModelSet::getDefaultModel()
{
    return *defmodel_;
}


const TimeDepthModel* TimeDepthModelSet::get( int idx ) const
{
    return mSelf().get( idx );
}


TimeDepthModel* TimeDepthModelSet::get( int idx )
{
    return singleton_ ? defmodel_
		      : (tdmodels_.validIdx( idx ) ? tdmodels_.get( idx )
						   : nullptr);
}


void TimeDepthModelSet::init( int modelsz, const TypeSet<float>* axisvals )
{
    mDeclareAndTryAlloc( float*, depths, float[modelsz] );
    if ( !depths )
    {
	isbad_ = true;
	return;
    }

    mDeclareAndTryAlloc( float*, times, float[modelsz] );
    if ( !times )
    {
	delete [] depths;
	isbad_ = true;
	return;
    }

    defmodel_ = new TimeDepthModel;
    defmodel_->setAllVals( depths, times, modelsz );
    if ( singleton_ )
	return;

    for ( const auto& xval : *axisvals )
    {
	if ( mIsZero(xval,1e-4f) )
	{
	    tdmodels_.add( defmodel_ );
	    continue;
	}

	mTryAlloc( times, float[modelsz] );
	if ( !times )
	{
	    isbad_ = true;
	    break;
	}

	auto* tdmodel = new TimeDepthModel;
	tdmodel->setVals( defmodel_->getDepths(), true, false );
	tdmodel->setVals( times, false );
	tdmodel->setSize( modelsz );
	tdmodels_.add( tdmodel );
    }
}


void TimeDepthModelSet::setFrom( const ElasticModel& emodel,
				 const Setup& tdmsu, float* velmax )
{
    float* twtarr = defmodel_->getTimes();
    float* deptharr = defmodel_->getDepths();
    int idz = 0;
    twtarr[idz] = tdmsu.starttime_;
    deptharr[idz++] = tdmsu.startdepth_;

    const bool zinfeet = tdmsu.areDepthsInFeet();
    float dnmotime, dvrmssum, unmotime, uvrmssum;
    float prevdnmotime, prevdvrmssum, prevunmotime, prevuvrmssum;
    prevdnmotime = prevdvrmssum = prevunmotime = prevuvrmssum = 0.f;
    for ( const auto* layer : emodel )
    {
	float dz = layer->getThickness();
	if ( zinfeet )
	    dz *= mToFeetFactorF;
	deptharr[idz] = deptharr[idz-1] + dz;

	float dvel = tdmsu.pdown_ ? layer->getPVel() : layer->getSVel();
	float uvel = tdmsu.pup_ ? layer->getPVel() : layer->getSVel();
	if ( zinfeet )
	{
	    dvel *= mToFeetFactorF;
	    uvel *= mToFeetFactorF;
	}

	dnmotime = dvrmssum = unmotime = uvrmssum = 0.f;

	dnmotime = dz / dvel;
	dvrmssum = dz * dvel;
	unmotime = dz / uvel;
	uvrmssum = dz * uvel;

	dvrmssum += prevdvrmssum;
	uvrmssum += prevuvrmssum;
	dnmotime += prevdnmotime;
	unmotime += prevunmotime;

	prevdvrmssum = dvrmssum;
	prevuvrmssum = uvrmssum;
	prevdnmotime = dnmotime;
	prevunmotime = unmotime;

	const float vrmssum = dvrmssum + uvrmssum;
	const float twt = unmotime + dnmotime;
	if ( velmax )
	    velmax[idz-1] = Math::Sqrt( vrmssum / twt );

	twtarr[idz++] = twt;
    }

    const int sz = modelSize();
    for ( auto* model : tdmodels_ )
    {
	if ( model != defmodel_ && sz > 0 )
	{
	    OD::sysMemValueSet( model->getTimes(), mUdf(float), sz );
	    model->getTimes()[0] = tdmsu.starttime_;
	}
    }
}


void TimeDepthModelSet::setDepth( int idz, float zval )
{
#ifdef __debug__
    if ( !defmodel_ || idz < 0 || idz >= defmodel_->sz_ )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    defmodel_->depths_[idz] = zval;
}


void TimeDepthModelSet::setDefTWT( int idz, float twt )
{
#ifdef __debug__
    if ( !defmodel_ || idz < 0 || idz >= defmodel_->sz_ )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    defmodel_->times_[idz] = twt;
}


void TimeDepthModelSet::setTWT( int imdl, int idz, float twt )
{
#ifdef __debug__
    if ( !get(imdl) || idz < 0 || idz >= get(imdl)->sz_ )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    get( imdl )->times_[idz] = twt;
}


void TimeDepthModelSet::forceTimes( const TimeDepthModel& tdmodel, bool defonly)
{
    TimeDepthModel& defmodel = getDefaultModel();
    defmodel.forceTimes( tdmodel );
    if ( defonly )
	return;

    for ( auto* model : tdmodels_ )
    {
	if ( model != &defmodel )
	    model->forceTimes( tdmodel );
    }
}
