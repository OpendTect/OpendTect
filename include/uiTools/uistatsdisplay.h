#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra / Bert
 Date:          Aug 2007
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "datapack.h"
class uiHistogramDisplay;
class uiGenInput;
class uiLabel;
template <class T> class Array2D;
template <class T> class DataDistribution;
namespace Stats { template <class T> class ParallelCalc; }


mExpClass(uiTools) uiStatsDisplay : public uiGroup
{ mODTextTranslationClass(uiStatsDisplay)
public:

    struct Setup
    {
				Setup()
				    : withplot_(true)
				    , withname_(true)
				    , withtext_(true)
				    , vertaxis_(true)
				    , countinplot_(false)	{}

	mDefSetupMemb(bool,withplot)
	mDefSetupMemb(bool,withname)
	mDefSetupMemb(bool,withtext)
	mDefSetupMemb(bool,vertaxis)
	mDefSetupMemb(bool,countinplot)
    };

				uiStatsDisplay(uiParent*,const Setup&);

    bool			setDataPackID(DataPack::ID,DataPackMgr::ID,
					      int version);
    void			setData(const TypeSet<float>&);
    void			setData(const Array2D<float>*);
    void			setData(const float*,od_int64);
    void			setData(const DataDistribution<float>&,
				    od_int64 count=mUdf(od_int64),
				    Interval<float> rg=Interval<float>::udf());
    void			setDataName(const char*);

    uiHistogramDisplay*		funcDisp()	{ return histgramdisp_; }
    void			setMarkValue(float,bool forx);
    void			usePar(const IOPar&);

    void			putN();

protected:

    uiHistogramDisplay*		histgramdisp_;
    uiLabel*			namefld_;
    uiGenInput*			countfld_;
    uiGenInput*			minmaxfld_;
    uiGenInput*			avgstdfld_;
    uiGenInput*			medrmsfld_;

    const Setup			setup_;

    void			setData(const Stats::ParallelCalc<float>&);
};
