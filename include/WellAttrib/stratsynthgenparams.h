#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellattribmod.h"

#include "enums.h"
#include "instantattrib.h"
#include "iopar.h"

class PropertyRef;
class PropertyRefSelection;
class Wavelet;


mExpClass(WellAttrib) SynthGenParams
{
public:
    enum SynthType	{ ZeroOffset, PreStack, StratProp, AngleStack,
			  AVOGradient, InstAttrib };
			mDeclareEnumUtils(SynthType);

			SynthGenParams( SynthType tp=ZeroOffset );
			SynthGenParams(const SynthGenParams&);

    SynthGenParams&	operator= (const SynthGenParams&);
    bool		operator== (const SynthGenParams&) const;
    bool		operator!= (const SynthGenParams&) const;
    bool		hasSamePars(const SynthGenParams&) const;
			//<! Everything but the name

    bool		isOK() const;

    SynthType		synthtype_;
    BufferString	name_;
    BufferString	inpsynthnm_;
    IOPar		raypars_;
    IOPar		synthpars_;
    Interval<float>	anglerg_;
    Attrib::Instantaneous::OutType attribtype_;

    const char*		getWaveletNm() const;
    const PropertyRef*	getRef(const PropertyRefSelection&) const;

    bool		hasOffsets() const;
    bool		isZeroOffset() const { return synthtype_==ZeroOffset; }
    bool		isPreStack() const	{ return synthtype_==PreStack; }
    bool		isCorrected() const;
			//<! Only for PS gathers
    bool		isPSBased() const
			{ return synthtype_==AngleStack ||
				 synthtype_==AVOGradient; }
    bool		canBeAttributeInput() const
			{ return isZeroOffset() || isPSBased(); }
    bool		isStratProp() const   { return synthtype_==StratProp; }
    bool		isAttribute() const   { return synthtype_==InstAttrib; }
    bool		needsInput() const
			{ return isPSBased() || isAttribute(); }
    bool		isRawOutput() const
			{ return !needsInput() && !isStratProp(); }
			/*!<Any type that can be created using
			    Seis::RaySynthGenerator */
    bool		needsSWave() const;
    void		createName(BufferString&) const;
			//!<Create name from wvlt and raypars
    void		setWavelet(const char*);
    void		setWavelet(const Wavelet&);
    void		fillPar(IOPar&) const;

    void		usePar(const IOPar&);

    static const char*	sKeySynthType()		{ return "Synthetic Type"; }
    static const char*	sKeyWaveLetName()	{ return "Wavelet Name"; }
    static const char*	sKeyRayPar();
    static const char*	sKeySynthPar();
    static const char*	sKeyInput()		{ return "Input Synthetic"; }
    static const char*	sKeyAngleRange()	{ return "Angle Range"; }
    static const char*	sKeyInvalidInputPS()	{ return "Invalid Input"; }

private:

    void		setDefaultValues();
    static void		cleanRayPar(const IOPar&,IOPar&);
    static void		setSynthGenPar(const IOPar&,IOPar&);

    BufferString	wvltnm_;

};
