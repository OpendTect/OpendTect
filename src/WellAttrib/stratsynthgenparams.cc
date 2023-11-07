/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratsynthgenparams.h"

#include "genc.h"
#include "ioman.h"
#include "prestackanglemute.h"
#include "propertyref.h"
#include "raytrace1d.h"
#include "reflcalc1d.h"
#include "synthseis.h"
#include "survinfo.h"
#include "wavelet.h"
#include "waveletio.h"

static const char* sKeyIsPreStack()		{ return "Is Pre Stack"; }
const char* SynthGenParams::sKeyRayPar()
{ return RayTracer1D::sKeyRayPar(); }
const char* SynthGenParams::sKeyReflPar()
{ return ReflCalc1D::sKeyReflPar(); }
const char* SynthGenParams::sKeySynthPar()
{ return Seis::RaySynthGenerator::sKeySynthPar(); }


mDefineEnumUtils(SynthGenParams,SynthType,"Synthetic Type")
{
    "Zero Offset Stack",
    "Extended Elastic Stack",
    "Elastic Gather",
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


SynthGenParams::~SynthGenParams()
{
    delete reqtype_;
}


SynthGenParams& SynthGenParams::operator=( const SynthGenParams& oth )
{
    if ( &oth == this )
	return *this;

    synthtype_ = oth.synthtype_;
    name_ = oth.name_;
    inpsynthnm_ = oth.inpsynthnm_;
    raypars_ = oth.raypars_;
    reflpars_ = oth.reflpars_;
    delete reqtype_;
    reqtype_ = oth.reqtype_ ? new RefLayer::Type(*oth.reqtype_) : nullptr;
    synthpars_ = oth.synthpars_;
    anglerg_ = oth.anglerg_;
    attribtype_ = oth.attribtype_;
    wvltnm_ = oth.wvltnm_;

    return *this;
}


bool SynthGenParams::operator== ( const SynthGenParams& oth ) const
{
    if ( this == &oth )
	return true;

    return name_ == oth.name_ && hasSamePars( oth );
}


bool SynthGenParams::hasSamePars( const SynthGenParams& oth ) const
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

    return synthtype_ == oth.synthtype_ && wvltnm_ == oth.wvltnm_ &&
	   raypars_ == oth.raypars_ && reflpars_ == oth.reflpars_ &&
	   synthpars_ == oth.synthpars_ &&
	   hassameanglerg && hassameinput && hassameattrib;
}


bool SynthGenParams::operator!= ( const SynthGenParams& oth ) const
{
    return !(*this == oth);
}


bool SynthGenParams::isOK() const
{
    if ( isRawOutput() )
    {
	if ( wvltnm_.isEmpty() )
	    return false;
    }
    else if ( needsInput() )
    {
	if ( inpsynthnm_.isEmpty() || inpsynthnm_ == sKeyInvalidInputPS() )
	    return false;

	if ( isPSBased() && anglerg_.isUdf() )
	    return false;
	if ( isAttribute() &&
	     attribtype_ == Attrib::Instantaneous::RotatePhase )
	    return false;
    }

    return !name_.isEmpty();
}


