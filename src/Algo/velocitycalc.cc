/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/


#include "velocitycalc.h"

#include "ailayer.h"
#include "bendpointfinder.h"
#include "genericnumer.h"
#include "idxable.h"
#include "interpol1d.h"
#include "limits.h"
#include "math2.h"
#include "scaler.h"
#include "survinfo.h"
#include "valseriesimpl.h"
#include "varlenarray.h"
#include "veldesc.h"

#include "uistrings.h"

mImplFactory( Vrms2Vint, Vrms2Vint::factory );


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
	    deleteAndZeroArrPtr( times_ );
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
    deleteAndZeroArrPtr( times_ );
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
{ return isOK() ? times_[0] : mUdf(float); }


float TimeDepthModel::getLastTime() const
{ return isOK() ? times_[sz_-1] : mUdf(float); }


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
	deleteAndZeroArrPtr( depths_ ) ;
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


// TimeDepthModelSet

TimeDepthModelSet::TimeDepthModelSet( const ElasticModel& emodel,
				      const TypeSet<float>* axisvals,
				      bool pup, bool pdown, float* velmax )
    : TimeDepthModelSet(emodel.size()+1,axisvals)
{
    if ( !isOK() )
	return;

    setFrom( emodel, pup, pdown, velmax );
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
	  (axisvals->size() == 1 && (!mIsZero(axisvals->first(),1e-4f)))) )
	axisvals_ = new TypeSet<float>( *axisvals );

    init( modelsz );
}


TimeDepthModelSet::~TimeDepthModelSet()
{
    if ( !axisvals_ || !tdmodels_.isPresent(defmodel_) )
	delete defmodel_;

    deepErase( tdmodels_ );
    delete axisvals_;
}


bool TimeDepthModelSet::isOK() const
{
    return defmodel_ && defmodel_->isOK() &&
	   (!axisvals_ || tdmodels_.size() == axisvals_->size() );
}


int TimeDepthModelSet::nrModels() const
{
    return axisvals_ ? tdmodels_.size() : 1;
}


int TimeDepthModelSet::modelSize() const
{
    return defmodel_->size();
}


const TimeDepthModel& TimeDepthModelSet::getDefaultModel() const
{
    return *defmodel_;
}


const TimeDepthModel* TimeDepthModelSet::get( int idx ) const
{
    return axisvals_ ? (tdmodels_.validIdx( idx ) ? tdmodels_.get( idx )
						  : nullptr)
		     : defmodel_;
}


void TimeDepthModelSet::init( int modelsz )
{
    mDeclareAndTryAlloc( float*, depths, float[modelsz] );
    if ( !depths )
	return;

    mDeclareAndTryAlloc( float*, times, float[modelsz] );
    if ( !times )
    {
	delete [] depths;
	return;
    }

    defmodel_ = new TimeDepthModel;
    defmodel_->setAllVals( depths, times, modelsz );
    if ( !axisvals_ )
	return;

    for ( const auto& xval : *axisvals_ )
    {
	if ( mIsZero(xval,1e-4f) )
	{
	    tdmodels_.add( defmodel_ );
	    continue;
	}

	mTryAlloc( times, float[modelsz] );
	if ( !times )
	    break;

	auto* tdmodel = new TimeDepthModel;
	tdmodel->setVals( defmodel_->getDepths(), true, false );
	tdmodel->setVals( times, false );
	tdmodel->setSize( modelsz );
	tdmodels_.add( tdmodel );
    }
}


