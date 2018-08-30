#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno/Satyaki
 Date:		July 2013
________________________________________________________________________

-*/

#include "seismod.h"
#include "iopar.h"
#include "enums.h"
#include "dbkey.h"


mExpClass(Seis) SynthGenParams
{
public:

			SynthGenParams();
    bool		operator==(const SynthGenParams&) const;
			mImplSimpleIneqOper(SynthGenParams);

    enum SynthType	{ PreStack, ZeroOffset, StratProp, AngleStack,
			  AVOGradient };
			mDeclareEnumUtils(SynthType);

    SynthType		synthtype_;
    IOPar		raypars_;
    DBKey		wvltid_;
    BufferString	name_;
    BufferString	inpsynthnm_;
    Interval<float>	anglerg_;

    static const char*	sKeyInvalidInputPS()	{ return "Invalid Input"; }

    bool		hasOffsets() const;
    bool		isPreStack() const	{ return synthtype_==PreStack; }
    bool		isPSBased() const	{ return synthtype_>StratProp; }
    BufferString	createName() const;	//!< from wvlt and raypars
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
    void		setDefaultValues();
    BufferString	waveletName() const;
    void		setWaveletName(const char*);

protected:

    BufferString	fallbackwvltnm_; //!< if wvlt not stored in DB

};
