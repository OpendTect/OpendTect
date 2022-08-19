#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uifunctiondisplay.h"
#include "datapack.h"

class uiTextItem;
template <class T> class Array2D;
template <class T> class Array3D;
namespace Stats { template <class T> class ParallelCalc; }

class DataPointSet;

mExpClass(uiTools) uiHistogramDisplay : public uiFunctionDisplay
{ mODTextTranslationClass(uiHistogramDisplay);
public:

				uiHistogramDisplay(uiParent*,Setup&,
						   bool withheader=false);
				~uiHistogramDisplay();

    void			setEmpty() override;
    bool			setDataPackID(DataPackID,DataPackMgr::MgrID,
					      int version);
    void			setData(const float*,od_int64 sz);
    void			setData(const Array2D<float>*);
    void			setData(const Array3D<float>*);
    void			setData(const LargeValVec<float>&);

    void			useDrawRange(bool yn);
    const Interval<float>&	getDrawRange() const	{ return mydrawrg_; }
    void			setDrawRange(const Interval<float>&);
    Notifier<uiHistogramDisplay> drawRangeChanged;

    void			setHistogram(const TypeSet<float>&,
					     Interval<float>,int N=-1);

    const Stats::ParallelCalc<float>&	getStatCalc()	{ return rc_; }
    od_int64			nrInpVals() const	{ return nrinpvals_; }
    int				nrClasses() const	{ return nrclasses_; }
    void			putN();

protected:

    Stats::ParallelCalc<float>&	rc_;
    od_int64			nrinpvals_ = 0;
    int				nrclasses_ = 0;
    bool			withheader_;
    uiTextItem*			header_ = nullptr;
    uiTextItem*			nitm_ = nullptr;
    ObjectSet<uiRectItem>	baritems_;

    Interval<float>		mydrawrg_;
    bool			usemydrawrg_ = false;
    LargeValVec<float>		originaldata_;

    void			updateAndDraw();
    void			updateHistogram();
    void			setDataDPS(const DataPointSet&,int dpsidx);
    void			drawData() override;

};
