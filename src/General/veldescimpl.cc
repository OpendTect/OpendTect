/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "veldescimpl.h"

#include "iopar.h"
#include "odmemory.h"
#include "separstr.h"
#include "stringview.h"
#include "uistrings.h"
#include "unitofmeasure.h"
#include "velocitycalc.h"
#include "zvalseriesimpl.h"


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
    const Scaler& inpvelscaler = getUnit( desc_ )->scaler();
    const Scaler& outvelscaler = getUnit( newdesc )->scaler();
    if ( !inpvelscaler.isEmpty() )
    {
	auto& Vined = const_cast<ValueSeries<double>&>( Vin_src );
	Vin_scaled =
		new ScaledValueSeries<double,double>( &inpvelscaler, Vined );
    }

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
    const Scaler& velscaler = getUnit( desc_ )->scaler();
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
    else if ( desc_.getType() == OD::VelocityType::Eta )
    {
	sampleEffectiveThomsenPars( Vin, *zvals_in, *zvals_out, Vout );
    }
    else if ( desc_.getType() == OD::VelocityType::Delta ||
	      desc_.getType() == OD::VelocityType::Epsilon )
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
    const Scaler& velscaler = getUnit( desc_ )->scaler();
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
    if ( !getUnit( desc_ )->scaler().isEmpty() )
	Vin_scaled = new ScaledValueSeries<double,double>(
				&getUnit( desc_ )->scaler(),
				const_cast<ValueSeries<double>&>( Vin_src ) );
    const ValueSeries<double>& Vin = Vin_scaled ? *Vin_scaled : Vin_src;

    PtrMan<ZValueSeries> zvals_in = getZVals( zvals_inp, srd_, nullptr );
    PtrMan<ZValueSeries> zvals_out = getZVals( zvals_outp, srd_, nullptr );
    PtrMan<ZValueSeries> zvals_trans = getZVals( Zout_src, srd_, nullptr,
						 false );
    if ( !zvals_in || !zvals_out || !zvals_trans )
	return false;

    return Vel::getSampledZ( Vin, *zvals_in, desc_.getType(), *zvals_out,
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


const UnitOfMeasure* Vel::Worker::getUnit( const VelocityDesc& desc )
{
    if ( !desc.isVelocity() )
	return nullptr;

    const UnitOfMeasure* veluom = UnitOfMeasure::surveyDefVelUnit();
    const UnitOfMeasure* velstoruom = UoMR().get( desc.velunit_.buf() );
    return velstoruom && velstoruom->isCompatibleWith( *veluom )
		    ? velstoruom : veluom;
}


void Vel::Worker::setUnit( const UnitOfMeasure* uom, VelocityDesc& desc )
{
    if ( uom )
    {
	if ( uom->propType() == Mnemonic::Vel )
	    desc.velunit_.set( uom->getLabel() );
	else
	    { pFreeFnErrMsg("Incorrect unit of measure"); }
    }
    else
	desc.velunit_.setEmpty();
}