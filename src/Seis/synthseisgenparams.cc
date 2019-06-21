/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
________________________________________________________________________

-*/


#include "synthseisgenparams.h"

#include "dbman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "raytrace1d.h"
#include "raytracerrunner.h"
#include "synthseisgenerator.h"
#include "waveletio.h"
#include "ctxtioobj.h"

static const char* sKeyIsPreStack()		{ return "Is Pre Stack"; }
static const char* sKeySynthType()		{ return "Synthetic Type"; }
static const char* sKeyWaveLetID()		{ return "Wavelet.ID"; }
static const char* sKeyWaveLetName()		{ return "Wavelet Name"; }
static const char* sKeyRayPar()			{ return "Ray Parameter"; }
static const char* sKeyInput()			{ return "Input Synthetic"; }
static const char* sKeyAngleRange()		{ return "Angle Range"; }
static const char* sKeyAdvancedRayTracer()	{ return "FullRayTracer"; }

#define cDefaultAngleRange Interval<int>( 0, 30 )
#define cDefaultOffsetRange StepInterval<float>( 0.f, 6000.f, 100.f )


mDefineNameSpaceEnumUtils(SynthSeis,SyntheticType,"Synthetic Type")
{
    "Zero Offset Stack",
    "Pre Stack",
    "Rock Property",
    "Angle Stack",
    "AVO Gradient",
    0
};

template<>
void EnumDefImpl<SynthSeis::SyntheticType>::init()
{
    uistrings_ += mEnumTr("Zero Offset Stack",0);
    uistrings_ += uiStrings::sPreStack();
    uistrings_ += mEnumTr("Rock Property",0);
    uistrings_ += mEnumTr("Angle Stack",0);
    uistrings_ += mEnumTr("AVO Gradient",0);
}


SynthSeis::GenParams::GenParams()
    : type_(ZeroOffset)
{
    setDefaultValues();
}


bool SynthSeis::GenParams::operator==( const GenParams& oth ) const
{
    if ( type_ != oth.type_
      || raypars_ != oth.raypars_
      || wvltid_ != oth.wvltid_ )
	return false;

    return !isPSPostProc()
	|| (anglerg_ == oth.anglerg_ && inpsynthnm_ == oth.inpsynthnm_);
}



void SynthSeis::GenParams::setDefaultValues()
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( Wavelet );
    ctio->fillDefault();
    if ( ctio->ioobj_ )
    {
	wvltid_ = ctio->ioobj_->key();
	ctio->setObj( 0 );
    }

    anglerg_ = cDefaultAngleRange;
    raypars_.setEmpty();
    if ( !isZeroOffset() && !isPS() )
	return;

    BufferString defrayparstr = sKeyAdvancedRayTracer();
    const BufferStringSet& fackys = RayTracer1D::factory().getKeys();
    if ( !fackys.isEmpty() )
    {
	const int typeidx = fackys.indexOf( defrayparstr );
	FixedString facnm( typeidx>=0 ? fackys.get(typeidx) : fackys.get(0) );
	raypars_.set( sKey::Type(), facnm );
    }

    if ( type_ == ZeroOffset )
    {
	RayTracer1D::setIOParsToZeroOffset( raypars_ );
	raypars_.setYN( SynthSeis::GenBase::sKeyNMO(), false );
    }
    else
    {
	const StepInterval<float> offsetrg = cDefaultOffsetRange;
	TypeSet<float> offsets;
	for ( int idx=0; idx<offsetrg.nrSteps()+1; idx++ )
	    offsets += offsetrg.atIndex( idx );
	raypars_.set( RayTracer1D::sKeyOffset(), offsets );
	raypars_.setYN( SynthSeis::GenBase::sKeyNMO(), true );
    }

    name_ = createName();
}


bool SynthSeis::GenParams::hasOffsets() const
{
    TypeSet<float> offsets;
    raypars_.get( RayTracer1D::sKeyOffset(), offsets );
    return offsets.size()>1;
}


BufferString SynthSeis::GenParams::waveletName() const
{
    const BufferString dbnm = wvltid_.name();
    return dbnm.isEmpty() ? fallbackwvltnm_ : dbnm;
}


void SynthSeis::GenParams::setWaveletName( const char* nm )
{
    fallbackwvltnm_ = nm;
    PtrMan<IOObj> ioobj = DBM().getByName( IOObjContext::Seis, nm, "Wavelet" );
    wvltid_ = ioobj ? ioobj->key() : DBKey();
}


void SynthSeis::GenParams::fillPar( IOPar& par ) const
{
    par.set( sKey::Name(), name_ );
    par.set( sKeySynthType(), toString(type_) );
    if ( type_ == AngleStack || type_ == AVOGradient )
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


void SynthSeis::GenParams::usePar( const IOPar& par )
{
    par.get( sKey::Name(), name_ );
    if ( !par.get(sKeyWaveLetID(),wvltid_) )
    {
	BufferString wvltnm;
	if ( par.get(sKeyWaveLetName(),wvltnm) )
	    setWaveletName( wvltnm );
    }

    PtrMan<IOPar> raypar = par.subselect( sKeyRayPar() );
    if ( raypar )
    {
	raypars_ = *raypar;
	raypars_.set( sKey::WaveletID(), wvltid_ ); // bw compat
    }

    if ( par.hasKey( sKeyIsPreStack()) )
    {
	bool isps = false;
	par.getYN( sKeyIsPreStack(), isps );
	if ( !isps && hasOffsets() )
	    type_ = AngleStack;
	else if ( !isps )
	    type_ = ZeroOffset;
	else
	    type_ = PreStack;
    }
    else
    {
	BufferString typestr;
	SyntheticTypeDef().parse( par, sKeySynthType(), type_ );
	if ( isPSPostProc(type_) )
	{
	    par.get( sKeyInput(), inpsynthnm_ );
	    par.get( sKeyAngleRange(), anglerg_ );
	}
    }
}


BufferString SynthSeis::GenParams::createName() const
{
    BufferString ret;

    if ( isPSPostProc(type_) )
    {
	ret = toString( type_ );
	ret.add( " [" )
	   .add( anglerg_.start ).add( "-" ).add( anglerg_.stop )
	   .add( "]" );
	return ret;
    }

    ret.set( "#" ).add( waveletName() );
    TypeSet<float> offset;
    raypars_.get( RayTracer1D::sKeyOffset(), offset );
    const int offsz = offset.size();
    const float offs0 = offsz < 1 ? 0.f : offset.first();
    const bool onlyoffs0 = offsz < 1 || (offsz==1 && mIsZero(offs0,0.001f));
    if ( !onlyoffs0 )
    {
	ret.add( " O=" ).add( offs0 );
	if ( offsz > 1 )
	    ret.add( "-" ).add( offset.last() );
	bool nmocorrected = true;
	if ( raypars_.getYN(SynthSeis::GenBase::sKeyNMO(),nmocorrected)
	  && !nmocorrected )
	    ret.add( " [no NMO]" );
    }

    return ret;
}
