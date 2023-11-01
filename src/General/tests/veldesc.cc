/*+
________________________________________________________________________

 Copyright:	(C) 1995-2023 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "arrayndimpl.h"
#include "unitofmeasure.h"
#include "veldescimpl.h"
#include "velocitycalc.h"
#include "zvalseriesimpl.h"

using namespace OD;

static BufferString msg_;

static double srd_ = 15.24;

static int modsz_ = 4;
static const double modvint[] = { 1960., 2175., 2260., 2400. };
static const double modvavg[] = { 1960., 1977.3801507516289,
				  2076.2825995719777, 2117.7742349340911 };
static const double modvrms[] = { 1960., 1978.24844833959,
				  2081.19010209987, 2124.72714711438 };
static const double modtimes[] = { 0.5730111, 0.623405933,
				   0.959010362625, 1.1 };
static const double moddepths[] = { 546.310878, 601.1152588875,
				    980.34826436375, 1149.53582921375 };

static const double t0[] = { 0., 6., 0., 6., 0., 6., 0., 6. };
static const double z0[] = { 0., 6000., -200., 5800.,
			     0., 6997.176152, -200., 6797.176152 };


#define mCheckVal(val,expected,idx) \
{ \
    if ( float(val) != float(expected) ) \
    { \
	msg_.set( "Invalid computed value: " ) \
	    .add( "Expected: " ).add( toStringPrecise( float(expected) ) ) \
	    .add( "; Computed: " ).add( toStringPrecise( float(val) ) ) \
	    .add( " at idx " ).add( idx ); \
	if ( zistime ) msg_.add( " (sampling in time)" ); \
	else if ( zinfeet ) msg_.add( " (sampling in depth-Feet)" ); \
	else msg_.add( " (sampling in depth-Meter)" ); \
	\
	return false; \
    } \
}


static bool checkSampledVintVals( const ValueSeries<double>& vels,
			      bool velinfeetsec, const ZDomain::Info& zinfo )
{
    const bool zistime = zinfo.isTime();
    const bool zinfeet = zinfo.isDepthFeet();
    const od_int64 idx1 = zistime ?  44 : (zinfeet ?  40 :  30);
    const od_int64 idx2 = zistime ?  56 : (zinfeet ?  52 :  41);
    const od_int64 idx3 = zistime ? 140 : (zinfeet ? 135 : 117);
    const double val1 = zistime ? 2120.65338
				: (zinfeet ? 2063.93485 : 2114.19767 );
    const double val2 = zistime ? 2187.62392
				: (zinfeet ? 2218.5965 : 2240.4699 );
    const double val3 = zistime ? 2294.63731
				: (zinfeet ? 2338.52255 : 2389.68902 );
    const double fact = velinfeetsec ? mToFeetFactorD : 1.;
    for ( od_int64 idx=0; idx<idx1-1; idx++ )
	{ mCheckVal( vels[idx], modvint[0]*fact, idx ) }
    { mCheckVal( vels[idx1], val1*fact, idx1 ) }
    for ( od_int64 idx=idx1+1; idx<idx2-1; idx++ )
	{ mCheckVal( vels[idx], modvint[1]*fact, idx  ) }
    { mCheckVal( vels[idx2], val2*fact, idx2 ) }
    for ( od_int64 idx=idx2+1; idx<idx3-1; idx++ )
	{ mCheckVal( vels[idx], modvint[2]*fact, idx ) }
    { mCheckVal( vels[idx3], val3*fact, idx3 ) }
    for ( od_int64 idx=idx3+1; idx<vels.size(); idx++ )
	{ mCheckVal( vels[idx], modvint[3]*fact, idx ) }

    return true;
}


static bool checkSampledVavgVals( const ValueSeries<double>& vels,
			      bool velinfeetsec, const ZDomain::Info& zinfo )
{
    const bool zistime = zinfo.isTime();
    const bool zinfeet = zinfo.isDepthFeet();
    const od_int64 idx1 = zistime ?  44 : (zinfeet ?  40 :  30);
    const od_int64 idx2 = zistime ?  56 : (zinfeet ?  52 :  41);
    const od_int64 idx3 = zistime ? 140 : (zinfeet ? 135 : 117);
    const od_int64 idx4 = vels.size()-1;
    const double val1 = zistime ? 1961.1156484
			       : (zinfeet ? 1960.8006048 : 1961.26533547 );
    const double val2 = zistime ? 1977.6492123
			       : (zinfeet ? 1978.3352581 : 1978.93013396 );
    const double val3 = zistime ? 2076.6163109
			       : (zinfeet ? 2077.0212037 : 2077.58584013 );
    const double val4 = zistime ? 2117.7742349
			       : (zinfeet ? 2127.7422055 : 2128.16654545 );
    const double fact = velinfeetsec ? mToFeetFactorD : 1.;
    { mCheckVal( vels[0], modvavg[0]*fact, 0 ) }
    { mCheckVal( vels[idx1], val1*fact, idx1 ) }
    { mCheckVal( vels[idx2], val2*fact, idx2 ) }
    { mCheckVal( vels[idx3], val3*fact, idx3 ) }
    { mCheckVal( vels[idx4], val4*fact, idx4 ) }

    return true;
}

static bool checkSampledVrmsVals( const ValueSeries<double>& vels,
			      bool velinfeetsec, const ZDomain::Info& zinfo )
{
    const bool zistime = zinfo.isTime();
    const bool zinfeet = zinfo.isDepthFeet();
    const od_int64 idx1 = 44;
    const od_int64 idx2 = 56;
    const od_int64 idx3 = 140;
    const od_int64 idx4 = vels.size()-1;
    const double val1 = 1961.16102718818;
    const double val2 = 1978.52014136807;
    const double val3 = 2081.53059124019;
    const double val4 = 2124.71577097610;
    const double fact = velinfeetsec ? mToFeetFactorD : 1.;
    { mCheckVal( vels[0], modvrms[0]*fact, 0 ) }
    { mCheckVal( vels[idx1], val1*fact, idx1 ) }
    { mCheckVal( vels[idx2], val2*fact, idx2 ) }
    { mCheckVal( vels[idx3], val3*fact, idx3 ) }
    { mCheckVal( vels[idx4], val4*fact, idx4 ) }

    return true;
}


static bool checkSampledZVals( const ValueSeries<double>& zvals,
			       bool velinfeetsec, const ZDomain::Info& zinfo,
			       const ZDomain::Info& zinfo_out )
{
    const bool zistime = zinfo.isTime();
    const bool zinfeet = zinfo.isDepthFeet();
    const double zfact = zinfo_out.isDepthFeet() ? mToFeetFactorD : 1.;
    const od_int64 idx0 = 0;
    const od_int64 idx1 = zistime ?  44 : (zinfeet ?  40 :  30);
    const od_int64 idx2 = zistime ?  56 : (zinfeet ?  52 :  41);
    const od_int64 idx3 = zistime ? 140 : (zinfeet ? 135 : 117);
    const od_int64 idx4 = zvals.size()-1;
    const double val0 = zistime ? 376.76 * zfact
				: (zinfeet ? 0.3887755102 : 0.423714285714 );
    const double val1 = zistime ? 549.56130675 * zfact
				: (zinfeet ? 0.5751528213 : 0.57640339609 );
    const double val2 = zistime ? 601.7865545975 * zfact
				: (zinfeet ? 0.625519863 : 0.6268437569);
    const double val3 = zistime ? 981.53582921375 * zfact
				: (zinfeet ? 0.961203475 : 0.9628868089 );
    const double val4 = zistime ? 1149.5358292 * zfact
				: (zinfeet ? 1.1402734756: 1.142053475);
    const double fact = zistime && zinfeet ? mToFeetFactorD : 1.;
    { mCheckVal( zvals[idx0], val0*fact, idx0 ) }
    { mCheckVal( zvals[idx1], val1*fact, idx1 ) }
    { mCheckVal( zvals[idx2], val2*fact, idx2 ) }
    { mCheckVal( zvals[idx3], val3*fact, idx3 ) }
    { mCheckVal( zvals[idx4], val4*fact, idx4 ) }

    return true;
}


#undef mCheckVal
#define mCheckVal(val,expected,idx) \
{ \
    if ( float(val) != float(expected) ) \
    { \
	msg_.set( "Invalid computed value: " ) \
	    .add( "Expected: " ).add( toStringPrecise(float(expected)) ) \
	    .add( "; Computed: " ).add( toStringPrecise(float(val)) ) \
	    .add( " at idx " ).add( idx ).add( "; ") \
	    .add( toString(typein) ).add( " to " ) \
	    .add( toString(typeout) ); \
	if ( zistime ) msg_.add( " (sampling in time)" ); \
	else if ( zinfeet ) msg_.add( " (sampling in depth-Feet)" ); \
	else msg_.add( " (sampling in depth-Meter)" ); \
	\
	return false; \
    } \
}

static bool checkConvertedVels( const ValueSeries<double>& vels,
				const double* velsqc,
				VelocityType typein, VelocityType typeout,
				const ZDomain::Info& zinfo )

{
    const bool zistime = zinfo.isTime();
    const bool zinfeet = zinfo.isDepthFeet();
    const od_int64 sz = vels.size();
    for ( od_int64 idx=0; idx<sz; idx++ )
    {
	mCheckVal( vels[idx], velsqc[idx], idx );
    }

    return true;
}


#undef mCheckVal
#define mCheckVal(val,expected,idx) \
{ \
    if ( float(val) != float(expected) ) \
    { \
	msg_.set( "Invalid computed value: " ) \
	    .add( "Expected: " ).add( toStringPrecise(float(expected)) ) \
	    .add( "; Computed: " ).add( toStringPrecise(float(val)) ) \
	    .add( " at idx " ).add( idx ).add( "; "); \
	if ( zistime ) msg_.add( " (sampling in time)" ); \
	else if ( zinfeet ) msg_.add( " (sampling in depth-Feet)" ); \
	else msg_.add( " (sampling in depth-Meter)" ); \
	\
	return false; \
    } \
}

static bool checkConvertedLinearVels( const double* calczarr,
				      const double* expectedzarr, int sz,
				      const ZDomain::Info& zinfo )
{
    const bool zistime = zinfo.isTime();
    const bool zinfeet = zinfo.isDepthFeet();
    for ( int idx=0; idx<sz; idx++ )
    {
	mCheckVal( calczarr[idx], expectedzarr[idx], idx );
    }

    return true;
}


#undef mCheckVal

static void setDescUnit( bool velinfeetsec, VelocityDesc& desc )
{
    const UnitOfMeasure* veluom = velinfeetsec
				? UnitOfMeasure::feetSecondUnit()
				: UnitOfMeasure::meterSecondUnit();
    Vel::Worker::setUnit( veluom, desc );
}


/*Here we create data like if it would be coming from storage
  (Velocity functions, seismic traces, ...) */
