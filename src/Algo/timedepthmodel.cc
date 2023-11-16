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
#include "scaler.h"
#include "valseriesimpl.h"
#include "veldesc.h"
#include "velocitycalc.h"

#include "uistrings.h"

#include "hiddenparam.h"


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

static HiddenParam<TimeDepthModelSet::Setup,ZDomain::DepthType>
		tdmodelsetsudepthtypemgr_(ZDomain::DepthType::Meter);


TimeDepthModelSet::Setup::Setup( const Setup& oth )
{
    tdmodelsetsudepthtypemgr_.setParam( this, ZDomain::DepthType::Meter );
    *this = oth;
}


void TimeDepthModelSet::Setup::removeParams()
{
    if ( tdmodelsetsudepthtypemgr_.hasParam(this) )
	tdmodelsetsudepthtypemgr_.removeParam( this );
}


TimeDepthModelSet::Setup&
TimeDepthModelSet::Setup::operator =( const Setup& oth )
{
    if ( &oth == this )
	return *this;

    pdown_ = oth.pdown_;
    pup_ = oth.pup_;
    starttime_ = oth.starttime_;
    startdepth_ = oth.startdepth_;
    depthtype( oth.depthType() );

    return *this;
}


void TimeDepthModelSet::Setup::fillPar( IOPar& iop ) const
{
}


bool TimeDepthModelSet::Setup::usePar( const IOPar& iop )
{
    return true;
}


TimeDepthModelSet::Setup&
TimeDepthModelSet::Setup::depthtype( ZDomain::DepthType typ )
{
    tdmodelsetsudepthtypemgr_.setParam( this, typ );
    return *this;
}


ZDomain::DepthType TimeDepthModelSet::Setup::depthType() const
{
    if ( !tdmodelsetsudepthtypemgr_.hasParam(this) )
	tdmodelsetsudepthtypemgr_.setParam( &mSelf(),
				ZDomain::DepthType::Meter );

    return tdmodelsetsudepthtypemgr_.getParam( this );
}


