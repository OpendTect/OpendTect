/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocitycalc.h"

#include "genericnumer.h"
#include "survinfo.h"
#include "timedepthmodel.h"
#include "valseriesimpl.h"
#include "veldesc.h"
#include "uistrings.h"

mImplFactory( Vrms2Vint, Vrms2Vint::factory );


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
    float		getValue(float val) const override
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
    const double v2 = (double)Vrms * (double)Vrms;

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
    float d[2]{mUdf(float),mUdf(float)};
    float t[2]{mUdf(float),mUdf(float)};
    float vt[2]{mUdf(float),mUdf(float)};
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