static void getVelocities( const VelocityDesc& desc, TypeSet<double>& modvels )
{
    const bool velinfeetsec =
		Vel::Worker::getUnit( desc ) == UnitOfMeasure::feetSecondUnit();
    const bool isinterval = desc.isInterval();
    const bool isaverage = desc.isAvg();
    const double* vels = isinterval
		       ? modvint : (isaverage  ? modvavg : modvrms );
    for ( int idx=0; idx<modsz_; idx++ )
    {
	double vel = vels[idx];
	if ( velinfeetsec )
	    vel *= mToFeetFactorD;

	modvels += vel;
    }
}


static void getZVals( const ZDomain::Info& zinfo, TypeSet<double>& zvals )
{
    for ( int idx=0; idx<modsz_; idx++ )
    {
	double zval = zinfo.isTime() ? modtimes[idx] : moddepths[idx];
	if ( zinfo.isDepthFeet() )
	    zval *= mToFeetFactorD;
	zvals += zval;
    }
}


static bool testVelConvs( VelocityType typein, VelocityType typeout,
			  const ZDomain::Info& zinfo,
			  const bool* velinfeetsec =nullptr )
{
    const bool velinftsec = velinfeetsec ? !(*velinfeetsec)
					 : zinfo.isDepthFeet();
    VelocityDesc descin, descout;
    descin.setType( typein );
    descout.setType( typeout );
    setDescUnit( velinfeetsec, descin );
    setDescUnit( velinfeetsec, descout );

    TypeSet<double> inpvels, outvelsqc, zvals;
    getVelocities( descin, inpvels );
    getZVals( zinfo, zvals );
    getVelocities( descout, outvelsqc );

    const ArrayValueSeries<double,double> Vin( inpvels.arr(), false,
					       inpvels.size() );
    const ArrayZValues<double> zvalsarr( zvals, zinfo );
    ArrayValueSeries<double,double> Vout( Vin.size() );

    BufferString suffix( " with model in ",
			 zinfo.isTime() ? "time"
			 : (zinfo.isDepthMeter()
			     ? "depth-meter" : "depth-feet") );
    suffix.add(" and velocities in ")
	  .add( Vel::Worker::getUnit( descin )->symbol() );

    const Vel::Worker worker( descin, srd_, UnitOfMeasure::meterUnit() );
    mRunStandardTest( worker.convertVelocities( Vin, zvalsarr, descout, Vout ),
		      BufferString("Convert velocity values",suffix) );

    mRunStandardTestWithError( checkConvertedVels(Vout,outvelsqc.arr(),
						  typein,typeout,zinfo),
					"Converted velocity values", msg_ );

    return velinfeetsec ? true
	: testVelConvs( typein, typeout, zinfo, &velinftsec );
}


