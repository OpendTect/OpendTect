#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          June 2011
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"

#include "paralleltask.h"
#include "prestackanglemute.h"

class TrcKeySampling;

namespace Vel { class VolumeFunctionSource; }

namespace PreStack
{

class MuteDef;

/*!
\brief Computes angle mute.
*/

mExpClass(PreStackProcessing) AngleMuteComputer : public ParallelTask
						, public AngleMuteBase
{ mODTextTranslationClass(AngleMuteComputer);
public:
				AngleMuteComputer();
				~AngleMuteComputer();

    mStruct(PreStackProcessing) AngleMuteCompPars : public AngleCompParams
    {
	MultiID			outputmutemid_;
	TrcKeySampling		tks_;
    };

    static const char*		sKeyMuteDefID() { return "Mute Def"; }

    od_int64			nrIterations() const;
    bool			doPrepare(int);
    bool			doWork(od_int64 start,od_int64 stop,int);
    bool			doFinish(bool success);

    uiString			uiMessage() const;
    uiString			errMsg() const		{ return errmsg_; }

    AngleMuteCompPars&		params();
    const AngleMuteCompPars&	params() const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:

    uiString			errmsg_;
    MuteDef&			outputmute_;
    Threads::Lock		lock_;
};

} // namespace PreStack

