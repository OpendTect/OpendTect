#ifndef stratsynthgenparams_h
#define stratsynthgenparams_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno/Satyaki
 Date:		July 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "iopar.h"
#include "enums.h"


mStruct(WellAttrib) SynthGenParams
{
			SynthGenParams();

    enum SynthType	{ PreStack, ZeroOffset, AngleStack, AVOGradient };
    			DeclareEnumUtils(SynthType);

    SynthType		synthtype_;
    BufferString	name_;
    BufferString	inpsynthnm_;
    IOPar		raypars_;
    BufferString	wvltnm_;
    Interval<float>	anglerg_;
    
    bool		hasOffsets() const;
    bool		isPreStack() const 	{ return synthtype_==PreStack; }
    void		createName(BufferString&) const;
    			//!<Create name from wvlt and raypars
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

bool operator==( const SynthGenParams& gp )
{ return isPreStack()==gp.isPreStack() && wvltnm_==gp.wvltnm_ &&
         raypars_==gp.raypars_; }

};


#endif