void TimeDepthModelSet::setFrom( const ElasticModel& emodel, bool pup,
				 bool pdown, float* velmax )
{
    float* deptharr = defmodel_->getDepths();
    float* twtarr = defmodel_->getTimes();
    int idz = 0;
    deptharr[idz] = 0.f;
    twtarr[idz++] = 0.f;

    const bool zinfeet = SI().zInFeet();
    float dnmotime, dvrmssum, unmotime, uvrmssum;
    float prevdnmotime, prevdvrmssum, prevunmotime, prevuvrmssum;
    prevdnmotime = prevdvrmssum = prevunmotime = prevuvrmssum = 0;
    for ( const auto& layer : emodel )
    {
	const float dz = layer.thickness_;
	const float thickness = zinfeet ? dz * mToFeetFactorF : dz;
	deptharr[idz] = idz>1 ? deptharr[idz-1] + thickness : thickness;

	const float dvel = pdown ? layer.vel_ : layer.svel_;
	const float uvel = pup ? layer.vel_ : layer.svel_;
	dnmotime = dvrmssum = unmotime = uvrmssum = 0;

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
	if ( model != defmodel_ )
	    OD::sysMemValueSet( model->getTimes(), mUdf(float), sz );
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


// TimeDepthConverter

TimeDepthConverter::TimeDepthConverter()
    : TimeDepthModel()
{}


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


class MoveoutComputerError : public FloatMathFunction
{
public:
    MoveoutComputerError( const MoveoutComputer& m,float* variables,
	    int variabletochange, int nroffsets,
	    const float* offsets, const float* moveout )
	: calc_( m )
	, variables_( variables )
	, variable_( variabletochange )
	, nroffsets_( nroffsets )
	, realmoveouts_( moveout )
	, calcmoveouts_( new float[nroffsets] )
	, offsets_( offsets )
    {}

    ~MoveoutComputerError() { delete [] calcmoveouts_; }

    float		getValue( const float* pos ) const
			{ return getValue(*pos); }
    float		getValue(float val) const
    {
	variables_[variable_] = val;
	if ( !calc_.computeMoveout( variables_, nroffsets_, offsets_,
	      calcmoveouts_ ) )
	    return mUdf(float);

	float sqsum = 0;
	for ( int idx=0; idx<nroffsets_; idx++ )
	{
	    const float diff = calcmoveouts_[idx]-realmoveouts_[idx];
	    sqsum += diff*diff;
	}

	return sqsum;
    }


protected:
    const MoveoutComputer&	calc_;
    mutable float*		variables_;
    const int			variable_;
    int				nroffsets_;
    const float*		offsets_;
    const float*		realmoveouts_;
    mutable float*		calcmoveouts_;
};



float MoveoutComputer::findBestVariable( float* variables, int variabletochange,
	const Interval<float>& range, int nroffsets, const float* offsets,
	const float* moveout ) const
{
    MoveoutComputerError errorfunc(*this, variables, variabletochange,
	    nroffsets, offsets, moveout );
    const float res = findExtreme( errorfunc, true, range.start, range.stop );
    if ( mIsUdf(res) )
	return res;

    variables[variabletochange] = res;
    return Math::Sqrt( errorfunc.getValue( res ) );
}


bool RMOComputer::computeMoveout( const float* variables, int nroffsets,
	const float* offsets, float* res ) const
{
    return computeMoveout( variables[0], variables[1], variables[2],
	    nroffsets, offsets, res );
}


bool RMOComputer::computeMoveout( float d0, float rmo, float refoffset,
	int nroffsets, const float* offsets, float* res )
{
    for ( int idx=0; idx<nroffsets; idx++ )
    {
	float ratio = offsets[idx]/refoffset;
	ratio *= ratio;
	res[idx] = d0 + rmo * ratio;
    }

    return true;
}




bool NormalMoveout::computeMoveout( const float* variables, int nroffsets,
	const float* offsets, float* res ) const
{
    return computeMoveout( variables[0], variables[1], variables[2],
	    nroffsets, offsets, res );
}


bool NormalMoveout::computeMoveout( float t0, float Vrms,
	float effectiveanisotropy, int nroffsets, const float* offsets,
	float* res )
{
    const double t0_2 = t0*t0;
    const double v2 = Vrms*Vrms;

    if ( mIsUdf(effectiveanisotropy) )
	effectiveanisotropy = 0;

    const bool hasanisotropy = !mIsZero(effectiveanisotropy, 1e-3);

    for ( int idx=nroffsets-1; idx>=0; idx-- )
    {
	const double offset = offsets[idx];
	const double offset2 = offset*offset;

	double t2 = t0_2 + offset2/v2;
	if ( hasanisotropy )
	{
	    const double offset4 = offset2*offset2;
	    const double numerator = 2 * effectiveanisotropy * offset4;
	    const double denominator =
		v2*(t0_2*v2+(1+2*effectiveanisotropy)*offset2);

	    double anisotropycontrib = 0;
	    if ( denominator>0 )
		anisotropycontrib = numerator/denominator;

	    if ( anisotropycontrib<t2 )
		t2 -= anisotropycontrib;
	}

	if ( t2<=0 )
	    res[idx] = 0;
	else
	    res[idx] = (float) Math::Sqrt( t2 );
    }

    return true;
}


#define mComputeDixImpl( first_t, first_v, timefetch ) \
    if ( !Vrms || !Vint ) \
	return false; \
\
    if ( !nrvels ) \
	return true; \
 \
    int idx_prev = -1; \
    double t_above = -first_t; \
    double v2t_prev = t_above*first_v*first_v;; \
    double vlayer = mUdf(double);\
 \
    for ( int idx=0; idx<nrvels; idx++ ) \
    { \
	const double v = Vrms[idx]; \
	if ( mIsUdf(v) ) \
	    continue; \
 \
	double t_below = timefetch-first_t; \
 \
	const double v2t = t_below*v*v; \
	const double numerator = v2t-v2t_prev; \
	if ( numerator<0 ) \
	    continue; \
 \
	if ( t_below<t_above || mIsEqual(t_below,t_above,1e-5) ) \
	    continue; \
 \
	vlayer = Math::Sqrt( numerator/(t_below-t_above) ); \
 \
	for ( int idy=idx_prev+1; idy<=idx; idy++ ) \
	    Vint[idy] = mCast(float,vlayer); \
 \
	v2t_prev = v2t; \
	t_above = t_below; \
	idx_prev = idx; \
    } \
 \
    for ( int idx=idx_prev+1; idx<nrvels; idx++ )\
	Vint[idx] = mCast(float,vlayer);\
 \
    return true

bool computeDix( const float* Vrms, const SamplingData<double>& sd, int nrvels,
		 float* Vint )
{
    mComputeDixImpl( 0, 0, sd.atIndex(idx) );
}


bool computeDix( const float* Vrms, float t0, float v0, const float* t,
		 int nrvels, float* Vint )
{
    mAllocVarLenArr( double, tvals, nrvels );
    if ( !mIsVarLenArrOK(tvals) ) return false;
    for ( int idx=0; idx<nrvels; idx++ )
	tvals[idx] = t[idx];

    return computeDix( Vrms, (double)t0, v0, tvals, nrvels, Vint );
}


bool computeDix( const float* Vrms, double t0, float v0, const double* t,
		 int nrvels, float* Vint )
{
    mComputeDixImpl( t0, v0, t[idx] );
}


bool computeVrms( const float* Vint, const SamplingData<double>& sd, int nrvels,
		  float* Vrms )
{
    double t_above = 0;
    int idx_prev = -1;
    double v2t_prev = 0;

    for ( int idx=0; idx<nrvels; idx++ )
    {
	const double V_interval = Vint[idx];
	if ( mIsUdf(V_interval) )
	    continue;

	double t_below = sd.atIndex(idx);

	double dt = t_below - t_above;
	double numerator = v2t_prev+V_interval*V_interval*dt;
	float res = (float) Math::Sqrt( numerator/t_below );

	if ( !Math::IsNormalNumber(res) ) //looks for division by zero above
	    continue;

	for ( int idy=idx_prev+1; idy<=idx; idy++ )
	    Vrms[idy] = res;

	v2t_prev = numerator;
	t_above = t_below;
	idx_prev = idx;
    }

    return true;
}


bool computeVrms( const float* Vint, float t0, const float* t, int nrvels,
		  float* Vrms )
{
    mAllocVarLenArr( double, td_in, nrvels );
    if ( !mIsVarLenArrOK(td_in) ) return false;
    for ( int idx=0; idx<nrvels; idx++ )
	td_in[idx] = t[idx];

    return computeVrms( Vint, (double)t0, td_in, nrvels, Vrms );
}


bool computeVrms( const float* Vint, double t0, const double* t, int nrvels,
		  float* Vrms )
{
    double t_above = t0;
    int idx_prev = -1;
    double v2t_prev = 0;

    for ( int idx=0; idx<nrvels; idx++ )
    {
	const double V_interval = Vint[idx];
	if ( mIsUdf(V_interval) )
	    continue;

	double t_below = t[idx];

	double dt = t_below - t_above;
	double numerator = v2t_prev+V_interval*V_interval*dt;
	float res = (float) Math::Sqrt( numerator/(t_below-t0) );
	//TODO: Check whether t0 should be subtracted above

	for ( int idy=idx_prev+1; idy<=idx; idy++ )
	    Vrms[idy] = res;

	v2t_prev = numerator;
	t_above = t_below;
	idx_prev = idx;
    }

    return true;
}


bool sampleVrms( const float* Vin, float t0_in, float v0_in, const float* t_in,
		 int nr_in, const SamplingData<double>& sd_out, float* Vout,
		 int nr_out )
{
    mAllocVarLenArr( double, td_in, nr_in );
    if ( !mIsVarLenArrOK(td_in) ) return false;
    for ( int idx=0; idx<nr_in; idx++ )
	td_in[idx] = t_in[idx];

    return sampleVrms( Vin, (double)t0_in, v0_in, td_in, nr_in, sd_out, Vout,
		       nr_out );
}


bool sampleVrms( const float* Vin, double t0_in, float v0_in,const double* t_in,
		 int nr_in, const SamplingData<double>& sd_out, float* Vout,
		 int nr_out )
{
    if ( nr_out<=0 )
	return true;

    if ( nr_in<=0 )
	return false;

    if ( nr_in==1 )
    {
	for ( int idx=0; idx<nr_out; idx++ )
	    Vout[idx] = Vin[0];

	return true;
    }

    ArrayValueSeries<float,float> Vint ( nr_in );
    if ( !computeDix( Vin, t0_in, v0_in, t_in, nr_in, Vint.arr() ) )
	return false;

    const ArrayValueSeries<double,double> tinser( const_cast<double*>(t_in),
						  false );
    mAllocVarLenArr( double, deptharr, nr_in );
    mAllocVarLenArr( double, depthsampled, nr_out );
    mAllocVarLenArr( float, Vint_sampled, nr_out );
    if ( !tinser.isOK() || !mIsVarLenArrOK(deptharr) ||
	 !mIsVarLenArrOK(depthsampled) || !mIsVarLenArrOK(Vint_sampled) )
	return false;

    TimeDepthConverter::calcDepths( Vint, nr_in, tinser, deptharr );
    if ( nr_in<2 )
	return false;
    else
	resampleZ( deptharr, t_in, nr_in, sd_out, nr_out, depthsampled );

    //compute Vint_sampled from depthsampled
    Vint_sampled[0] = 0;
    const double halfinvstep = 2. / sd_out.step; //time is TWT
    for ( int idx=1; idx<nr_out; idx++ )
    {
	Vint_sampled[idx] = mCast(float, halfinvstep *
				(depthsampled[idx]-depthsampled[idx-1]) );
    }

    return computeVrms( (const float*)Vint_sampled, sd_out, nr_out, Vout );
}


bool computeVavg( const float* Vint, float z0, const float* z, int nrvels,
		  float* Vavg )
{
    mAllocVarLenArr( double, zvals, nrvels );
    if ( !mIsVarLenArrOK(zvals) ) return false;
    for ( int idx=0; idx<nrvels; idx++ )
	zvals[idx] = z[idx];

    return computeVavg( Vint, zvals, nrvels, Vavg );
}


bool computeVavg( const float* Vint, const double* z, int nrvels, float* Vavg )
{
    const bool zistime = SI().zIsTime();

    int idx_prev;
    for ( idx_prev=0; idx_prev<nrvels; idx_prev++ )
    {
	if ( !mIsUdf(Vint[idx_prev]) )
	    break;
    }
    if ( idx_prev == nrvels )
    {
	OD::sysMemValueSet( Vavg, mUdf(float), nrvels );
	return false;
    }

    Vavg[idx_prev] = Vint[idx_prev];
    double z_above = z[idx_prev];
    double v2z_prev = zistime ? Vavg[idx_prev] * z_above
			      : z_above / Vavg[idx_prev];

    for ( int idx=1; idx<nrvels; idx++ )
    {
	const double V_interval = Vint[idx];
	if ( mIsUdf(V_interval) )
	    continue;

	double z_below = z[idx];
	float res = 0;
	double dz = z_below - z_above;

	if ( zistime )
	{
	    double numerator = v2z_prev+V_interval*dz;
	    res = (float) ( numerator/z_below );

	    if ( !Math::IsNormalNumber(res) ) //looks for division by zero above
		continue;

	    v2z_prev = numerator;
	}
	else
	{
	    double denominator = v2z_prev + dz/V_interval;
	    res = (float) ( z_below / denominator );
	    if ( !Math::IsNormalNumber(res) ) //looks for division by zero above
		continue;

	    v2z_prev = denominator;
	}

	for ( int idy=idx_prev+1; idy<=idx; idy++ )
	    Vavg[idy] = res;

	z_above = z_below;
	idx_prev = idx;
    }

    return true;
}


bool computeVint( const float* Vavg, float z0, const float* z, int nrvels,
		  float* Vint )
{
    mAllocVarLenArr( double, zvals, nrvels );
    if ( !mIsVarLenArrOK(zvals) ) return false;
    for ( int idx=0; idx<nrvels; idx++ )
	zvals[idx] = z[idx];

    return computeVint( Vavg, zvals, nrvels, Vint );
}


bool computeVint( const float* Vavg, const double* z, int nrvels, float* Vint )
{
    const bool zistime = SI().zIsTime();

    int idx_prev;
    for ( idx_prev=0; idx_prev<nrvels; idx_prev++ )
    {
	if ( !mIsUdf(Vavg[idx_prev]) )
	    break;
    }
    if ( idx_prev == nrvels )
    {
	OD::sysMemValueSet( Vint, mUdf(float), nrvels );
	return false;
    }

    Vint[idx_prev] = Vavg[idx_prev];
    double z_above = z[idx_prev];
    double v2z_prev = zistime ? z_above*Vavg[idx_prev] : z_above/Vavg[idx_prev];
    bool hasvals = false;

    for ( int idx=1; idx<nrvels; idx++ )
    {
	const double v = Vavg[idx];
	if ( mIsUdf(v) )
	    continue;

	hasvals = true;

	double z_below = z[idx];

	const double v2z = zistime ? z_below*v : z_below/v;
	const double numerator = v2z-v2z_prev;
	if ( numerator<0 )
	    continue;

	if ( ( zistime && (z_below<z_above || mIsEqual(z_below,z_above,1e-5)) )
	   || ( !zistime && mIsZero(numerator,1e-3) ) )
	    continue;

	const double vlayer = zistime ? numerator/(z_below-z_above)
				      : (z_below-z_above)/numerator;

	for ( int idy=idx_prev+1; idy<=idx; idy++ )
	    Vint[idy] = (float) vlayer;

	v2z_prev = v2z;
	z_above = z_below;
	idx_prev = idx;
    }

    if ( !hasvals )
    {
	idx_prev = 0;
	Vint[0] = mUdf(float);
    }

    for ( int idx=idx_prev+1; idx<nrvels; idx++ )
	Vint[idx] = Vint[idx_prev];

    return true;
}


void resampleZ( const float* zarr, const float* tord_in, int nr_in,
		const SamplingData<double>& sd_out, int nr_out,float* zsampled )
{
    mAllocVarLenArr( double, dzarr, nr_in );
    mAllocVarLenArr( double, tordd_in, nr_in );
    mAllocVarLenArr( double, dzsampled, nr_out );
    if ( !mIsVarLenArrOK(dzarr) || !mIsVarLenArrOK(tordd_in) ||
	 !mIsVarLenArrOK(dzsampled) ) return;
    for ( int idx=0; idx<nr_in; idx++ )
    {
	dzarr[idx] = zarr[idx];
	tordd_in[idx] = tord_in[idx];
    }

    resampleContinuousData( dzarr, tordd_in, nr_in, sd_out, nr_out, dzsampled );
    for ( int idx=0; idx<nr_out; idx++ )
	zsampled[idx] = mCast(float,zsampled[idx]);
}


void resampleZ( const double* zarr, const double* tord_in, int nr_in,
		const SamplingData<double>& sd_out, int nr_out,double* zsampled)
{
    resampleContinuousData( zarr, tord_in, nr_in, sd_out, nr_out, zsampled );
}


void sampleEffectiveThomsenPars( const float* vinarr, const float* t_in,
				 int nr_in, const SamplingData<double>& sd_out,
				 int nr_out, float* voutarr )
{
    mAllocVarLenArr( double, td_in, nr_in );
    if ( !mIsVarLenArrOK(td_in) ) return;
    for ( int idx=0; idx<nr_in; idx++ )
	td_in[idx] = t_in[idx];

    sampleEffectiveThomsenPars( vinarr, td_in, nr_in, sd_out, nr_out, voutarr );
}


void sampleEffectiveThomsenPars( const float* vinarr, const double* t_in,
				 int nr_in, const SamplingData<double>& sd_out,
				 int nr_out, float* voutarr )
{
    mAllocVarLenArr( double, dvinarr, nr_in );
    mAllocVarLenArr( double, dvoutarr, nr_out );
    if ( !mIsVarLenArrOK(dvinarr) || !mIsVarLenArrOK(dvoutarr) ) return;
    resampleContinuousData( dvinarr, t_in, nr_in, sd_out, nr_out, dvoutarr );
    for ( int idx=0; idx<nr_out; idx++ )
	voutarr[idx] = mCast(float,dvoutarr[idx]);
}


void resampleContinuousData( const float* inarr, const float* t_in, int nr_in,
			     const SamplingData<double>& sd_out, int nr_out,
			     float* outarr )
{
    mAllocVarLenArr( double, dinarr, nr_in );
    mAllocVarLenArr( double, td_in, nr_in );
    mAllocVarLenArr( double, doutarr, nr_out );
    if ( !mIsVarLenArrOK(dinarr) || !mIsVarLenArrOK(td_in) ||
	 !mIsVarLenArrOK(doutarr) ) return;
    for ( int idx=0; idx<nr_in; idx++ )
    {
	dinarr[idx] = inarr[idx];
	td_in[idx] = t_in[idx];
    }

    resampleContinuousData( dinarr, td_in, nr_in, sd_out, nr_out, doutarr );
    for ( int idx=0; idx<nr_out; idx++ )
	outarr[idx] = mCast(float,doutarr[idx]);
}


void resampleContinuousData( const double* inarr, const double* t_in, int nr_in,
			     const SamplingData<double>& sd_out, int nr_out,
			     double* outarr )
{
    int intv = 0;
    const double eps = sd_out.step / 1e3;
    for ( int idx=0; idx<nr_out; idx++ )
    {
	bool match = false;
	const double z = sd_out.atIndex( idx );
	for ( ; intv<nr_in; intv++ )
	{
	    if ( mIsEqual(z,t_in[intv],eps) )
		match = true;
	    else if ( t_in[intv]<=z )
		continue;

	    break;
	}

	//intv is always after pos
	if ( match )
	    outarr[idx] = inarr[intv];
	else
	{
	    if ( intv == nr_in )
		intv--;

	    const double v0 = !intv? 0. : inarr[intv-1];
	    const double v1 = inarr[intv];
	    const float t0 = mCast(float,!intv? 0. : t_in[intv-1]);
	    const float t1 = mCast(float,t_in[intv]);
	    outarr[idx] = mIsEqual( v0, v1, 1e-6 ) ? v0 :
			Interpolate::linear1D( t0, v0, t1, v1, mCast(float,z) );
	}
    }
}


bool sampleVint( const float* Vin, const float* t_in, int nr_in,
		 const SamplingData<double>& sd_out, float* Vout, int nr_out )
{
    mAllocVarLenArr( double, td_in, nr_in );
    if ( !mIsVarLenArrOK(td_in) ) return false;
    for ( int idx=0; idx<nr_in; idx++ )
	td_in[idx] = t_in[idx];

    return sampleVint( Vin, td_in, nr_in, sd_out, Vout, nr_out );
}


bool sampleVint( const float* Vin, const double* t_in, int nr_in,
		 const SamplingData<double>& sd_out, float* Vout, int nr_out )
{
    if ( !nr_in )
	return false;

    const ArrayValueSeries<double,double> tinser( const_cast<double*>(t_in),
						  false );
    const ArrayValueSeries<float,float> Vinser( const_cast<float*>(Vin),
						false );
    mAllocVarLenArr( double, deptharr, nr_in );
    mAllocVarLenArr( double, depthsampled, nr_out );
    if ( !tinser.isOK() || !Vinser.isOK() ||
	 !mIsVarLenArrOK(deptharr) || !mIsVarLenArrOK(depthsampled) )
	return false;

    TimeDepthConverter::calcDepths( Vinser, nr_in, tinser, deptharr );
    if ( nr_in<2 )
	return false;
    else
	resampleZ( deptharr, t_in, nr_in, sd_out, nr_out, depthsampled );

    //compute Vout from depthsampled
    Vout[0] = Vin[0];
    const double halfinvstep = 2. / sd_out.step; //time is TWT
    for ( int idx=1; idx<nr_out; idx++ )
    {
	Vout[idx] = mCast(float, halfinvstep *
				 (depthsampled[idx] - depthsampled[idx-1]) );
    }

    return true;
}


bool sampleVavg( const float* Vin, const float* t_in, int nr_in,
		 const SamplingData<double>& sd_out, float* Vout, int nr_out )
{
    mAllocVarLenArr( double, td_in, nr_in );
    if ( !mIsVarLenArrOK(td_in) ) return false;
    for ( int idx=0; idx<nr_in; idx++ )
	td_in[idx] = t_in[idx];

    return sampleVavg( Vin, td_in, nr_in, sd_out, Vout, nr_out );
}


bool sampleVavg( const float* Vin, const double* t_in, int nr_in,
		 const SamplingData<double>& sd_out, float* Vout, int nr_out )
{
    mAllocVarLenArr( float, vintarr, nr_in );
    mAllocVarLenArr( float, vintsampledarr, nr_out );
    mAllocVarLenArr( double, outtimesampsarr, nr_out );
    if ( !mIsVarLenArrOK(vintarr) || !mIsVarLenArrOK(vintsampledarr) ||
	 !mIsVarLenArrOK(outtimesampsarr) )
	return false;

    for ( int idx=0; idx<nr_out; idx++ )
	outtimesampsarr[idx] = sd_out.atIndex( idx );

    return computeVint( Vin, t_in, nr_in, vintarr ) &&
	sampleVint( vintarr,t_in,nr_in, sd_out, vintsampledarr, nr_out ) &&
	computeVavg( vintsampledarr, outtimesampsarr, nr_out, Vout );
}


void computeResidualMoveouts( float z0, float rmo, float refoffset,
			      int nroffsets, bool outputdepth,
			      const float* offsets, float* output )
{
    const bool alludf =
	    (outputdepth && mIsUdf(z0)) || mIsUdf(rmo) || mIsUdf(refoffset) ||
	    mIsZero(refoffset,1e-5);

    for ( int idx=0; idx<nroffsets; idx++, offsets++, output++ )
    {
	if ( alludf )
	{
	    *output = mUdf(float);
	    continue;
	}

	const float offset = *offsets;
	if ( mIsUdf(offset) )
	{
	    *output = mUdf(float);
	    continue;
	}

	const float ratio = offset/refoffset;
	*output = rmo * ratio * ratio;
	if ( outputdepth )
	    *output += z0;
    }
}

bool fitLinearVelocity( const float* vint, const float* zin, int nr,
	const Interval<float>& zlayer, float refz, bool zisdepth,
	float& v0, float& gradient,  float& error )
{
    if ( nr < 2 )
	return false;

    mAllocVarLenArr( float, tmparr, nr );
    if ( !mIsVarLenArrOK(tmparr) ) return false;
    const ArrayValueSeries<float,float> inputvels( (float*)vint, false, nr );
    const ArrayValueSeries<float,float> inputzs( (float*)zin, false, nr );

mStartAllowDeprecatedSection
    if ( zisdepth )
    {
	TimeDepthConverter::calcTimes( inputvels, nr, inputzs, tmparr );
    }
    else
    {
	TimeDepthConverter::calcDepths( inputvels, nr, inputzs, tmparr );
    }
mStopAllowDeprecatedSection

    const float* depths = zisdepth ? zin : tmparr;
    const float* times = zisdepth ? tmparr : zin;

    // Get boundary pairs
    float d[2], t[2], vt[2];
    for ( int idx=0; idx<2; idx++ )
    {
	if ( zisdepth )
	{
	    d[idx] = !idx ? zlayer.start : zlayer.stop;
	    if ( d[idx] <= depths[0] )
	    {
		for ( int idy=0; idy<nr; idy++ )
		{
		    if ( mIsUdf(vint[idy]) ) continue;
		    t[idx] = times[idy] - ( depths[idy] - d[idx] ) / vint[idy];
		    vt[idx] = vint[idy];
		    break;
		}
	    }
	    else if ( d[idx] >= depths[nr-1] )
	    {
		for ( int idy=nr-1; idy>=0; idy-- )
		{
		    if ( mIsUdf(vint[idy]) ) continue;
		    t[idx] = times[idy] + ( d[idx] - depths[idy] ) / vint[idy];
		    vt[idx] = vint[idy];
		    break;
		}
	    }
	    else
	    {
		for ( int idy=0; idy<nr-1; idy++ )
		{
		    if ( depths[idy] <= d[idx] && d[idx] <= depths[idy+1] )
		    {
			t[idx] = times[idy] + (times[idy+1] - times[idy]) /
			    (depths[idy+1]-depths[idy]) * (d[idx]-depths[idy]);
			vt[idx] = vint[idy];
			break;
		    }
		}
	    }
	}
	else
	{
	    t[idx] = !idx ? zlayer.start : zlayer.stop;
	    if ( t[idx] <= times[0] )
	    {
		for ( int idy=0; idy<nr; idy++ )
		{
		    if ( mIsUdf(vint[idy]) ) continue;
		    d[idx] = depths[idy] - ( times[idy] - t[idx] ) * vint[idy];
		    vt[idx] = vint[idy];
		    break;
		}
	    }
	    else if ( t[idx] >= times[nr-1] )
	    {
		for ( int idy=nr-1; idy>=0; idy-- )
		{
		    if ( mIsUdf(vint[idy]) ) continue;
		    d[idx] = depths[idy] + ( t[idx] - times[idy] ) * vint[idy];
		    vt[idx] = vint[idy];
		    break;
		}
	    }
	    else
	    {
		for ( int idy=0; idy<nr-1; idy++ )
		{
		    if ( times[idy] <= t[idx] && t[idx] <= times[idy+1] )
		    {
			d[idx] = depths[idy] + (depths[idy+1]-depths[idy]) /
			    (times[idy+1] - times[idy]) * (t[idx] - times[idy]);
			vt[idx] = vint[idy];
			break;
		    }
		}
	    }
	}
    }

    if ( mIsUdf(d[0]) || mIsUdf(d[1]) || mIsUdf(t[0]) || mIsUdf(t[1]) )
	return false;

    const float v = (d[1] - d[0]) / (t[1] - t[0]);
    const float diff = (t[1] + t[0] - 2 * refz) * 0.5f;

    TypeSet<int> indices;
    for ( int idx=0; idx<nr; idx++ )
    {
	if ( zlayer.includes(zin[idx],false) )
	    indices += idx;
    }
    const int nrsmps = indices.size();
    if ( !nrsmps )
    {
	gradient = zisdepth ? (vt[1] - vt[0]) / (d[1] - d[0])
			    : (vt[1] - vt[0]) / (t[1] - t[0]);
	v0 = zisdepth ? vt[1] - gradient * (d[1] - refz)
		      : vt[1] - gradient * (t[1] - refz);
	return true;
    }

    error = 0;
    if ( zisdepth )
    {
	const StepInterval<float> gradrg(-2, 2, 0.01);//Make your range
	const int nrsteps = (int)(gradrg.width() / gradrg.step);

	const float d10 = d[1] - d[0];
	const float t10 = t[1] - t[0];
	const float d1z = d[1] - refz;
	for ( int idx=0; idx<nrsteps; idx++ )
	{
	    const float grad = gradrg.atIndex( idx );
	    const float v_est = grad * (d10 / (Math::Exp(grad*t10)-1) - d1z);

	    float errsum = 0;
	    for ( int idy=0; idy<nrsmps; idy++ )
	    {
		const float epsilon = v_est - vint[indices[idy]];
		errsum += epsilon * epsilon;
	    }

	    if ( !idx || error > errsum )
	    {
		error = errsum;
		gradient = grad;
		v0 = v_est;
	    }
	}
    }
    else
    {
	float sum_v0 = 0, sum_grad = 0, sum_vg = 0, sum_v0sqr = 0;
	for ( int idx=0; idx<nrsmps; idx++ )
	{
	    const float grad = (v-(depths[indices[idx]]-d[0]) /
		    (times[indices[idx]]-t[0]))*2 / (t[1]-times[indices[idx]]);
	    const float ve = v - grad * diff;
	    const float epsilon = vint[indices[idx]] -
		(ve + grad * (times[indices[idx]] - t[0]));

	    sum_v0 += ve;
	    sum_grad += grad;
	    sum_vg = ve * grad;
	    sum_v0sqr += ve * ve;
	    error += epsilon * epsilon;
	}

	gradient = nrsmps==1 ? sum_grad :
	    (sum_vg - sum_v0 * sum_grad) / (sum_v0sqr - sum_v0 * sum_v0);
	v0 = v - gradient * diff;
    }

    return true;
}


void sampleIntvThomsenPars( const float* inarr, const float* t_in, int nr_in,
		    const SamplingData<double>& sd_out, int nr_out,
		    float* outarr )
{
    mAllocVarLenArr( double, td_in, nr_in );
    if ( !mIsVarLenArrOK(td_in) ) return;
    for ( int idx=0; idx<nr_in; idx++ )
	td_in[idx] = t_in[idx];

    sampleIntvThomsenPars( inarr, td_in, nr_in, sd_out, nr_out, outarr );
}


void sampleIntvThomsenPars( const float* inarr, const double* t_in, int nr_in,
			    const SamplingData<double>& sd_out, int nr_out,
			    float* outarr )
{
    int intv = 0;
    for ( int idx=0; idx<nr_out; idx++ )
    {
	const double z = sd_out.atIndex( idx );
	for ( ; intv<nr_in; intv++ )
	{
	    if ( t_in[intv]<z )
		continue;

	    break;
	}

	//intv is always after pos
	if ( intv == nr_in )
	    intv--;

	outarr[idx] = inarr[intv];
    }
}


#define mSmallNumber 1e-7


bool computeLinearT2D( double v0, double dv, double v0depth,
		       const SamplingData<float>& sd,
		       int sz, float* res )
{
    if ( sz<=0 )
	return true;

    if ( v0<=0 || mIsUdf(v0) )
	return false;

    const bool zerogradient = mIsZero( dv, 1e-6 ) || mIsUdf(dv);

    if ( zerogradient )
	dv = mSmallNumber;

    const double fact = dv / v0;
    const double dv2 = 2. / dv;

    const double logval = Math::Log( 1. + v0depth * fact );
    if ( mIsUdf(logval) )
	return false;

    const double sealeveltwt = - dv2 * logval;

    for ( int idx=0; idx<sz; idx++ )
    {
	const double time = (sd.start+idx*sd.step-sealeveltwt) / 2.;
	const double expterm = Math::Exp( dv * time );

	if ( mIsUdf(expterm) )
	{
	    res[idx] = mUdf(float);
	    continue;
	}

	const double depth = ( expterm - 1. ) / fact;

	if ( depth>MAXFLOAT )
	    res[idx] = mUdf(float);
	else
	    res[idx] = mCast(float,depth);
    }

    return true;
}


bool computeLinearD2T( double v0, double dv, double v0depth,
		      const SamplingData<float>& sd,
		      int sz, float* res )
{
    if ( sz < 0 )
	return true;

    if ( v0<=0 || mIsUdf(v0) )
	return false;

    const bool zerogradient = mIsZero( dv, 1e-6 ) || mIsUdf(dv);

    if ( zerogradient )
	dv = mSmallNumber;

    const double fact = dv / v0;
    const double dv2 = 2. / dv;
    double logval = Math::Log( 1. + v0depth * fact );
    if ( mIsUdf(logval) )
	return false;

    const double sealeveltwt = - dv2 * logval;

    for ( int idx=0; idx<sz; idx++ )
    {
	const double depth = sd.start + idx*sd.step;
	logval = Math::Log(1.+depth*fact);
	if ( mIsUdf(logval) )
	    res[idx] = mUdf(float);
	else
	    res[idx] = mCast(float,dv2 * logval+sealeveltwt);
    }

    return true;
}


bool convertToVintIfNeeded( const float* inpvel, const VelocityDesc& veldesc,
			    const StepInterval<float>& zrange, float* outvel )
{
    const int nrzvals = zrange.nrSteps() + 1;
    const SamplingData<double> sd = getDoubleSamplingData(
						SamplingData<float>( zrange ) );
    if ( veldesc.type_ == VelocityDesc::Avg )
    {
	mAllocVarLenArr( double, zvals, nrzvals );
	if ( !mIsVarLenArrOK(zvals) ) return false;
	for ( int idx=0; idx<nrzvals; idx++ )
	    zvals[idx] = sd.atIndex( idx );

	if ( !computeVint(inpvel,zvals,nrzvals,outvel) )
	    return false;
    }
    else if ( veldesc.type_ == VelocityDesc::RMS &&
	      !computeDix(inpvel,sd,nrzvals,outvel) )
	      return false;
    else if ( veldesc.type_ == VelocityDesc::Interval )
    {
	OD::sysMemCopy( outvel, inpvel, nrzvals*sizeof(float) );
    }

    return true;
}


SamplingData<double> getDoubleSamplingData( const SamplingData<float>& sdf )
{
    SamplingData<double> ret( sdf.start, sdf.step );
    if ( sdf.step > 0.7 )
	return ret;

    float nrsamplesf = 1.f / sdf.step;
    int nrsamples = mNINT32( nrsamplesf );
    float relpos = nrsamplesf - nrsamples;
    if ( Math::Abs(relpos) > nrsamplesf*1e-4f )
	return ret;

    ret.step = 1. / mCast(double,nrsamples);

    nrsamplesf = mCast(float,ret.start / ret.step);
    nrsamples = mNINT32( nrsamplesf );
    relpos = nrsamplesf - nrsamples;
    if ( Math::Abs(relpos) > nrsamplesf*1e-4f )
	return ret;

    ret.start = ret.step * mCast(double, nrsamples);

    return ret;
}
