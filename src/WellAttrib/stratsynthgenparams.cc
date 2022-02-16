/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
________________________________________________________________________

-*/

#include "stratsynthgenparams.h"

#include "ioman.h"
#include "prestackanglemute.h"
#include "raytrace1d.h"
#include "synthseis.h"
#include "survinfo.h"
#include "wavelet.h"
#include "waveletio.h"

static const char* sKeyIsPreStack()		{ return "Is Pre Stack"; }
const char* SynthGenParams::sKeyRayPar()
{ return RayTracer1D::sKeyRayPar(); }
const char* SynthGenParams::sKeySynthPar()
{ return Seis::RaySynthGenerator::sKeySynthPar(); }


mDefineEnumUtils(SynthGenParams,SynthType,"Synthetic Type")
{
    "Zero Offset Stack",
    "Pre Stack",
    "Strat Property",
    "Angle Stack",
    "AVO Gradient",
    "Attribute",
    nullptr
};


SynthGenParams::SynthGenParams( SynthType tp )
    : synthtype_(tp)
{
    setDefaultValues();
}


SynthGenParams::SynthGenParams( const SynthGenParams& oth )
{
    *this = oth;
}


SynthGenParams& SynthGenParams::operator=( const SynthGenParams& oth )
{
    if ( &oth == this )
	return *this;

    synthtype_ = oth.synthtype_;
    name_ = oth.name_;
    inpsynthnm_ = oth.inpsynthnm_;
    raypars_ = oth.raypars_;
    synthpars_ = oth.synthpars_;
    anglerg_ = oth.anglerg_;
    attribtype_ = oth.attribtype_;
    wvltnm_ = oth.wvltnm_;

    return *this;
}


bool SynthGenParams::operator== ( const SynthGenParams& oth ) const
{
    bool hassameanglerg = true;
    bool hassameinput = true;
    bool hassameattrib = true;
    if ( oth.isPSBased() )
    {
	hassameanglerg = anglerg_ == oth.anglerg_;
	hassameinput = inpsynthnm_ == oth.inpsynthnm_;
    }
    else if ( oth.isAttribute() )
    {
	hassameinput = inpsynthnm_ == oth.inpsynthnm_;
	hassameattrib = attribtype_ == oth.attribtype_;
    }

    return isPreStack() == oth.isPreStack() && wvltnm_ == oth.wvltnm_ &&
	   raypars_ == oth.raypars_ && synthpars_ == oth.synthpars_ &&
	   hassameanglerg && hassameinput && hassameattrib;
}


bool SynthGenParams::operator!= ( const SynthGenParams& oth ) const
{
    return !(*this == oth);
}


bool SynthGenParams::isOK() const
{
    if ( isRawOutput() )
	return !wvltnm_.isEmpty();
    else if ( isPSBased() )
	return !inpsynthnm_.isEmpty() && !anglerg_.isUdf();
    else if ( isAttribute() )
	return !inpsynthnm_.isEmpty();

    return true;
}


void SynthGenParams::setDefaultValues()
{
    if ( isRawOutput() )
    {
	const BufferString defrtnm( RayTracer1D::factory().getDefaultName() );
	raypars_.set( sKey::Type(), defrtnm.isEmpty()
		       ? VrmsRayTracer1D::sFactoryKeyword() : defrtnm.str() );
	const BufferString defsyntgennm =
			Seis::SynthGenerator::factory().getDefaultName();
	synthpars_.set( sKey::Type(), defsyntgennm.isEmpty()
			? Seis::SynthGeneratorBasic::sFactoryKeyword()
			: defsyntgennm.str() );
    }

    if ( isZeroOffset() )
	RayTracer1D::setIOParsToZeroOffset( raypars_ );
    else if ( isPreStack() )
    {
	const StepInterval<float> offsetrg = RayTracer1D::sDefOffsetRange();
	TypeSet<float> offsets;
	for ( int idx=0; idx<offsetrg.nrSteps()+1; idx++ )
	    offsets += offsetrg.atIndex( idx );
	raypars_.set( RayTracer1D::sKeyOffset(), offsets );
	raypars_.set( RayTracer1D::sKeyOffsetInFeet(), SI().xyInFeet() );
    }

    if ( isRawOutput() )
    {
	const TranslatorGroup& wvlttrgrp = WaveletTranslatorGroup::theInst();
	const BufferString keystr( wvlttrgrp.getSurveyDefaultKey() );
	MultiID wvltkey;
	PtrMan<IOObj> wvltobj = SI().getPars().get( keystr, wvltkey )
			      ? IOM().get( wvltkey )
			      : IOM().getFirst( wvlttrgrp.ioCtxt() );
	if ( wvltobj )
	    setWavelet( wvltobj->name() );
    }

    if ( synthtype_ == AngleStack || synthtype_ == AVOGradient )
    {
	const PreStack::AngleCompParams anglepars;
	anglerg_.start = anglepars.anglerange_.start;
	anglerg_.stop = anglepars.anglerange_.stop;
    }
    else
	anglerg_ = Interval<float>::udf();

    createName( name_ );
}


const char* SynthGenParams::getWaveletNm() const
{
    return wvltnm_.buf();
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
    par.set( sKeySynthType(), toString(synthtype_) );
    if ( isRawOutput() )
    {
	par.set( sKeyWaveLetName(), wvltnm_ );
	IOPar raypar, synthpar;
	raypar.mergeComp( raypars_, sKeyRayPar() );
	synthpar.mergeComp( synthpars_, sKeySynthPar() );
	par.merge( raypar );
	par.merge( synthpar );
    }
    else if ( needsInput() )
    {
	par.set( sKeyInput(), inpsynthnm_ );
	if ( synthtype_ == AngleStack || synthtype_ == AVOGradient )
	    par.set( sKeyAngleRange(), anglerg_ );
	else if ( synthtype_ == InstAttrib )
	    par.set( sKey::Attribute(),
			     Attrib::Instantaneous::toString( attribtype_) );
    }
}