static bool testSampleVels( VelocityDesc& desc, const ZDomain::Info& modzinfo,
			    const ZSampling& zsamp, const ZDomain::Info& zinfo,
			    const bool* velinfeetsec =nullptr )
{
    const bool velinftsec = velinfeetsec ? !(*velinfeetsec)
					 : modzinfo.isDepthFeet();
    setDescUnit( velinftsec, desc );

    TypeSet<double> modvels, zvals;
    getVelocities( desc, modvels );
    getZVals( modzinfo, zvals );

    const ArrayValueSeries<double,double> Vin( modvels.arr(), false,
					       modvels.size() );
    const ArrayZValues<double> zvals_in( zvals, modzinfo );
    const RegularZValues zvals_out( zsamp, zinfo );
    ArrayValueSeries<double,double> Vout( zvals_out.size() );

    BufferString suffix( " with model in ",
			 modzinfo.isTime() ? "time"
			 : (modzinfo.isDepthMeter() ? "depth-meter"
						    : "depth-feet") );
    suffix.add( "; within project in " )
	  .add( zinfo.isTime() ? "time"
		  : (zinfo.isDepthMeter() ? "depth-meter"
					  : "depth-feet") )
	  .add( " and velocities in " )
	  .add( Vel::Worker::getUnit( desc )->symbol() );

    const Vel::Worker worker( desc, srd_, UnitOfMeasure::meterUnit() );
    mRunStandardTest( worker.sampleVelocities( Vin, zvals_in, zvals_out, Vout ),
		      BufferString("Sample velocities",suffix) );

    if ( desc.isInterval() )
    {
	mRunStandardTestWithError(
				checkSampledVintVals(Vout,velinftsec,zinfo),
			       "Sampled Vint values", msg_ );
    }
    else if ( desc.isAvg() )
    {
	mRunStandardTestWithError(
				checkSampledVavgVals(Vout,velinftsec,zinfo),
			       "Sampled Vavg values", msg_ );
    }
    else if ( desc.isRMS() )
    {
	mRunStandardTestWithError(
				checkSampledVrmsVals(Vout,velinftsec,zinfo),
			       "Sampled Vrms values", msg_ );
    }

    return velinfeetsec ? true
		: testSampleVels( desc, modzinfo, zsamp, zinfo, &velinftsec );
}


