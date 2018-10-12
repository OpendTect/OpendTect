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
#include "ranges.h"
#include "trckeysampling.h"
#include "threadlock.h"

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
	DBKey			outputmutemid_;
	TrcKeySampling		tks_;
    };

    static const char*		sKeyMuteDefID() { return "Mute Def"; }

    virtual uiString		message() const final	{ return msg_; }

    AngleMuteCompPars&		params();
    const AngleMuteCompPars&	params() const;

    virtual void		fillPar(IOPar&) const final;
    virtual bool		usePar(const IOPar&) final;

protected:

    virtual od_int64		nrIterations() const final;

    MuteDef&			outputmute_;
    Threads::Lock		lock_;

private:

    virtual bool		doPrepare(int) final;
    virtual bool		doWork(od_int64,od_int64,int) final;
    virtual bool		doFinish(bool) final;
};

} // namespace PreStack
