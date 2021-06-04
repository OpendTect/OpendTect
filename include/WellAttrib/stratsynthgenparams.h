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
			SynthGenParams();

    enum SynthType	{ PreStack, ZeroOffset, StratProp, AngleStack,
			  AVOGradient };
    			mDeclareEnumUtils(SynthType);

    SynthType		synthtype_;
    BufferString	name_;
    BufferString	inpsynthnm_;
    IOPar		raypars_;
    BufferString	wvltnm_;
    Interval<float>	anglerg_;

    static const char*	sKeyInvalidInputPS()	{ return "Invalid Input"; }
    
    bool		hasOffsets() const;
    bool		isPreStack() const 	{ return synthtype_==PreStack; }
    bool		isPSBased() const
			{ return synthtype_==AngleStack ||
				 synthtype_==AVOGradient; }
    void		createName(BufferString&) const;
    			//!<Create name from wvlt and raypars
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
    void		setDefaultValues();

bool operator==( const SynthGenParams& gp ) const
{
    bool hassameanglerg = true;
    bool hassameinput = true;
    if ( gp.isPSBased() )
    {
	hassameanglerg = anglerg_==gp.anglerg_;
	hassameinput = inpsynthnm_==gp.inpsynthnm_;
    }

    return isPreStack()==gp.isPreStack() && wvltnm_==gp.wvltnm_ &&
	   raypars_==gp.raypars_ && hassameanglerg && hassameinput; }

};


