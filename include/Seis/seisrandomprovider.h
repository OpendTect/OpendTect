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
    					SeisRandomProvider(const MultiID& mid);
   					~SeisRandomProvider();


    void				requestTrace(const BinID&);

    Notifier<SeisRandomProvider>	traceAvailable;
    const SeisTrc&			getTrace() const { return curtrace_; }


protected:
    bool				readTraces();
    void				triggerWork();
    void				readFinished(CallBacker*);

    bool				isreading_;

    Threads::ConditionVar		lock_;

    SeisTrcReader*			reader_;
    SeisTrcTranslator*			translator_;

    SeisTrc				curtrace_;
    BinIDValueSet			wantedbids_;
};



mExpClass(Seis) SeisRandomRepository : public CallBacker
{
public:
    				SeisRandomRepository( const MultiID& mid );

    void			addInterest(const BinID&);
    void			removeInterest(const BinID);

    const SeisTrc*		getTrace(const BinID&) const;
    Notifier<SeisRandomRepository>	traceAvailable;
    const BinID&		newTraceBid() const { return newtracebid_; }

protected:

	struct TraceHolder : public ReferencedObject
	{
				TraceHolder() : trc_( 0 ) {}

	    SeisTrc*		trc_;
	protected:
				~TraceHolder();
	};

    void				incomingTrace( CallBacker* );


    MultiDimStorage<TraceHolder*>	storage_;
    BinID				newtracebid_;
};
