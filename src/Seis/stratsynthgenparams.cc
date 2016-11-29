/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
________________________________________________________________________

-*/


#include "stratsynthgenparams.h"

#include "raytrace1d.h"
#include "raytracerrunner.h"
#include "synthseis.h"

static const char* sKeyIsPreStack()		{ return "Is Pre Stack"; }
static const char* sKeySynthType()		{ return "Synthetic Type"; }
static const char* sKeyWaveLetName()		{ return "Wavelet Name"; }
static const char* sKeyRayPar()			{ return "Ray Parameter"; }
static const char* sKeyInput()			{ return "Input Synthetic"; }
static const char* sKeyAngleRange()		{ return "Angle Range"; }
static const char* sKeyAdvancedRayTracer()	{ return "FullRayTracer"; }

#define sDefaultAngleRange Interval<float>( 0.0f, 30.0f )
#define sDefaultOffsetRange StepInterval<float>( 0.f, 6000.f, 100.f )


mDefineEnumUtils(SynthGenParams,SynthType,"Synthetic Type")
{
    "Pre Stack",
    "Zero Offset Stack",
    "Strat Property",
    "Angle Stack",
    "AVO Gradient",
    0
};


SynthGenParams::SynthGenParams()
{
    synthtype_ = ZeroOffset;	//init to avoid nasty crash in generateSD!
    setDefaultValues();
}

void SynthGenParams::setDefaultValues()
{
    anglerg_ = sDefaultAngleRange;
    raypars_.setEmpty();
    FixedString defrayparstr = sKeyAdvancedRayTracer();
    const BufferStringSet& facnms = RayTracer1D::factory().getNames();
    if ( !facnms.isEmpty() )
    {
	const int typeidx = facnms.indexOf( defrayparstr );
	FixedString facnm( typeidx>=0 ? facnms.get(typeidx) : facnms.get(0) );
	raypars_.set( sKey::Type(), facnm );
    }

    if ( synthtype_==ZeroOffset )
	RayTracer1D::setIOParsToZeroOffset( raypars_ );
    else
    {
	const StepInterval<float> offsetrg = sDefaultOffsetRange;
	TypeSet<float> offsets;
	for ( int idx=0; idx<offsetrg.nrSteps()+1; idx++ )
	    offsets += offsetrg.atIndex( idx );
	raypars_.set( RayTracer1D::sKeyOffset(), offsets );
    }
}


bool SynthGenParams::hasOffsets() const
{
    TypeSet<float> offsets;
    raypars_.get( RayTracer1D::sKeyOffset(), offsets );
    return offsets.size()>1;
}


void SynthGenParams::fillPar( IOPar& par ) const
{
    par.set( sKey::Name(), name_ );
    par.set( sKeySynthType(), SynthGenParams::toString(synthtype_) );
    if ( synthtype_ == SynthGenParams::AngleStack ||
	 synthtype_ == SynthGenParams::AVOGradient )
    {
	par.set( sKeyInput(), inpsynthnm_ );
	par.set( sKeyAngleRange(), anglerg_ );
    }
    par.set( sKeyWaveLetName(), wvltnm_ );
    IOPar raypar;
    raypar.mergeComp( raypars_, sKeyRayPar() );
    par.merge( raypar );
}


void SynthGenParams::usePar( const IOPar& par )
{
    par.get( sKey::Name(), name_ );
    par.get( sKeyWaveLetName(), wvltnm_ );
    PtrMan<IOPar> raypar = par.subselect( sKeyRayPar() );
    raypars_ = *raypar;
    if ( par.hasKey( sKeyIsPreStack()) )
    {
	bool isps = false;
	par.getYN( sKeyIsPreStack(), isps );
	if ( !isps && hasOffsets() )
	    synthtype_ = SynthGenParams::AngleStack;
	else if ( !isps )
	    synthtype_ = SynthGenParams::ZeroOffset;
	else
	    synthtype_ = SynthGenParams::PreStack;
    }
    else
    {
	BufferString typestr;
	SynthTypeDef().parse( par, sKeySynthType(), synthtype_ );
	if ( synthtype_ == SynthGenParams::AngleStack ||
	     synthtype_ == SynthGenParams::AVOGradient )
	{
	    par.get( sKeyInput(), inpsynthnm_ );
	    par.get( sKeyAngleRange(), anglerg_ );
	}
    }
}


void SynthGenParams::createName( BufferString& nm ) const
{
    if ( synthtype_==SynthGenParams::AngleStack ||
	 synthtype_==SynthGenParams::AVOGradient )
    {
	nm = SynthGenParams::toString( synthtype_ );
	nm += " ["; nm += anglerg_.start; nm += ",";
	nm += anglerg_.stop; nm += "] degrees";
	return;
    }
    nm = wvltnm_;
    TypeSet<float> offset;
    raypars_.get( RayTracer1D::sKeyOffset(), offset );
    const int offsz = offset.size();
    if ( offsz )
    {
	nm += " ";
	nm += "Offset ";
	nm += ::toString( offset[0] );
	if ( offsz > 1 )
	{
	    nm += "-"; nm += offset[offsz-1];
	    bool nmocorrected = true;
	    if ( raypars_.getYN(Seis::SynthGenBase::sKeyNMO(),nmocorrected) &&
		 !nmocorrected )
		nm += " uncorrected";
	}
    }
}