static bool testCalcZ( VelocityDesc& desc, const ZDomain::Info& modzinfo,
		       const ZSampling& zsamp, const ZDomain::Info& zinfo,
		       const bool* velinfeetsec =nullptr )
{
    const bool velinftsec = velinfeetsec ? !(*velinfeetsec)
					 : modzinfo.isDepthFeet();
    setDescUnit( velinftsec, desc );

    TypeSet<double> modvels, modzvals, zvals_qc;
    getVelocities( desc, modvels );
    getZVals( modzinfo, modzvals );
    getZVals( zinfo, zvals_qc );

    const ArrayValueSeries<double,double> Vin( modvels.arr(), false,
					       modvels.size() );
    const ArrayZValues<double> zvals_in( modzvals, modzinfo );
    const RegularZValues zvals_out( zsamp, zinfo );
    const ZDomain::Info& zinfo_trans = zinfo.isTime() ? ZDomain::DepthFeet()
						      : ZDomain::TWT();
    ArrayValueSeries<double,double> Zout_src( zvals_out.size() );
    ArrayZValues<double> Zout( Zout_src.arr(), Zout_src.size(), zinfo_trans );

    const Vel::Worker worker( desc, srd_, UnitOfMeasure::meterUnit() );
    mRunStandardTest( worker.getSampledZ( Vin, zvals_in, zvals_out, Zout ),
	    zinfo.isTime() ? "Compute depth values" : "Compute time values" );

    mRunStandardTestWithError( checkSampledZVals(Zout_src,velinftsec,
						 zinfo,zinfo_trans),
	    zinfo.isTime() ? "Sampled depth values" : "Sampled time values",
	    msg_ );

    return velinfeetsec ? true
	: testCalcZ( desc, modzinfo, zsamp, zinfo, &velinftsec );
}