void SynthGenParams::usePar( const IOPar& par )
{
    BufferString nm;
    if ( par.get(sKey::Name(),nm) && !nm.isEmpty() )
	name_ = nm;

    if ( par.hasKey(sKeyIsPreStack()) ) //Legacy
    {
	bool isps = false;
	par.getYN( sKeyIsPreStack(), isps );
	if ( !isps && hasOffsets() )
	    synthtype_ = AngleStack;
	else if ( isps )
	    synthtype_ = PreStack;
	else
	    synthtype_ = ZeroOffset;
    }
    else
    {
	BufferString typestr;
	parseEnum( par, sKeySynthType(), synthtype_ );
    }

    if ( isRawOutput() )
    {
	par.get( sKeyWaveLetName(), wvltnm_ );
	PtrMan<IOPar> raypar = par.subselect( sKeyRayPar() );
	if ( raypar )
	    cleanRayPar( *raypar, raypars_ );

	PtrMan<IOPar> synthpar = par.subselect( sKeySynthPar() );
	if ( synthpar )
	    synthpars_.merge( *synthpar.ptr() );
	else if ( raypar )
	    setSynthGenPar( *raypar, synthpars_ );

	MultiID wvltkey;
	if ( synthpars_.get(sKey::WaveletID(),wvltkey) )
	{
	    PtrMan<IOObj> wvltobj = IOM().get( wvltkey );
	    if ( wvltobj && wvltnm_ != wvltobj->name() )
	    {
		wvltnm_.set( wvltobj->name() );
		createName( name_ );
	    }
	}

	inpsynthnm_.setEmpty();
	anglerg_ = Interval<float>::udf();
    }
    else
    {
	raypars_.setEmpty();
	synthpars_.setEmpty();
	if ( needsInput() )
	{
	    par.get( sKeyInput(), inpsynthnm_ );
	    if ( synthtype_ == AngleStack || synthtype_ == AVOGradient )
		par.get( sKeyAngleRange(), anglerg_ );
	    else if ( synthtype_ == InstAttrib )
	    {
		BufferString attribstr;
		par.get( sKey::Attribute(), attribstr );
		Attrib::Instantaneous::parseEnum( attribstr, attribtype_ );
	    }
	}
    }
}


void SynthGenParams::createName( BufferString& nm ) const
{
    if ( isPSBased() )
    {
	nm = toString( synthtype_ );
	nm += " ["; nm += anglerg_.start; nm += ",";
	nm += anglerg_.stop; nm += "] degrees";
	return;
    }
    else if ( isAttribute() )
    {
	nm = Attrib::Instantaneous::toString( attribtype_ );
	nm += " ["; nm += inpsynthnm_; nm += "]";
	return;
    }
    else if ( !isRawOutput() )
	return;

    nm = wvltnm_;
    TypeSet<float> offset;
    if ( !raypars_.get(RayTracer1D::sKeyOffset(),offset) && isZeroOffset() )
	offset += 0.f;

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
	    if ( synthpars_.getYN(Seis::SynthGenBase::sKeyNMO(),nmocorrected) &&
		 !nmocorrected )
		nm += " uncorrected";
	}
    }
}


void SynthGenParams::setWavelet( const char* wvltnm )
{
    if ( !isRawOutput() || wvltnm_ == wvltnm )
	return;

    wvltnm_.set( wvltnm );
    PtrMan<IOObj> wvltobj = Wavelet::getIOObj( wvltnm_.buf() );
    if ( !wvltobj )
	return;

    synthpars_.set( sKey::WaveletID(), wvltobj->key() );
}


void SynthGenParams::setWavelet( const Wavelet& wvlt )
{
    if ( !isRawOutput() || wvltnm_ == wvlt.name() )
	return;

    wvltnm_.set( wvlt.name() );
    PtrMan<IOObj> wvltobj = Wavelet::getIOObj( wvltnm_.buf() );
    if ( !wvltobj )
	return;

    synthpars_.set( sKey::WaveletID(), wvltobj->key() );
}


namespace Strat
{

static BufferStringSet fillSynthSeisKeys()
{
    BufferStringSet rmkeys;
    rmkeys.add( sKey::WaveletID() )
	  .add( Seis::SynthGenBase::sKeyConvDomain() )
	  .add( Seis::SynthGenBase::sKeyTimeRefs() )
	  .add( Seis::SynthGenBase::sKeyNMO() )
	  .add( Seis::SynthGenBase::sKeyMuteLength() )
	  .add( Seis::SynthGenBase::sKeyStretchLimit() )
	  .add( "Internal Multiples" )
	  .add( "Surface Reflection coef" );
    return rmkeys;
}

}

void SynthGenParams::cleanRayPar( const IOPar& iop, IOPar& raypar )
{
    raypar.merge( iop );
    const BufferStringSet rmkeys = Strat::fillSynthSeisKeys();
    for ( const auto* rmky : rmkeys )
	raypar.removeWithKey( rmky->buf() );
}


void SynthGenParams::setSynthGenPar( const IOPar& iop, IOPar& synthpar )
{
    const BufferStringSet rmkeys = Strat::fillSynthSeisKeys();
    for ( const auto* rmky : rmkeys )
    {
	BufferString valstr;
	if ( iop.get(rmky->buf(),valstr) && !valstr.isEmpty() )
	    synthpar.set( rmky->buf(), valstr );
    }
}
