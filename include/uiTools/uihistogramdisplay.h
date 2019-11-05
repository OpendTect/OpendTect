#ifndef uihistogramdisplay_h
#define uihistogramdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id$
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

    void			setEmpty();
    bool			setDataPackID(DataPack::ID,DataPackMgr::ID,
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
    int				nrInpVals() const	{ return nrinpvals_; }
    int				nrClasses() const	{ return nrclasses_; }
    void			putN();

protected:

    Stats::ParallelCalc<float>&	rc_;
    int				nrinpvals_;
    int				nrclasses_;
    bool			withheader_;
    uiTextItem*			header_;
    uiTextItem*			nitm_;

    Interval<float>		mydrawrg_;
    bool			usemydrawrg_;
    LargeValVec<float>		originaldata_;

    void			updateAndDraw();
    void			updateHistogram();
    void			setDataDPS(const DataPointSet&,int dpsidx);
};

#endif
