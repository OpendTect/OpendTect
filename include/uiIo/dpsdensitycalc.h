/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          March 2010
 RCS:           $Id: dpsdensitycalc.h,v 1.1 2010-03-03 10:05:56 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "task.h"
#include "arraynd.h"
#include "datapointset.h"
#include "ranges.h"

class uiDataPointSet;

class DPSDensityCalcND : public ParallelTask
{
public:

    struct AxisParam
    {
	int			colid_;
	StepInterval<float>	valrange_;
	int			nrbins_;
    };

				DPSDensityCalcND(const uiDataPointSet& uidps,
						 const ObjectSet<AxisParam>&);

    od_int64			nrDone() const		{ return nrdone_; }
    od_int64			nrIterations() const;

    bool			getPositions(TypeSet<int>&,int);
    bool			setFreqValue(const int*);
    bool			doWork(od_int64 start,od_int64 stop,int);

    void			getFreqData(ArrayND<float>&) const;

protected:
    const uiDataPointSet&		uidps_;
    ArrayND<float>*			freqdata_;
    ObjectSet<AxisParam>		axisdatas_;
    int					nrdims_;
    int					nrdone_;

};

