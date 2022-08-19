#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocitymod.h"
#include "trckeyzsampling.h"
#include "thread.h"
#include "paralleltask.h"
#include "veldesc.h"
#include "uistring.h"

class IOObj;
class SeisTrc;
class SeisTrcReader;
class SeisTrcWriter;
class SeisSequentialWriter;

namespace Vel
{

/*!Reads in a volume with eather Vrms or Vint, and writes out a volume
   with eather Vrms or Vint. */

mExpClass(Velocity) VolumeConverter : public ParallelTask
{ mODTextTranslationClass(VolumeConverter);
public:
				VolumeConverter(const IOObj& input,
						const IOObj& output,
						const TrcKeySampling& ranges,
						const VelocityDesc& outdesc);
				~VolumeConverter();

    uiString			errMsg() const { return errmsg_; }

    static const char*		sKeyInput();
    static const char*		sKeyOutput();

protected:
    od_int64			nrIterations() const override
				{ return totalnr_; }

    bool			doPrepare(int) override;
    bool			doFinish(bool) override;
    bool			doWork(od_int64,od_int64,int) override;
    uiString			uiNrDoneText() const override
				{ return tr("Traces written"); }

    char			getNewTrace(SeisTrc&,int threadidx);

    od_int64			totalnr_;
    IOObj*			input_;
    IOObj*			output_;
    VelocityDesc		velinpdesc_;
    VelocityDesc		veloutpdesc_;
    TrcKeySampling			tks_;
    uiString			errmsg_;

    SeisTrcReader*		reader_;
    SeisTrcWriter*		writer_;
    SeisSequentialWriter*	sequentialwriter_;

    Threads::ConditionVar	lock_;
};

} // namespace Vel
