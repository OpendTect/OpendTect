#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"
#include "prestackprocessor.h"
#include "multiid.h"

template <class T> class Array1D;

namespace PreStack
{

class MuteDef;

/*!
\brief Processor for PreStack Trim Statics.
*/

mExpClass(PreStackProcessing) TrimStatics : public Processor
{ mODTextTranslationClass(TrimStatics)
public:
			mDefaultFactoryInstantiation(
				Processor, TrimStatics, "Trim Statics",
				toUiString(sFactoryKeyword()))

				TrimStatics();
				~TrimStatics();

    bool			prepareWork() override;

    uiString			errMsg() const override { return errmsg_; }

	mExpClass(PreStackProcessing) Iteration
	{
	public:
				Iteration();
	    bool		operator==(const Iteration&) const;
	    bool		operator!=(const Iteration&) const;

	    Interval<float>	ptoffsetrg_;
	    Interval<float>	tsoffsetrg_;
	    float		maxshift_;
	};

    void			addIteration(const Iteration&);
    void			removeIteration(int);
    void			removeAllIterations();

    const TypeSet<Iteration>&	getIterations() const;
    TypeSet<Iteration>&		getIterations();

    void			setOutput( int op )	{ output_ = op; }
    int				getOutput() const	{ return output_; }

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

protected:
    uiString			errmsg_;
    TypeSet<Iteration>		iterations_;
    int				output_;
    ObjectSet<Array1D<float> >	pilottrcs_;

    od_int64			nrIterations() const override;
    bool			doWork(od_int64,od_int64,int) override;
    bool			doPilotTraceOutput(od_int64,od_int64);
    bool			doShiftOutput(od_int64,od_int64);
    bool			doTrimStaticsOutput(od_int64,od_int64);
};

} // namespace PreStack
