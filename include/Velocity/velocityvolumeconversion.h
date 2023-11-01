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
class UnitOfMeasure;
namespace ZDomain { class Info; }

namespace Vel
{

/*!Reads in a volume with either Vrms or Vint, and writes out a volume
   with either Vrms or Vint. */

mExpClass(Velocity) VolumeConverterNew : public ParallelTask
{ mODTextTranslationClass(VolumeConverter);
public:
				VolumeConverterNew(const IOObj& input,
						const IOObj& output,
						const TrcKeySampling&,
						const VelocityDesc& outdesc,
						double srd,
						const UnitOfMeasure* srduom);
				~VolumeConverterNew();

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


// Old task, do not use it

mExpClass(Velocity) VolumeConverter : public ParallelTask
{ mODTextTranslationClass(VolumeConverter);
public:
				mDeprecated("Use VolumeConverterNew")
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
