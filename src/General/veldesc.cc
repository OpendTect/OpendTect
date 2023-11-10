/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "veldesc.h"

#include "iopar.h"
#include "odmemory.h"
#include "separstr.h"
#include "stringview.h"
#include "staticsdesc.h"
#include "uistrings.h"
#include "unitofmeasure.h"
#include "velocitycalc.h"
#include "zvalseriesimpl.h"

const char* sKeyIsVelocity = "Is Velocity";
const char* VelocityDesc::sKeyVelocityVolume()	{ return "Velocity volume"; }
const char* VelocityDesc::sKeyVelocityUnit()	{ return "Velocity Unit"; }
const char* VelocityDesc::sKeyVelocityType()
{ return OD::VelocityTypeDef().name().buf(); }


VelocityDesc::VelocityDesc()
{
}


VelocityDesc::VelocityDesc( OD::VelocityType typ, const UnitOfMeasure* uom )
    : type_(typ)
{
    if ( uom )
	setUnit( uom );
}


VelocityDesc::~VelocityDesc()
{
}


bool VelocityDesc::operator==( const VelocityDesc& oth ) const
{
    return type_==oth.type_ && velunit_ == oth.velunit_ &&
	   statics_==oth.statics_;
}


bool VelocityDesc::operator!=( const VelocityDesc& oth ) const
{
    return !(*this==oth);
}


bool VelocityDesc::isUdf( OD::VelocityType type )
{
    return type == OD::VelocityType::Unknown;
}


bool VelocityDesc::isUdf() const
{ return isUdf(type_); }


bool VelocityDesc::isVelocity( OD::VelocityType type )
{ return type==OD::VelocityType::Interval || type==OD::VelocityType::RMS ||
	 type==OD::VelocityType::Avg; }


bool VelocityDesc::isVelocity() const
{ return isVelocity(type_); }


bool VelocityDesc::isInterval() const
{
    return type_ == OD::VelocityType::Interval;
}


bool VelocityDesc::isRMS() const
{
    return type_ == OD::VelocityType::RMS;
}


bool VelocityDesc::isAvg() const
{
    return type_ == OD::VelocityType::Avg;
}


bool VelocityDesc::isThomsen( OD::VelocityType type )
{ return type==OD::VelocityType::Delta || type==OD::VelocityType::Epsilon ||
	 type==OD::VelocityType::Eta; }


bool VelocityDesc::isThomsen() const
{ return isThomsen(type_); }


bool VelocityDesc::hasVelocityUnit() const
{
    if ( velunit_.isEmpty() || !isVelocity() )
	return false;

    const UnitOfMeasure* veluom = UnitOfMeasure::surveyDefVelUnit();
    const UnitOfMeasure* velstoruom = UoMR().get( velunit_.buf() );
    return velstoruom && velstoruom->isCompatibleWith( *veluom );
}


const UnitOfMeasure* VelocityDesc::getUnit() const
{
    if ( !isVelocity() )
	return nullptr;

    return hasVelocityUnit() ? UoMR().get( velunit_.buf() )
			     : UnitOfMeasure::surveyDefVelUnit();
}


void VelocityDesc::setUnit( const UnitOfMeasure* uom )
{
    if ( uom )
    {
	if ( uom->propType() == Mnemonic::Vel )
	    velunit_.set( uom->getLabel() );
	else
	    { pErrMsg("Incorrect unit of measure"); }
    }
    else
	velunit_.setEmpty();
}


void VelocityDesc::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), sKey::Velocity() );
    par.set( sKeyVelocityType(), OD::toString(type_) );
    if ( hasVelocityUnit() )
	par.set( sKeyVelocityUnit(), velunit_ );

    if ( type_ == OD::VelocityType::RMS )
	statics_.fillPar( par );
    else
	StaticsDesc::removePars( par );

    // Legacy, keep it for backward readability of the database:
    par.setYN( sKeyIsVelocity, true );
}


