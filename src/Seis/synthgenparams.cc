/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
________________________________________________________________________

-*/


#include "synthgenparams.h"

#include "dbman.h"
#include "ioobj.h"
#include "raytrace1d.h"
#include "raytracerrunner.h"
#include "synthseis.h"

static const char* sKeyIsPreStack()		{ return "Is Pre Stack"; }
static const char* sKeySynthType()		{ return "Synthetic Type"; }
static const char* sKeyWaveLetID()		{ return "Wavelet.ID"; }
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

template<>
void EnumDefImpl<SynthGenParams::SynthType>::init()
{
    uistrings_ += uiStrings::sPreStack();
    uistrings_ += mEnumTr("Zero Offset Stack",0);
    uistrings_ += mEnumTr("Startigraphic Property",0);
    uistrings_ += mEnumTr("Angle Mute",0);
    uistrings_ += mEnumTr("AVO Gradient",0);
}


SynthGenParams::SynthGenParams()
{
    synthtype_ = ZeroOffset;	//init to avoid nasty crash in generateSD!
    setDefaultValues();
}


bool SynthGenParams::operator==( const SynthGenParams& oth ) const
{
    if ( synthtype_ != oth.synthtype_
	    || raypars_ != oth.raypars_
	    || wvltid_ != oth.wvltid_ )
	return false;

    return !isPSBased()
	|| (anglerg_ == oth.anglerg_ && inpsynthnm_ == oth.inpsynthnm_);
}



void SynthGenParams::setDefaultValues()
{
    anglerg_ = sDefaultAngleRange;
    raypars_.setEmpty();
    FixedString defrayparstr = sKeyAdvancedRayTracer();
    const BufferStringSet& fackys = RayTracer1D::factory().getKeys();
    if ( !fackys.isEmpty() )
    {
	const int typeidx = fackys.indexOf( defrayparstr );
	FixedString facnm( typeidx>=0 ? fackys.get(typeidx) : fackys.get(0) );
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


BufferString SynthGenParams::waveletName() const
{
    BufferString dbnm = nameOf( wvltid_ );
    return dbnm.isEmpty() ? fallbackwvltnm_ : dbnm;
}


void SynthGenParams::setWaveletName( const char* nm )
{
    fallbackwvltnm_ = nm;
    PtrMan<IOObj> ioobj = DBM().getByName( IOObjContext::Seis, nm, "Wavelet" );
    wvltid_ = ioobj ? ioobj->key() : DBKey();
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
    par.set( sKeyWaveLetID(), wvltid_ );
    par.set( sKeyWaveLetName(), waveletName() ); // bw compat
    IOPar raypar;
    raypar.mergeComp( raypars_, sKeyRayPar() );
    par.merge( raypar );
}


void SynthGenParams::usePar( const IOPar& par )
{
    par.get( sKey::Name(), name_ );
    if ( !par.get(sKeyWaveLetID(),wvltid_) )
    {
	BufferString wvltnm;
	if ( par.get(sKeyWaveLetName(),wvltnm) )
	    setWaveletName( wvltnm );
    }

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


BufferString SynthGenParams::createName() const
{
    BufferString ret;

    if ( synthtype_==SynthGenParams::AngleStack ||
	 synthtype_==SynthGenParams::AVOGradient )
    {
	ret = SynthGenParams::toString( synthtype_ );
	ret.add( " [" )
	   .add( anglerg_.start ).add( "," ).add( anglerg_.stop )
	   .add( "] degrees" );
	return ret;
    }

    ret = waveletName();
    TypeSet<float> offset;
    raypars_.get( RayTracer1D::sKeyOffset(), offset );
    const int offsz = offset.size();
    if ( offsz )
    {
	ret.add( " Offset " ).add( ::toString(offset[0]) );
	if ( offsz > 1 )
	{
	    ret.add( "-" ).add( offset[offsz-1] );
	    bool nmocorrected = true;
	    if ( raypars_.getYN(Seis::SynthGenBase::sKeyNMO(),nmocorrected)
	      && !nmocorrected )
		ret.add( " uncorrected" );
	}
    }

    return ret;
}
