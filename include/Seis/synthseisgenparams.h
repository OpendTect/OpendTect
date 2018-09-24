#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno/Satyaki / Bert
 Date:		July 2013 / August 2018
________________________________________________________________________

-*/

#include "seismod.h"
#include "iopar.h"
#include "enums.h"
#include "dbkey.h"


namespace SynthSeis
{

enum SyntheticType { ZeroOffset, PreStack, StratProp, AngleStack, AVOGradient };
mDeclareNameSpaceEnumUtils(Seis,SyntheticType);

mExpClass(Seis) GenParams
{
public:

    typedef SyntheticType Type;

			GenParams();
    bool		operator==(const GenParams&) const;
			mImplSimpleIneqOper(GenParams);

    static bool		isZeroOffset( Type st )	{ return st==ZeroOffset; }
    static bool		isPS( Type st )		{ return st==PreStack; }
    static bool		isStratProp( Type st )	{ return st==StratProp; }
    static bool		isPSPostProc( Type st ) { return st>StratProp; }

    Type		type_;
    BufferString	name_;
    DBKey		wvltid_;
    IOPar		raypars_;
    BufferString	inpsynthnm_;	    //!< for PSPostProc only
    Interval<int>	anglerg_;	    //!< for PSPostProc only, in degrees

    static const char*	sKeyInvalidInputPS()	{ return "Invalid Input"; }

    bool		isPS() const	     { return isPS(type_); }
    bool		isStratProp() const  { return isStratProp(type_); }
    bool		isPSPostProc() const { return isPSPostProc(type_); }
    bool		isZeroOffset() const { return isZeroOffset(type_); }
    bool		hasOffsets() const;
    BufferString	createName() const;	//!< from wvlt and raypars
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
    void		setDefaultValues();
    BufferString	waveletName() const;
    void		setWaveletName(const char*);

protected:

    BufferString	fallbackwvltnm_; //!< if wvlt not stored in DB

};

} // namespace SynthSeis