bool VelocityDesc::usePar( const IOPar& par )
{
    BufferString typstr;
    if ( !par.get(sKey::Type(),typstr) && typstr != sKey::Velocity() )
    {
	bool isvel;
	if ( !par.getYN(sKeyIsVelocity,isvel) || !isvel )
	    return false;
    }

    OD::VelocityType type = type_;
    if ( OD::parseEnum(par,sKeyVelocityType(),type) )
    {
	type_ = type;
	if ( type_ == OD::VelocityType::RMS && !statics_.usePar(par) )
	    return false;

	BufferString velunit;
	if ( par.get(sKeyVelocityUnit(),velunit) && !velunit.isEmpty() &&
	     isVelocity() )
	    setUnit( UoMR().get(velunit.buf()) );
    }
    else
    {  // legacy format
	BufferString arr;
	if ( !par.get("Velocity Desc",arr) )
	    return false;

	const FileMultiString fms( arr );
	if ( fms.isEmpty() )
	    return false;

	if ( !OD::parseEnum(par,fms[0],type) )
	    return false;

	type_ = type;
	BufferString velunit;
	if ( par.get(sKeyVelocityUnit(),velunit) && !velunit.isEmpty() &&
	     isVelocity() )
	    setUnit( UoMR().get(velunit.buf()) );

	statics_.velattrib_.setEmpty();
	statics_.vel_ = mUdf(double);
	return true;
    }

    return true;
}


void VelocityDesc::removePars( IOPar& par )
{
    par.removeWithKey( sKey::Type() );
    par.removeWithKey( sKeyVelocityType() );
    par.removeWithKey( sKeyVelocityUnit() );
    StaticsDesc::removePars( par );
    par.removeWithKey( sKeyIsVelocity );
}


uiString VelocityDesc::getVelVolumeLabel()
{
    return tr( "Velocity model");
}


bool VelocityDesc::isUsable( OD::VelocityType type, const ZDomain::Def& zddef,
			     uiRetVal& uirv )
{
    if ( type == OD::VelocityType::Interval || type == OD::VelocityType::Avg )
	return uirv.isOK();

    if ( zddef.isTime() )
    {
	if ( type != OD::VelocityType::RMS )
	    uirv.add( tr("Only RMS, Avg and Interval allowed for time based "
			 "models") );
    }
    else if ( zddef.isDepth() )
	uirv.add( tr("Only Avg and Interval allowed for depth based models") );
    else
	uirv.add( tr("Velocity not usable in the '%1' domain")
			.arg(zddef.userName()) );

    return uirv.isOK();
}


// Vel::Worker

Vel::Worker::Worker( const VelocityDesc& desc, double srd,
		     const UnitOfMeasure* srduom )
    : desc_(desc)
    , srd_(srd)
{
    if ( srduom )
    {
	if ( srduom->propType() == Mnemonic::Dist )
	    srd_ = srduom->getSIValue( srd_ );
	else
	    { pErrMsg("Incorrect unit of measure"); }
    }
}


Vel::Worker::Worker( double v0, double dv, double srd,
		     const UnitOfMeasure* v0uom, const UnitOfMeasure* srduom )
    : desc_(OD::VelocityType::Interval)
    , srd_(srd)
    , v0_(v0)
    , dv_(dv)
{
    if ( v0uom )
    {
	if ( v0uom->propType() == Mnemonic::Vel )
	    v0_ = v0uom->getSIValue( v0_ );
	else
	    { pErrMsg("Incorrect unit of measure"); }
    }

    if ( srduom )
    {
	if ( srduom->propType() == Mnemonic::Dist )
	    srd_ = srduom->getSIValue( srd_ );
	else
	    { pErrMsg("Incorrect unit of measure"); }
    }
}


Vel::Worker::~Worker()
{
}


