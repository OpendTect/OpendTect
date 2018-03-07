#ifndef tutvolproc_h
#define tutvolproc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		March 2016
 RCS:		$Id$
________________________________________________________________________


-*/

#include "tutmod.h"

#include "paralleltask.h"
#include "trckeyzsampling.h"
#include "uistring.h"
#include "volprocstep.h"

template<class T> class Array3D;

namespace VolProc
{

/*!
\brief tutorial for various Volume Operations
*/

mExpClass(Tut) TutOpCalculator : public Step
{ mODTextTranslationClass(TutOpCalculator);
public:
			mDefaultFactoryInstantiation(
					VolProc::Step, TutOpCalculator,
					"VolumeProcessingTutorial",
					tr("Volume Processing Tutorial") )

			TutOpCalculator();
			~TutOpCalculator();

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    static const char*	sKeyTypeIndex()		{ return "Type Index"; }

private:

    virtual bool	needsInput() const	{ return true; }
    virtual bool	needsFullVolume() const { return false; }

    bool		prepareWork();
    Task*		createTask();

    void		setStepParameters();
			//Replaced by virtual functions after 6.2

    od_int64		extraMemoryUsage(OutputSlotID,const TrcKeySampling&,
					 const StepInterval<int>&) const
			{ return 0; }

    BinID		shift_;
    int			type_;

};


mExpClass(Tut) TutOpCalculatorTask : public ParallelTask
{ mODTextTranslationClass(TutOpCalculatorTask);
public:
				TutOpCalculatorTask(const Array3D<float>&,
						const TrcKeyZSampling& tkzsin,
						const TrcKeyZSampling& tkzsout,
						int,BinID,Array3D<float>& out);

    od_int64			totalNr() const		{ return totalnr_; }
    uiString			uiMessage() const;

private:

    bool			doWork(od_int64,od_int64,int);

    od_int64			nrIterations() const	{ return totalnr_; }

    od_int64			totalnr_;

    const Array3D<float>&	input_;
    Array3D<float>&		output_;

    BinID			shift_;
    int				type_;

    TrcKeyZSampling		tkzsin_;
    TrcKeyZSampling		tkzsout_;
};

} // namespace VolProc

#endif
