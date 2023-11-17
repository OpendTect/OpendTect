#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellattribmod.h"

#include "ailayer.h"
#include "odcommonenums.h"
#include "instantattrib.h"
#include "iopar.h"

class PropertyRef;
class PropertyRefSelection;
class Wavelet;


mExpClass(WellAttrib) SynthGenParams
{
public:
    enum SynthType	{ ZeroOffset, ElasticStack, ElasticGather, PreStack,
			  StratProp, AngleStack, AVOGradient, InstAttrib,
			  FilteredSynthetic, FilteredStratProp };
			mDeclareEnumUtils(SynthType);

			SynthGenParams(SynthType tp=ZeroOffset);
			SynthGenParams(const SynthGenParams&);
			~SynthGenParams();

    SynthGenParams&	operator= (const SynthGenParams&);
    bool		operator== (const SynthGenParams&) const;
    bool		operator!= (const SynthGenParams&) const;
    bool		hasSamePars(const SynthGenParams&) const;
			//<! Everything but the name

    bool		isOK() const;

    SynthType		synthtype_;
    BufferString	name_;
    BufferString	inpsynthnm_;
    IOPar		synthpars_;
    Interval<float>	anglerg_;
    Attrib::Instantaneous::OutType attribtype_;

    BufferString&	filtertype_();
    int&		windowsz_();
    Interval<float>&	freqrg_();
    BufferString&	filtertype_() const;
    int&		windowsz_() const;
    Interval<float>&	freqrg_() const;

    const char*		getWaveletNm() const;
    MultiID		getWaveletID() const;
    const IOPar*	reflPars() const;
    const PropertyRef*	getRef(const PropertyRefSelection&) const;

    bool		hasOffsets() const;
    bool		isZeroOffset() const { return synthtype_==ZeroOffset; }
    bool		isElasticStack() const
					    { return synthtype_==ElasticStack; }
    bool		isElasticGather() const
					    { return synthtype_==ElasticGather;}
    bool		isPreStack() const	{ return synthtype_==PreStack; }
    bool		isCorrected() const;
			//<! Only for PS gathers
    Seis::OffsetType	offsetType() const;
    bool		isPSBased() const
			{ return synthtype_==AngleStack ||
				 synthtype_==AVOGradient; }
    bool		canBeAttributeInput() const
			{ return isZeroOffset() || isElasticStack() ||
				 isPSBased(); }
    bool		canBeFilterInput() const;
    bool		isStratProp() const   { return synthtype_==StratProp; }
    bool		isAttribute() const   { return synthtype_==InstAttrib; }
    bool		isFilteredSynthetic() const;
    bool		isFilteredStratProp() const;
    bool		isFiltered() const;
    bool		isFilterOK() const;
    bool		needsInput_() const;
    bool		needsInput() const
			{ return isPSBased() || isAttribute(); }
    bool		isRawOutput_() const;
    bool		isRawOutput() const
			{ return !needsInput() && !isStratProp(); }
			/*!<Any type that can be created using
			    Seis::RaySynthGenerator */
    const RefLayer::Type* requiredRefLayerType() const	{ return reqtype_; }
    void		createName(BufferString&) const;
			//!<Create name from wvlt and raypars
    void		setWavelet(const char*);
    void		setWavelet(const Wavelet&);
    void		fillPar(IOPar&) const;

    void		usePar(const IOPar&);

    static const char*	sKeySynthType()		{ return "Synthetic Type"; }
    static const char*	sKeyWaveLetName()	{ return "Wavelet Name"; }
    static const char*	sKeyRayPar();
    static const char*	sKeyReflPar();
    static const char*	sKeySynthPar();
    static const char*	sKeyInput()		{ return "Input Synthetic"; }
    static const char*	sKeyAngleRange()	{ return "Angle Range"; }
    static const char*	sKeyInvalidInputPS()	{ return "Invalid Input"; }
    static const char*	sKeyFreqRange()		{ return "Frequency Range"; }

private:

    void		setDefaultValues();
    void		setReqType();
    static void		cleanRayPar(const IOPar&,IOPar&);
    static void		setSynthGenPar(const IOPar&,IOPar&);

    BufferString	wvltnm_;
    IOPar		raypars_;
    IOPar		reflpars_;
    RefLayer::Type*	reqtype_ = nullptr;

};
