#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocitymod.h"

#include "paralleltask.h"
#include "trckeyzsampling.h"
#include "thread.h"
#include "uistring.h"

class IOObj;
class SeisTrc;
class SeisTrcReader;
class SeisTrcWriter;
class SeisSequentialWriter;
class UnitOfMeasure;
class VelocityDesc;
namespace ZDomain { class Info; }

namespace Vel
{

/*!Reads in a volume with either Vrms or Vint, and writes out a volume
   with either Vrms or Vint. */

mExpClass(Velocity) VolumeConverter : public ParallelTask
{ mODTextTranslationClass(VolumeConverter);
public:
				VolumeConverter(const IOObj& input,
						const IOObj& output,
						const TrcKeySampling&,
						const VelocityDesc& outdesc,
						double srd,
						const UnitOfMeasure* srduom);
				~VolumeConverter();

    uiString			uiMessage() const override;
    uiString			uiNrDoneText() const override;

    static const char*		sKeyInput();
    static const char*		sKeyOutput();

private:
    od_int64			nrIterations() const override;

    bool			doPrepare(int) override;
    bool			doFinish(bool) override;
    bool			doWork(od_int64,od_int64,int) override;

    void			setRanges();
    char			getNewTrace(SeisTrc&,int threadidx);

    uiString			msg_;
    od_int64			totalnr_ = -1;

    const IOObj&		input_;
    const IOObj&		output_;
    const ZDomain::Info*	zdomaininfo_	    = nullptr;
    VelocityDesc&		velinpdesc_;
    VelocityDesc&		veloutpdesc_;
    double			srd_;
    const UnitOfMeasure*	srduom_;
    TrcKeySampling		tks_;

    SeisTrcReader*		reader_		    = nullptr;
    SeisTrcWriter*		writer_		    = nullptr;
    SeisSequentialWriter*	sequentialwriter_   = nullptr;

    Threads::ConditionVar	lock_;
};

} // namespace Vel
