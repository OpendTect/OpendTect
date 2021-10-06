#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno/Satyaki
 Date:		July 2013
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "iopar.h"
#include "enums.h"


mStruct(WellAttrib) SynthGenParams
{
    enum SynthType	{ PreStack, ZeroOffset, StratProp, AngleStack,
			  AVOGradient };
			mDeclareEnumUtils(SynthType);

			SynthGenParams( SynthType tp=ZeroOffset );

    bool		operator== (const SynthGenParams&) const;
    bool		operator!= (const SynthGenParams&) const;

    bool		isOK() const;

    SynthType		synthtype_;
    BufferString	name_;
    BufferString	inpsynthnm_;
    IOPar		raypars_;
    BufferString	wvltnm_;
    Interval<float>	anglerg_;

    bool		hasOffsets() const;
    bool		isPreStack() const	{ return synthtype_==PreStack; }
    bool		isPSBased() const
			{ return synthtype_==AngleStack ||
				 synthtype_==AVOGradient; }
    void		createName(BufferString&) const;
			//!<Create name from wvlt and raypars
    void		fillPar(IOPar&) const;

    void		usePar(const IOPar&);

    static const char*	sKeyInvalidInputPS()	{ return "Invalid Input"; }

private:

    void		setDefaultValues();

};


