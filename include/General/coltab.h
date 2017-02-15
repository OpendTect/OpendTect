#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2007
________________________________________________________________________

-*/


#include "generalmod.h"
#include "color.h"
#include "ranges.h"

/*!\brief %Color %Table common stuff */

namespace ColTab
{

    enum SeqUseMode { UnflippedSingle, UnflippedCyclic, FlippedSingle,
					FlippedCyclic };

    mGlobal(General) bool		isFlipped(SeqUseMode);
    mGlobal(General) bool		isCyclic(SeqUseMode);
    mGlobal(General) SeqUseMode		getSeqUseMode(bool flipped,bool cyclic);
    mGlobal(General) BufferString	toString(SeqUseMode);
    mGlobal(General) void		toPar(SeqUseMode,IOPar&);
    mGlobal(General) bool		fromPar(const IOPar&,SeqUseMode&);
    mGlobal(General) inline const char*	sKeySeqUseMode()
					{ return "Color Table Use Mode"; }

    mGlobal(General) const char*	defSeqName();
    mGlobal(General) Interval<float>	defClipRate();
    mGlobal(General) float		defSymMidval();
    mGlobal(General) bool		defAutoSymmetry();
    mGlobal(General) void		setMapperDefaults(Interval<float> cr,
						      float sm,bool autosym,
						      bool histeq=false);
    mGlobal(General) bool		defHistEq();

}