bool TimeDepthModelSet::Setup::areDepthsInFeet() const
{
    return depthType() == ZDomain::DepthType::Feet;
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
    const double aboveth = emodel.aboveThickness();
    const double starttime = emodel.startTime();

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
	if ( idz==1 )
	    deptharr[idz] += aboveth;

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

	twtarr[idz++] = twt + starttime;
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


// TimeDepthConverter
mStartAllowDeprecatedSection

TimeDepthConverter::TimeDepthConverter()
    : TimeDepthModel()
    , sd_(*new SamplingData<double>)
{}


TimeDepthConverter::~TimeDepthConverter()
{
    delete &sd_;
}


bool TimeDepthConverter::isOK() const
{
    return size() > 0 && (getTimes() || getDepths());
}


bool TimeDepthConverter::isVelocityDescUseable(const VelocityDesc& vd,
					       bool velintime,
					       uiString* errmsg )
{
    if ( vd.type_==VelocityDesc::Avg || vd.type_==VelocityDesc::Interval )
	return true;

    if ( velintime )
    {
	if ( vd.type_==VelocityDesc::RMS )
	    return true;

	if ( errmsg )
	    *errmsg = tr("Only RMS, Avg and Interval allowed for time based "
		         "models");
	return false;
    }

    if ( errmsg )
	*errmsg = tr("Only Avg and Interval allowed for depth based "
		     "models");

    return false;
}


#define mIsValidVel( v ) (!mIsUdf(v) && v>1e-3)


bool TimeDepthConverter::setVelocityModel( const ValueSeries<float>& vel,
		    int sz, const SamplingData<double>& sd,
		    const VelocityDesc& vd, bool istime, const Scaler* scaler )
{
    setEmpty();

    PtrMan<ValueSeries<float> > ownvint;
    const ValueSeries<float>* vint = &vel;

    switch ( vd.type_ )
    {
	case VelocityDesc::RMS:
	{
	    if ( !istime )
	    {
		errmsg_ = tr("Cannot use RMS velocities in depth");
		break;
	    }

	    mDeclareAndTryAlloc( float*, ptr, float[sz] );
	    if ( !ptr )
	    {
		errmsg_ = uiStrings::phrCannotAllocateMemory( sz*sizeof(float));
		break;
	    }

	    ownvint = new ArrayValueSeries<float,float>( ptr, true, sz );
	    if ( !ownvint || !ownvint->isOK() )
	    {
		errmsg_ = uiStrings::phrCannotAllocateMemory( sz*sizeof(float));
		break;
	    }

	    const float* vrms = vel.arr();
	    ArrPtrMan<float> ownvrms;
	    if ( !vrms )
	    {
		mTryAlloc( ownvrms, float[sz] );
		vrms = ownvrms.ptr();

		if ( vrms )
		{
		    for ( int idx=0; idx<sz; idx++ )
			ownvrms[idx] = vel.value( idx );
		}
	    }

	    if ( !vrms )
	    {
		errmsg_ = uiStrings::phrCannotAllocateMemory( sz*sizeof(float));
		break;
	    }

	    if ( !computeDix(vrms,sd,sz,ownvint->arr()) )
		break;

	    vint = ownvint.ptr();

	    //Don't break, go into Vint
	}
	case VelocityDesc::Interval:
	{
	    if ( istime )
	    {
		mDeclareAndTryAlloc(float*,depths,float[sz])
		if ( depths && calcDepths(*vint,sz,sd,depths,scaler) )
		    setVals( depths, true );
		else
		    { delete [] depths; break; }
	    }
	    else
	    {
		mDeclareAndTryAlloc(float*,times,float[sz])
		if ( times && calcTimes(*vint,sz,sd,times,scaler) )
		    setVals( times, false );
		else
		    { delete [] times; break; }
	    }

	    for ( int idx=0; idx<sz; idx++ )
	    {
		if ( mIsValidVel(vint->value(idx) ) )
		{
		    firstvel_ = vint->value(idx);
		    break;
		}
	    }

	    lastvel_ = vint->value(sz-1);
	    if ( scaler )
	    {
		firstvel_ =  scaler->scale( firstvel_ );
		lastvel_ = scaler->scale( lastvel_ );
	    }

	    setSize( sz );
	    sd_ = sd;
	    break;
	}
	case VelocityDesc::Avg :
	{
	    mAllocVarLenArr( float, vavg, sz );
	    if ( !mIsVarLenArrOK(vavg) ) return false;
	    int previdx = -1;
	    float prevvel = 0;
	    for ( int idx=0; idx<sz; idx++ )
	    {
		const float curvel = vel.value(idx);
		if ( !mIsValidVel(curvel) )
		    continue;

		vavg[idx] = curvel;
		if ( previdx!=-1 )
		{
		    const float gradient = (curvel-prevvel)/(idx-previdx);
		    for ( int idy=previdx+1; idy<idx; idy++ )
			vavg[idy] = prevvel + (idx-idy)*gradient;
		}
		else
		{
		    for ( int idy=0; idy<idx; idy++ )
		        vavg[idy] = curvel;
		}

		previdx = idx;
		prevvel = curvel;
	    }

	    if ( previdx==-1 )
		break;

	    for ( int idx=previdx+1; idx<sz; idx++ )
		vavg[idx] = prevvel;

	    if ( istime )
	    {
		mDeclareAndTryAlloc(float*,depths,float[sz])
		if ( !depths )
		    break;

		for ( int idx=0; idx<sz; idx++ )
		    depths[idx] = (float) ( sd.atIndex(idx) * vavg[idx]/2 );
		setVals( depths, true );
	    }
	    else
	    {
		mDeclareAndTryAlloc(float*,times,float[sz])
		if ( !times )
		    break;

		for ( int idx=0; idx<sz; idx++ )
		    times[idx] = (float) ( sd.atIndex(idx) * 2 / vavg[idx] );
		setVals( times, false );
	    }

	    firstvel_ = vavg[0];
	    lastvel_ = vavg[sz-1];
	    setSize( sz );
	    sd_ = sd;
	    break;
	}
        default:
            break;
    }

    return isOK();
}


bool TimeDepthConverter::calcDepths( ValueSeries<float>& res, int outputsz,
				const SamplingData<double>& timesamp ) const
{
    if ( !isOK() )
	return false;

    calcZ( res, outputsz, timesamp, false );
    return true;
}


bool TimeDepthConverter::calcTimes( ValueSeries<float>& res, int outputsz,
				const SamplingData<double>& depthsamp ) const
{
    if ( !isOK() )
	return false;

    calcZ( res, outputsz, depthsamp, true );
    return true;
}


void TimeDepthConverter::calcZ( ValueSeries<float>& res, int outputsz,
				const SamplingData<double>& zsamp,
				bool time ) const
{
    const float* zvals = time ? getDepths() : getTimes();
    const int inpsz = size();
    float seisrefdatum = SI().seismicReferenceDatum();
    if ( SI().zIsTime() && SI().depthsInFeet() )
	seisrefdatum *= mToFeetFactorF;

    const float* zrevvals = time ? getTimes() : getDepths();
    if ( zrevvals )
    {
	StepInterval<double> zrg;
	if ( !regularinput_ )
	{
	    zrg.start = zvals[0];
	    zrg.stop = zvals[inpsz-1];
	    zrg.step = mUdf(double);
	}
	else
	    zrg = sd_.interval( inpsz );

	for ( int idx=0; idx<outputsz; idx++ )
	{
	    double z = zsamp.atIndex( idx );
	    if ( time )
		z += seisrefdatum;

	    float zrev;
	    if ( z <= zrg.start )
	    {
		const double dz = z-zrg.start;
		zrev = (float) ( time ? firstvel_ > 0 ?
					(zrevvals[0]+dz*2/firstvel_)
				      : zrevvals[0]
				      : (zrevvals[0] + dz*firstvel_/2) );
	    }
	    else if ( z >= zrg.stop )
	    {
		const double dz = z-zrg.stop;
		zrev = (float) (  time ?  (zrevvals[inpsz-1] + dz*2/lastvel_)
				    :  (zrevvals[inpsz-1] + dz*lastvel_/2) );
	    }
	    else
	    {
		float zsample;
		if ( regularinput_ )
		{
		    zsample = zrg.getfIndex( z );
		}
		else
		{
		    int sampidx;
		    IdxAble::findPos( zvals, inpsz, z, -1, sampidx );
		    zsample = zvals[sampidx];
		}

		zrev = IdxAble::interpolateReg( zrevvals, inpsz, zsample );
	    }

	    if ( !time )
		zrev -= seisrefdatum;

	    res.setValue( idx, zrev );
	}
    }
    else
    {
	int zidx = 0;
	for ( int idx=0; idx<outputsz; idx++ )
	{
	    double z = zsamp.atIndex( idx );
	    if ( time )
		z += seisrefdatum;

	    float zrev;
	    if ( z<=zvals[0] )
	    {
		const double dz = z-zvals[0];
		zrev = (float)
			( time ? firstvel_>0 ? (sd_.start+dz*2/firstvel_)
					     : (sd_.start)
					     : (sd_.start+dz*firstvel_/2) );
	    }
	    else if ( z > zvals[inpsz-1] )
	    {
		const double dz = z-zvals[inpsz-1];
		zrev = (float) ( time ? (sd_.atIndex(inpsz-1)+dz*2/lastvel_)
			    : (sd_.atIndex(inpsz-1)+dz*lastvel_/2) );
	    }
	    else
	    {
		while ( z>zvals[zidx+1] )
		    zidx++;

		const float relidx = (float) ( zidx +
		    (z-zvals[zidx])/(zvals[zidx+1]-zvals[zidx]) );

		zrev = (float) sd_.atIndex( relidx );
	    }

	    if ( !time ) zrev -= seisrefdatum;
	    res.setValue( idx, zrev );
	}
    }
}



/*!For every time in the velocity model, calculate the depth. The velocity is
   is assumed to be constant at vel[0] above the first time. The velocity is
   specified as Interval velocity, and the sample span is above. That means
   the velocity at a certain sample is the velocity of the layer above the
   corresponding time:
\code
Time(ms) Vel(m/s) Samplespan (ms)	Depth (m)
20	 1000	  16-20			0.020*1000 = 20
24	 1500	  20-24			20+0.004*1500 = 26
28	 2000	  24-28			26+0.004*2000 = 34
\endcode
*/

bool TimeDepthConverter::calcDepths( const ValueSeries<float>& vels, int velsz,
				     const SamplingData<double>& sd,
				     float* depths, const Scaler* scaler )
{
    mAllocVarLenArr( double, zvals, velsz );
    if ( !mIsVarLenArrOK(zvals) )
	return false;

    ArrayValueSeries<double,double> times( velsz );
    double* timesptr = times.arr();
    for ( int idx=0; idx<velsz; idx++, timesptr++ )
	*timesptr = sd.atIndex( idx );

    if ( !calcDepths(vels,velsz,times,zvals,scaler) )
	return false;

    for ( int idx=0; idx<velsz; idx++ )
	depths[idx] = mCast(float,zvals[idx]);

    return true;
}


bool TimeDepthConverter::calcDepths(const ValueSeries<float>& vels, int velsz,
				    const ValueSeries<float>& times,
				    float* depths )
{
    mAllocVarLenArr( double, tvals, velsz );
    mAllocVarLenArr( double, zvals, velsz );
    if ( !mIsVarLenArrOK(tvals) || !mIsVarLenArrOK(zvals) )
	return false;

    for ( int idx=0; idx<velsz; idx++ )
	tvals[idx] = times.value( idx );

    double* tvalsptr = tvals;
    const ArrayValueSeries<double,double> tinser(
					const_cast<double*>(tvalsptr), false );

    if ( !calcDepths(vels,velsz,tinser,zvals,nullptr) )
	return false;

    for ( int idx=0; idx<velsz; idx++ )
	depths[idx] = mCast(float,zvals[idx]);

    return true;
}


bool TimeDepthConverter::calcDepths(const ValueSeries<float>& vels, int velsz,
				    const ValueSeries<double>& times,
				    double* depths, const Scaler* scaler )
{
    if ( !depths )
    {
	pFreeFnErrMsg("No output pointer given." );
	return false;
    }

    if ( !velsz )
	return true;

    double prevvel = mUdf(double);
    int startidx = -1;
    for ( int idx=0; idx<velsz; idx++ )
    {
	if ( !mIsValidVel(vels.value(idx)) )
	    continue;

	startidx = idx;
	prevvel = vels.value(idx);
	if ( scaler )
	    prevvel = scaler->scale( prevvel );

	break;
    }

    if ( startidx==-1 )
	return false;

    prevvel /= 2.; //time is TWT
    for ( int idx=0; idx<=startidx; idx++ )
	depths[idx] = times.value(idx) * prevvel;

    double depth = depths[startidx];
    for ( int idx=startidx+1; idx<velsz; idx++ )
    {
	double curvel = vels.value( idx );
	if ( scaler )
	    curvel = scaler->scale( curvel );

	if ( mIsValidVel(curvel) )
	    curvel /= 2.;
	else
	    curvel = prevvel;

	depth += (times.value(idx)-times.value(idx-1)) * curvel;

	depths[idx] = depth;
	prevvel = curvel;
    }

    return true;
}


/*!For every depth in the velocity model, calculate the time. The velocity is
   is assumed to be constant at vel[0] above the depth time. The velocity is
   specified as Interval velocity, and the sample span is above. That means
   the velocity at a certain sample is the velocity of the layer above the
   corresponding depth:

\code
Depth(m) Vel(m/s) Samplespan (m)	Time (s)
200	 1000	  160-200		200/1000 = 0.2
240	 1500	  200-240		0.2+40/1500 = 0.23
280	 2000	  240-280		0.23+40/2000 = 0.25

\endcode
*/

bool TimeDepthConverter::calcTimes( const ValueSeries<float>& vels, int velsz,
				    const SamplingData<double>& sd,
				    float* times, const Scaler* scaler )
{
    ArrayValueSeries<float,float> depths( velsz );
    float* depthsptr = depths.arr();
    for ( int idx=0; idx<velsz; idx++, depthsptr++ )
	*depthsptr = (float) sd.atIndex( idx );

    return calcTimes( vels, velsz, depths, times, scaler );
}


bool TimeDepthConverter::calcTimes( const ValueSeries<float>& vels, int velsz,
				    const ValueSeries<float>& depths,
				    float* times, const Scaler* scaler )
{
    if ( !times )
    {
	pFreeFnErrMsg( "No output pointer" );
	return false;
    }

    if ( !velsz )
	return true;

    float prevvel = 0;
    int startidx = -1;
    for ( int idx=0; idx<velsz; idx++ )
    {
	if ( !mIsValidVel(vels.value(idx)) )
	    continue;

	startidx = idx;
	prevvel = vels.value(idx);
	break;
    }

    if ( startidx==-1 )
	return false;

    if ( scaler )
	prevvel = scaler->scale( prevvel );

    for ( int idx=0; idx<startidx; idx++ )
    {
	float time = depths.value(idx) / prevvel;
	times[idx] = time;
    }

    double time = times[startidx] = depths.value(startidx) / prevvel;

    for ( int idx=startidx+1; idx<velsz; idx++ )
    {
	float curvel = vels.value( idx );
	if ( scaler )
	    curvel = scaler->scale( curvel );

	const double depth = depths.value(idx) - depths.value(idx-1);
	if ( !mIsValidVel(curvel) )
	{
	    curvel = prevvel;
	    if ( curvel>0 )
		time += depth*2/curvel;
	}
	else
	    time += depth*2/curvel; //time is TWT

	times[idx] = (float) time;
	prevvel = curvel;
    }

    return true;
}

mStopAllowDeprecatedSection
