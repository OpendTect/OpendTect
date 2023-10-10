/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "zvalseriesimpl.h"

#include "samplingdata.h"
#include "zdomain.h"


// ZValueSerie

ZValueSerie::ZValueSerie( const ZDomain::Info& zinfo )
    : zdomaininfo_(zinfo)
{
}


ZValueSerie::~ZValueSerie()
{
    delete scaler_;
}


bool ZValueSerie::operator ==( const ZValueSerie& oth ) const
{
    if ( &oth == this )
	return true;

    if ( (zdomaininfo_ != oth.zdomaininfo_) ||
	 (scaler_ && !oth.scaler_) || (!scaler_ && oth.scaler_) )
	return false;

    return scaler_ ? *scaler_ == *oth.scaler_ : true;
}


bool ZValueSerie::operator !=( const ZValueSerie& oth ) const
{
    return !(oth == *this);
}


void ZValueSerie::setScaler( const LinScaler& scaler )
{
    if ( scaler_ )
	*scaler_ = scaler;
    else
	scaler_ = new LinScaler( scaler );
}


bool ZValueSerie::isTime() const
{
    return zdomaininfo_.isTime();
}


bool ZValueSerie::isDepth() const
{
    return zdomaininfo_.isDepth();
}


bool ZValueSerie::inMeter() const
{
    return zdomaininfo_.isDepthMeter();
}


bool ZValueSerie::inFeet() const
{
    return zdomaininfo_.isDepthFeet();
}


// RegularZValues

RegularZValues::RegularZValues( const ZSampling& zsamp,
				const ZDomain::Info& zinfo )
    : RegularZValues(getDoubleSamplingData(SamplingData<float>(zsamp)),
				od_int64(zsamp.nrSteps())+1,zinfo)
{}


RegularZValues::RegularZValues( const SamplingData<float>& sd, od_int64 sz,
				const ZDomain::Info& zinfo )
    : RegularZValues(getDoubleSamplingData(sd),sz,zinfo)
{}


RegularZValues::RegularZValues( const SamplingData<double>& sd, od_int64 sz,
				const ZDomain::Info& zinfo )
    : SamplingValues<double>(sd,sz)
    , ZValueSerie(zinfo)
{}


RegularZValues::~RegularZValues()
{}


bool RegularZValues::operator ==( const RegularZValues& oth ) const
{
    if ( &oth == this )
	return true;

    return SamplingValues<double>::operator ==( oth ) &&
	   ZValueSerie::operator ==( oth );
}


bool RegularZValues::operator !=( const RegularZValues& oth ) const
{
    return !(oth == *this);
}


ValueSeries<double>* RegularZValues::clone() const
{
    return new RegularZValues( sd_, size(), zDomainInfo() );
}


double RegularZValues::getStep() const
{
    return getScaler() ? sd_.step * getScaler()->factor : sd_.step;
}


od_int64 RegularZValues::getIndex( double val ) const
{
    if ( !isOK() )
	return mUdf(od_int64);

    return (od_int64)((val - value(0)) / getStep());
}


double RegularZValues::value( od_int64 idx ) const
{
    double val = SamplingValues<double>::value( idx );
    return getScaler() ? getScaler()->scale( val ) : val;
}


SamplingData<double> RegularZValues::getDoubleSamplingData(
						const SamplingData<float>& sdf )
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