static bool testVelocityConversion()
{
    const ZDomain::Info& ztwt = ZDomain::TWT();
    const ZDomain::Info& zdepthm = ZDomain::DepthMeter();
    const ZDomain::Info& zdepthft = ZDomain::DepthFeet();

    if ( !testVelConvs(VelocityType::Interval,VelocityType::Avg,ztwt) ||
	 !testVelConvs(VelocityType::Interval,VelocityType::RMS,ztwt) ||
	 !testVelConvs(VelocityType::Avg,VelocityType::Interval,ztwt) ||
	 !testVelConvs(VelocityType::Avg,VelocityType::RMS,ztwt) ||
	 !testVelConvs(VelocityType::RMS,VelocityType::Interval,ztwt) ||
	 !testVelConvs(VelocityType::RMS,VelocityType::Avg,ztwt) ||
	 !testVelConvs(VelocityType::Interval,VelocityType::Avg,zdepthm) ||
	 !testVelConvs(VelocityType::Avg,VelocityType::Interval,zdepthm) ||
	 !testVelConvs(VelocityType::Interval,VelocityType::Avg,zdepthft) ||
	 !testVelConvs(VelocityType::Avg,VelocityType::Interval,zdepthft) )
	return false;

    return true;
}


static bool testSampleVelocities()
{
    const ZSampling zsamp_time( 0.4f, 1.1f, 0.004f );
    const ZSampling zsamp_depthm( 400.f, 1200.f, 5.f );
    const ZSampling zsamp_depthft( 1200.f, 3930.f, 15.f );
    const ZDomain::Info& ztwt = ZDomain::TWT();
    const ZDomain::Info& zdepthm = ZDomain::DepthMeter();
    const ZDomain::Info& zdepthft = ZDomain::DepthFeet();

    VelocityDesc desc;
    desc.setType( VelocityType::Interval );
    if ( !testSampleVels(desc,ztwt,zsamp_time,ztwt) ||
	 !testSampleVels(desc,ztwt,zsamp_depthm,zdepthm) ||
	 !testSampleVels(desc,ztwt,zsamp_depthft,zdepthft) ||
	 !testSampleVels(desc,zdepthm,zsamp_depthm,zdepthm) ||
	 !testSampleVels(desc,zdepthm,zsamp_time,ztwt) ||
	 !testSampleVels(desc,zdepthm,zsamp_depthft,zdepthft) ||
	 !testSampleVels(desc,zdepthft,zsamp_time,ztwt) ||
	 !testSampleVels(desc,zdepthft,zsamp_depthm,zdepthm) ||
	 !testSampleVels(desc,zdepthft,zsamp_depthft,zdepthft) )
	return false;

    desc.setType( VelocityType::Avg );
    if ( !testSampleVels(desc,ztwt,zsamp_time,ztwt) ||
	 !testSampleVels(desc,ztwt,zsamp_depthm,zdepthm) ||
	 !testSampleVels(desc,ztwt,zsamp_depthft,zdepthft) ||
	 !testSampleVels(desc,zdepthm,zsamp_time,ztwt) ||
	 !testSampleVels(desc,zdepthm,zsamp_depthm,zdepthm) ||
	 !testSampleVels(desc,zdepthm,zsamp_depthft,zdepthft) ||
	 !testSampleVels(desc,zdepthft,zsamp_time,ztwt) ||
	 !testSampleVels(desc,zdepthft,zsamp_depthm,zdepthm) ||
	 !testSampleVels(desc,zdepthft,zsamp_depthft,zdepthft) )
	return false;

    desc.setType( VelocityType::RMS );
    if ( !testSampleVels(desc,ztwt,zsamp_time,ztwt) )
	return false;

    return true;
}


