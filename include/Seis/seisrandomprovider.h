#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "refcount.h"

#include "binidvalset.h"
#include "multidimstorage.h"
#include "seistrc.h"
#include "thread.h"

class SeisTrcTranslator;
class SeisTrcReader;

/* Request a trace anywhere, and it will become available to you as soon as
   soon as possible. All reading is done int the background.
*/

mExpClass(Seis) SeisRandomProvider : public CallBacker
{
public:
					SeisRandomProvider(const MultiID&);
					~SeisRandomProvider();


    void				requestTrace(const BinID&);

    Notifier<SeisRandomProvider>	traceAvailable;
    const SeisTrc&			getTrace() const { return curtrace_; }


protected:
    bool				readTraces();
    void				triggerWork();
    void				readFinished(CallBacker*);

    bool				isreading_ = false;

    Threads::ConditionVar		lock_;

    SeisTrcReader*			reader_ = nullptr;
    SeisTrcTranslator*			translator_ = nullptr;

    SeisTrc				curtrace_;
    BinIDValueSet			wantedbids_;
};