bool Vel::Worker::convertVelocities( const ValueSeries<double>& Vin_src,
				     const ZValueSeries& zvals_in,
				     const VelocityDesc& newdesc,
				     ValueSeries<double>& Vout_src,
				     double t0 ) const
{
    if ( !desc_.isVelocity() || !newdesc.isVelocity() )
	return false;

    ConstPtrMan<const ValueSeries<double> > Vin_scaled;
    PtrMan<ValueSeries<double> > Vout_scaled;
    const Scaler& inpvelscaler = desc_.getUnit()->scaler();
    const Scaler& outvelscaler = newdesc.getUnit()->scaler();
    if ( !inpvelscaler.isEmpty() )
	Vin_scaled = new ScaledValueSeries<double,double>(
				&inpvelscaler,
				const_cast<ValueSeries<double>&>( Vin_src ) );
    const ValueSeries<double>& Vin = Vin_scaled ? *Vin_scaled : Vin_src;

    if ( !outvelscaler.isEmpty() )
	Vout_scaled = new ScaledValueSeries<double,double>(
				    &outvelscaler, Vout_src );
    ValueSeries<double>& Vout = Vout_scaled ? *Vout_scaled : Vout_src;

    if ( desc_.type_ == newdesc.type_ )
    {
	if ( Vout.size() != Vin.size() )
	    return false;

	if ( &Vout != &Vin )
	    Vin.getValues( Vout, Vout.size() );

	return true;
    }

    if ( zvals_in.isDepth() && (desc_.isRMS() || newdesc.isRMS()) )
	return false;

    PtrMan<ZValueSeries> zvals = getZVals( zvals_in, srd_, nullptr );
    if ( !zvals )
	return false;

    bool isok = false;
    if ( desc_.isInterval() )
    {
	if ( newdesc.isAvg() )
	    isok = computeVavg( Vin, *zvals, Vout );
	else if ( newdesc.isRMS() )
	    isok = computeVrms( Vin, *zvals, Vout, t0 );
    }
    else if ( desc_.isAvg() )
    {
	isok = computeVint( Vin, *zvals, Vout );
	if ( isok && newdesc.isRMS() )
	    isok = computeVrms( Vout, *zvals, Vout, t0 ) ;
    }
    else if ( desc_.isRMS() )
    {
	isok = zvals->isTime() && computeDix( Vin, *zvals, Vout, t0 );
	if ( isok && newdesc.isAvg() )
	    isok = computeVavg( Vout, *zvals, Vout );
    }

    return isok;
}


bool Vel::Worker::sampleVelocities( const ValueSeries<double>& Vin_src,
				    const ZValueSeries& zvals_inp,
				    const ZValueSeries& zvals_outp,
				    ValueSeries<double>& Vout_src,
				    double t0 ) const
{
    if ( desc_.isUdf() )
	return false;

    ConstPtrMan<const ValueSeries<double> > Vin_scaled;
    PtrMan<ValueSeries<double> > Vout_scaled;
    const Scaler& velscaler = desc_.getUnit()->scaler();
    if ( !velscaler.isEmpty() )
    {
	Vin_scaled = new ScaledValueSeries<double,double>(
				&velscaler,
				const_cast<ValueSeries<double>&>( Vin_src ) );
	Vout_scaled = new ScaledValueSeries<double,double>(
				    &velscaler, Vout_src );
    }

    const ValueSeries<double>& Vin = Vin_scaled ? *Vin_scaled : Vin_src;
    ValueSeries<double>& Vout = Vout_scaled ? *Vout_scaled : Vout_src;

    PtrMan<ZValueSeries> zvals_in = getZVals( zvals_inp, srd_, nullptr );
    PtrMan<ZValueSeries> zvals_out = getZVals( zvals_outp, srd_, nullptr );
    if ( !zvals_in || !zvals_out )
	return false;

    bool isok = desc_.isThomsen();
    if ( desc_.isInterval() )
    {
	isok = sampleVint( Vin, *zvals_in, *zvals_out, Vout );
    }
    else if ( desc_.isAvg() )
    {
	isok = sampleVavg( Vin, *zvals_in, *zvals_out, Vout );
    }
    else if ( desc_.isRMS() )
    {
	isok = zvals_in->isTime() && zvals_out->isTime() &&
	       sampleVrms( Vin, *zvals_in, *zvals_out, Vout, t0 );
    }
    else if ( desc_.type_ == OD::VelocityType::Eta )
    {
	sampleEffectiveThomsenPars( Vin, *zvals_in, *zvals_out, Vout );
    }
    else if ( desc_.type_ == OD::VelocityType::Delta ||
	      desc_.type_ == OD::VelocityType::Epsilon )
    {
	sampleIntvThomsenPars( Vin, *zvals_in, *zvals_out, Vout );
    }

    return isok;
}


