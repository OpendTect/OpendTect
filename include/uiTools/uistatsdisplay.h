#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "datapack.h"
#include "uigroup.h"

class uiGenInput;
class uiHistogramDisplay;
class uiLabel;
template <class T> class Array2D;
namespace Stats { template <class T> class ParallelCalc; }


mExpClass(uiTools) uiStatsDisplay : public uiGroup
{
mODTextTranslationClass(uiStatsDisplay);
public:

    mExpClass(uiTools) Setup
    {
    public:
				Setup();
				~Setup();

	mDefSetupMemb(bool,withplot)	//!< true
	mDefSetupMemb(bool,withname)	//!< true
	mDefSetupMemb(bool,withtext)	//!< true
	mDefSetupMemb(bool,vertaxis)	//!< true
	mDefSetupMemb(bool,countinplot) //!< false
    };
				uiStatsDisplay(uiParent*,const Setup&);
				~uiStatsDisplay();

    bool			setDataPack(const DataPack&,int version=0);
    mDeprecated("Use setDataPack")
    bool			setDataPackID(const DataPackID&,
					      const DataPackMgr::MgrID&,
					      int version=0);
    void			setData(const float*,int sz);
    void			setData(const Array2D<float>*);
    void			setDataName(const char*);

    uiHistogramDisplay*		funcDisp()	{ return histgramdisp_; }
    void			setMarkValue(float,bool forx);

    void			putN();

private:

    uiHistogramDisplay*		histgramdisp_	= nullptr;
    uiLabel*			namefld_	= nullptr;
    uiGenInput*			countfld_	= nullptr;
    uiGenInput*			minmaxfld_	= nullptr;
    uiGenInput*			avgstdfld_	= nullptr;
    uiGenInput*			medrmsfld_	= nullptr;

    const Setup			setup_;

    void			setData(const Stats::ParallelCalc<float>&);
    void			finalizeCB(CallBacker*);
};
