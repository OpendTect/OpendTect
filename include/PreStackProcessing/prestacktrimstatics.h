#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id$
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

    bool			prepareWork();

    uiString			errMsg() const		{ return errmsg_; }

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

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
    uiString			errmsg_;
    TypeSet<Iteration>		iterations_;
    int				output_;
    ObjectSet<Array1D<float> >	pilottrcs_;

    od_int64			nrIterations() const;
    bool			doWork(od_int64,od_int64,int);
    bool			doPilotTraceOutput(od_int64,od_int64);
    bool			doShiftOutput(od_int64,od_int64);
    bool			doTrimStaticsOutput(od_int64,od_int64);
};

} // namespace PreStack