bool Vel::Worker::calcZ_( const ValueSeries<double>& Vin_src,
			  const ZValueSeries& zvals_in,
			  ZValueSeries& Zout_src, double t0 ) const
{
    if ( !desc_.isVelocity() )
	return false;

    ConstPtrMan<const ValueSeries<double> > Vin_scaled;
    const Scaler& velscaler = desc_.getUnit()->scaler();
    if ( !velscaler.isEmpty() )
    {
	Vin_scaled = new ScaledValueSeries<double,double>(
				&velscaler,
				const_cast<ValueSeries<double>&>( Vin_src ) );
    }

    const ValueSeries<double>& Vin = Vin_scaled ? *Vin_scaled : Vin_src;
    PtrMan<ZValueSeries> zvals = getZVals( zvals_in, srd_, nullptr, true );
    PtrMan<ZValueSeries> zout = getZVals( Zout_src, srd_, nullptr, false );
    if ( !zvals || !zout )
	return false;

    if ( zvals_in.isTime() )
    {
	if ( desc_.isInterval() )
	    return calcDepthsFromVint( Vin, *zvals, *zout );
	if ( desc_.isRMS() )
	    return calcDepthsFromVrms( Vin, *zvals, *zout, t0 );
	if ( desc_.isAvg() )
	    return calcDepthsFromVavg( Vin, *zvals, *zout );
    }
    else
    {
	if ( desc_.isInterval() )
	    return calcTimesFromVint( Vin, *zvals, *zout );
	if ( desc_.isAvg() )
	    return calcTimesFromVavg( Vin, *zvals, *zout );
    }

    return false;
}


bool Vel::Worker::getSampledZ_( const ValueSeries<double>& Vin_src,
			       const ZValueSeries& zvals_inp,
			       const ZValueSeries& zvals_outp,
			       ZValueSeries& Zout_src, double t0 ) const
{
    if ( !desc_.isVelocity() )
	return false;

    ConstPtrMan<const ValueSeries<double> > Vin_scaled;
    if ( !desc_.getUnit()->scaler().isEmpty() )
	Vin_scaled = new ScaledValueSeries<double,double>(
				&desc_.getUnit()->scaler(),
				const_cast<ValueSeries<double>&>( Vin_src ) );
    const ValueSeries<double>& Vin = Vin_scaled ? *Vin_scaled : Vin_src;

    PtrMan<ZValueSeries> zvals_in = getZVals( zvals_inp, srd_, nullptr );
    PtrMan<ZValueSeries> zvals_out = getZVals( zvals_outp, srd_, nullptr );
    PtrMan<ZValueSeries> zvals_trans = getZVals( Zout_src, srd_, nullptr,
						 false );
    if ( !zvals_in || !zvals_out || !zvals_trans )
	return false;

    return Vel::getSampledZ( Vin, *zvals_in, desc_.type_, *zvals_out,
			    *zvals_trans, t0 );
}


bool Vel::Worker::calcZLinear_( const ZValueSeries& zvals_in,
			        ZValueSeries& zvals_out ) const
{
    if ( mIsUdf(v0_) || !desc_.isInterval() )
	return false;

    PtrMan<ZValueSeries> zvals = getZVals( zvals_in, srd_, nullptr, true );
    PtrMan<ZValueSeries> zout = getZVals( zvals_out, srd_, nullptr, false );
    if ( !zvals || !zout )
	return false;

    return zvals_in.isTime() ? calcDepthsFromLinearV0k( v0_, dv_, *zvals, *zout)
			     : calcTimesFromLinearV0k( v0_, dv_, *zvals, *zout);
}


ZValueSeries* Vel::Worker::getZVals( const ZValueSeries& zvals_in, double srd,
				     const UnitOfMeasure* srduom,
				     bool forward )
{

    auto* ret = zvals_in.clone();
    mDynamicCastGet(ZValueSeries*,zvals,ret);
    if ( zvals && zvals->isDepth() )
    {
	LinScaler scaler;
	if ( zvals->inFeet() )
	    scaler.factor = forward ? mFromFeetFactorD : mToFeetFactorD;

	if ( srduom && srduom->propType() == Mnemonic::Dist )
	{
	    if ( srduom->propType() == Mnemonic::Dist )
		srd = srduom->getSIValue( srd );
	    else
		{ pFreeFnErrMsg( "Incorrect unit of measure" ); }
	}

	scaler.constant = forward ? srd
				  : (zvals->inFeet() ? -srd * mToFeetFactorD
						     : -srd);

	zvals->setScaler( scaler );
    }

    return zvals;
}
