#pragma once

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          March 2010
________________________________________________________________________

-*/

#include "task.h"
#include "arraynd.h"
#include "datapointset.h"
#include "ranges.h"
#include "enums.h"

mClass(uiIo) DPSDensityCalcND : public ParallelTask
{ mODTextTranslationClass(DPSDensityCalcND);
public:

    enum CalcAreaType		{ All, Selected, NonSelected };
				mDeclareEnumUtils(CalcAreaType);

    struct AxisParam
    {
	int			colid_;
	StepInterval<float>	valrange_;
    };

				DPSDensityCalcND(const DataPointSet&,
						 const ObjectSet<AxisParam>&,
						 ArrayND<float>&,
				 CalcAreaType areatype=DPSDensityCalcND::All);

    od_int64			nrIterations() const override;
    od_int64			nrDone() const override { return nrdone_; }
    uiString			uiNrDoneText() const override;

    void			setGroup(int grp)	{ grp_ = grp; }
    bool			getPositions(TypeSet<int>&,int);
    bool			setFreqValue(const int*);
    bool			doWork(od_int64 start,
				       od_int64 stop,int) override;

protected:
    const DataPointSet&			dps_;
    ArrayND<float>&			freqdata_;
    ObjectSet<AxisParam>		axisdatas_;
    int					nrdims_;
    int					nrdone_;
    int					grp_;
    CalcAreaType			areatype_;

    float				getVal(int colid,int rowid) const;
};