static bool testCalcZ()
{
    const ZSampling zsamp_time( 0.4f, 1.1f, 0.004f );
    const ZSampling zsamp_depthm( 400.f, 1200.f, 5.f );
    const ZSampling zsamp_depthft( 1200.f, 3930.f, 15.f );
    const ZDomain::Info& ztwt = ZDomain::TWT();
    const ZDomain::Info& zdepthm = ZDomain::DepthMeter();
    const ZDomain::Info& zdepthft = ZDomain::DepthFeet();

    VelocityDesc desc;
    desc.setType( VelocityType::Interval );
    if ( !testCalcZ(desc,ztwt,zsamp_time,ztwt) ||
	 !testCalcZ(desc,ztwt,zsamp_depthm,zdepthm) ||
	 !testCalcZ(desc,ztwt,zsamp_depthft,zdepthft) ||
	 !testCalcZ(desc,zdepthm,zsamp_time,ztwt) ||
	 !testCalcZ(desc,zdepthm,zsamp_depthm,zdepthm) ||
	 !testCalcZ(desc,zdepthm,zsamp_depthft,zdepthft) ||
	 !testCalcZ(desc,zdepthft,zsamp_time,ztwt) ||
	 !testCalcZ(desc,zdepthft,zsamp_depthm,zdepthm) ||
	 !testCalcZ(desc,zdepthft,zsamp_depthft,zdepthft) )
	return false;

    desc.setType( VelocityType::Avg );
    if ( !testCalcZ(desc,ztwt,zsamp_time,ztwt) ||
	 !testCalcZ(desc,ztwt,zsamp_depthm,zdepthm) ||
	 !testCalcZ(desc,ztwt,zsamp_depthft,zdepthft) ||
	 !testCalcZ(desc,zdepthm,zsamp_time,ztwt) ||
	 !testCalcZ(desc,zdepthm,zsamp_depthm,zdepthm) ||
	 !testCalcZ(desc,zdepthm,zsamp_depthft,zdepthft) ||
	 !testCalcZ(desc,zdepthft,zsamp_time,ztwt) ||
	 !testCalcZ(desc,zdepthft,zsamp_depthm,zdepthm) ||
	 !testCalcZ(desc,zdepthft,zsamp_depthft,zdepthft) )
	return false;

    desc.setType( VelocityType::RMS );
    if ( !testCalcZ(desc,ztwt,zsamp_time,ztwt) )
	return false;

    return true;
}


static bool testCalcZLinear()
{
    const double v0 = 2000.;
    ObjectSet<const ZDomain::Info> zinfos_in, zinfos_out;
    zinfos_in += &ZDomain::TWT();
    zinfos_in += &ZDomain::DepthMeter();
    zinfos_out += &ZDomain::DepthMeter();
    zinfos_out += &ZDomain::TWT();
    TypeSet<double> srdvals;
    srdvals += 0.;
    srdvals += 200.;
    TypeSet<double> vgrads;
    vgrads += 0.;
    vgrads += 0.1;
    for ( int idx=0; idx<zinfos_in.size(); idx++ )
    {
	const ZDomain::Info& zinfoin = *zinfos_in.get( idx );
	const ZDomain::Info& zinfoout = *zinfos_out.get( idx );
	const BufferString msg( "Linear ", zinfoin.isTime() ? "time-to-depth"
							    : "depth-to-time",
				" transformation" );
	const double* inpzarr = zinfoin.isTime() ? t0 : z0;
	const double* qczarr = zinfoin.isTime() ? z0 : t0;
	int idy = 0;
	for ( const auto& dv : vgrads )
	{
	    for ( const auto& srd : srdvals )
	    {
		TypeSet<double> inpzvals, qczvals;
		inpzvals += inpzarr[idy]; qczvals += qczarr[idy++];
		inpzvals += inpzarr[idy]; qczvals += qczarr[idy++];
		const ArrayZValues zvals_in( inpzvals.arr(), inpzvals.size(),
					     zinfoin );
		ArrayValueSeries<double,double> outpzvals( inpzvals.size() );
		ArrayZValues zvals_out( outpzvals.arr(), outpzvals.size(),
					zinfoout );

		const Vel::Worker worker( v0, dv, srd,
					  UnitOfMeasure::meterSecondUnit(),
					  UnitOfMeasure::meterUnit() );
		mRunStandardTest( worker.calcZLinear( zvals_in, zvals_out ),
				  msg );
		mRunStandardTestWithError(
			checkConvertedLinearVels( outpzvals.arr(),qczvals.arr(),
			outpzvals.size(), zinfoin ),
			BufferString( msg, " QC"), msg_ );
	    }
	}
    }

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testVelocityConversion() ||
	 !testSampleVelocities() ||
	 !testCalcZ() ||
	 !testCalcZLinear() )
	return 1;

    return 0;
}