void SynthGenParams::setDefaultValues()
{
    if ( isRawOutput() )
    {
	if ( isZeroOffset() || isElasticStack() || isElasticGather() )
	{
	    const BufferString defnm( isZeroOffset()
				     ? AICalc1D::sFactoryKeyword()
				     : ReflCalc1D::factory().getDefaultName() );
	    reflpars_.set( sKey::Type(), defnm.isEmpty()
			   ? AICalc1D::sFactoryKeyword() : defnm.str() );
	    raypars_.setEmpty();
	}
	else
	{
	    const BufferString defnm( RayTracer1D::factory().getDefaultName() );
	    raypars_.set( sKey::Type(), defnm.isEmpty()
			   ? VrmsRayTracer1D::sFactoryKeyword() : defnm.str() );
	    reflpars_.setEmpty();
	}

	const BufferString defsyntgennm =
			Seis::SynthGenerator::factory().getDefaultName();
	synthpars_.set( sKey::Type(), defsyntgennm.isEmpty()
			? Seis::SynthGeneratorBasic::sFactoryKeyword()
			: defsyntgennm.str() );
    }

    const SurveyInfo& si = IOMan::isOK() ? SI() : SurveyInfo::empty();
    if ( isZeroOffset() )
    {
	ReflCalc1D::setIOParsToSingleAngle( reflpars_ );
    }
    else if ( isElasticStack() )
    {
	const Seis::OffsetType angtyp = Seis::AngleDegrees;
	ReflCalc1D::setIOParsToSingleAngle( reflpars_,
					    ReflCalc1D::sDefAngle(angtyp) );
    }
    else if ( isElasticGather() )
    {
	const Seis::OffsetType angtyp = Seis::AngleDegrees;
	const StepInterval<float> anglerg = ReflCalc1D::sDefAngleRange(angtyp);
	TypeSet<float> angles;
	for ( int idx=0; idx<anglerg.nrSteps()+1; idx++ )
	    angles += anglerg.atIndex( idx );
	reflpars_.set( ReflCalc1D::sKeyAngle(), angles );
	reflpars_.setYN( ReflCalc1D::sKeyAngleInDegrees(),
			 angtyp == Seis::AngleDegrees );
    }
    else if ( isPreStack() )
    {
	const StepInterval<float> offsetrg = RayTracer1D::sDefOffsetRange();
	TypeSet<float> offsets;
	for ( int idx=0; idx<offsetrg.nrSteps()+1; idx++ )
	    offsets += offsetrg.atIndex( idx );
	raypars_.set( RayTracer1D::sKeyOffset(), offsets );
	raypars_.set( RayTracer1D::sKeyOffsetInFeet(), si.xyInFeet() );
    }

    if ( isRawOutput() )
    {
	setReqType();
	if ( IOMan::isOK() )
	{
	    const TranslatorGroup& wvlttrgrp =WaveletTranslatorGroup::theInst();
	    const BufferString keystr( wvlttrgrp.getSurveyDefaultKey() );
	    MultiID wvltkey;
	    PtrMan<IOObj> wvltobj = si.getPars().get( keystr, wvltkey )
				  ? IOM().get( wvltkey )
				  : IOM().getFirst( wvlttrgrp.ioCtxt() );
	    if ( wvltobj )
		setWavelet( wvltobj->name() );
	}
    }
    else
	deleteAndNullPtr( reqtype_ );

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


void SynthGenParams::setReqType()
{
    uiString msg;
    if ( !reflpars_.isEmpty() )
    {
	PtrMan<ReflCalc1D> calc = ReflCalc1D::createInstance( reflpars_, msg );
	if ( calc && calc->isOK() )
	{
	    if ( !reqtype_ )
		reqtype_ = new RefLayer::Type;
	    *reqtype_ = RefLayer::getType( calc->needsSwave(),
					   calc->needFracRho(),
					   calc->needFracAzi() );
	}
    }
    else if ( !raypars_.isEmpty() )
    {
	PtrMan<RayTracer1D> calc = RayTracer1D::createInstance( raypars_, msg );
	if ( calc && msg.isEmpty() )
	{
	    if ( !reqtype_ )
		reqtype_ = new RefLayer::Type;
	    *reqtype_ = RefLayer::getType( calc->needsSwave(), false, false );
	}
    }
}


const char* SynthGenParams::getWaveletNm() const
{
    return wvltnm_.buf();
}


MultiID SynthGenParams::getWaveletID() const
{
    MultiID wvltkey;
    if ( synthpars_.get(sKey::WaveletID(),wvltkey) )
    {
	PtrMan<IOObj> wvltobj = IOM().get( wvltkey );
	if ( wvltobj )
	    wvltkey = wvltobj->key();

    }

    return wvltkey;
}


const IOPar* SynthGenParams::reflPars() const
{
    if ( !isRawOutput() )
	return nullptr;

    return isPreStack() ? &raypars_ : &reflpars_;
}


const PropertyRef* SynthGenParams::getRef(const PropertyRefSelection& prs) const
{
    if ( !isStratProp() )
	return nullptr;

    BufferString propnm( name_ );
    propnm.unEmbed( '[', ']' );
    return prs.getByName( propnm.buf(), false );
}


bool SynthGenParams::hasOffsets() const
{
    TypeSet<float> offsets;
    raypars_.get( RayTracer1D::sKeyOffset(), offsets );
    return offsets.size()>1;
}


bool SynthGenParams::isCorrected() const
{
    bool corrected = true;
    synthpars_.getYN( Seis::SynthGenBase::sKeyNMO(), corrected );

    return corrected;
}


Seis::OffsetType SynthGenParams::offsetType() const
{
    Seis::OffsetType ret = SI().xyInFeet() ? Seis::OffsetFeet
					   : Seis::OffsetMeter;
    bool offsetsinfeet = SI().xyInFeet();
    if ( raypars_.getYN(RayTracer1D::sKeyOffsetInFeet(),offsetsinfeet) )
	ret = offsetsinfeet ? Seis::OffsetFeet
			    : Seis::OffsetMeter;

    return ret;
}


void SynthGenParams::fillPar( IOPar& par ) const
{
    par.set( sKey::Name(), name_ );
    par.set( sKeySynthType(), toString(synthtype_) );
    if ( isRawOutput() )
    {
	par.set( sKeyWaveLetName(), wvltnm_ );
	IOPar raypar, reflpar, synthpar;
	raypar.mergeComp( raypars_, sKeyRayPar() );
	reflpar.mergeComp( reflpars_, sKeyReflPar() );
	synthpar.mergeComp( synthpars_, sKeySynthPar() );
	par.merge( raypar );
	par.merge( reflpar );
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

	PtrMan<IOPar> relfpar = par.subselect( sKeyReflPar() );
	if ( relfpar )
	{
	    raypars_.setEmpty();
	    reflpars_.merge( *relfpar.ptr() );
	}
	else
	{
	    TypeSet<float> offsets;
	    raypars_.get( RayTracer1D::sKeyOffset(), offsets );
	    if ( offsets.isEmpty() ||
		 (offsets.size()==1 && mIsZero(offsets[0],1e-3f)) )
	    {
		raypars_.setEmpty();
		reflpars_.set( sKey::Type(), AICalc1D::sFactoryKeyword() );
	    }
	    else
		reflpars_.setEmpty();
	}

	PtrMan<IOPar> synthpar = par.subselect( sKeySynthPar() );
	if ( synthpar )
	    synthpars_.merge( *synthpar.ptr() );
	else if ( raypar )
	    setSynthGenPar( *raypar, synthpars_ );

	setReqType();
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
	reflpars_.setEmpty();
	synthpars_.setEmpty();
	deleteAndNullPtr( reqtype_ );
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
    if ( isZeroOffset() )
    {
	nm.addSpace().add( "Zero-Offset" );
	return;
    }

    if ( isElasticStack() )
    {
	Seis::OffsetType angtyp = Seis::AngleDegrees;
	bool isindegrees = angtyp == Seis::AngleDegrees;
	float angle = ReflCalc1D::sDefAngle( angtyp );
	if ( reflpars_.get(ReflCalc1D::sKeyAngle(),angle) &&
	     reflpars_.getYN(ReflCalc1D::sKeyAngleInDegrees(),isindegrees) &&
	     !isindegrees )
	    angle *= mRad2DegF;
	nm.addSpace().add( "chi=" ).add( angle );
	return;
    }

    if ( isElasticGather() )
    {
	Seis::OffsetType angtyp = Seis::AngleDegrees;
	bool isindegrees = angtyp == Seis::AngleDegrees;
	TypeSet<float> angles;
	if ( reflpars_.get(ReflCalc1D::sKeyAngle(),angles) && !angles.isEmpty())
	{
	    reflpars_.getYN( ReflCalc1D::sKeyAngleInDegrees(), isindegrees );
	    if ( !isindegrees )
	    {
		for (  auto& angle : angles )
		    angle *= mRad2DegF;
	    }
	}
	else
	{
	    pErrMsg( "Should not be reached" );
	    const StepInterval<float> anglerg =
				ReflCalc1D::sDefAngleRange( angtyp );
	    for ( int idx=0; idx<anglerg.nrSteps()+1; idx++ )
		angles += anglerg.atIndex( idx );
	}

	nm.addSpace().add( "Angle" ).addSpace()
	  .add( ::toString( angles.first() ) );
	nm.add( "-" ).add( angles.last() );
    }

    TypeSet<float> offset;
    if ( !raypars_.get(RayTracer1D::sKeyOffset(),offset) && isZeroOffset() )
    {
	pErrMsg( "Should not be reached" );
	offset += 0.f;
    }

    const int offsz = offset.size();
    if ( offsz )
    {
	nm.addSpace().add( "Offset" ).addSpace()
	  .add( ::toString( offset.first() ) );
	if ( offsz > 1 )
	{
	    nm.add( "-" ).add( offset.last() );
	    bool nmocorrected = true;
	    if ( synthpars_.getYN(Seis::SynthGenBase::sKeyNMO(),nmocorrected) &&
		 !nmocorrected )
		nm.addSpace().add( "uncorrected" );
	}
    }
}


void SynthGenParams::setWavelet( const char* wvltnm )
{
    if ( !isRawOutput() || wvltnm_ == wvltnm )
	return;

    wvltnm_.set( wvltnm );
    PtrMan<IOObj> wvltobj = Wavelet::getIOObj( wvltnm_.buf() );
    if ( wvltobj )
	synthpars_.set( sKey::WaveletID(), wvltobj->key() );
    else if ( synthpars_.isPresent(sKey::WaveletID()) )
	synthpars_.removeWithKey( sKey::WaveletID() );
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

} // namespace Strat

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
