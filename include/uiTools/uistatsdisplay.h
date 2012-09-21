#ifndef uistatsdisplay_h
#define uistatsdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra / Bert
 Date:          Aug 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "datapack.h"
class uiHistogramDisplay;
class uiGenInput;
class uiLabel;
template <class T> class Array2D;
namespace Stats { template <class T> class ParallelCalc; }


mClass(uiTools) uiStatsDisplay : public uiGroup
{
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

    bool                        setDataPackID(DataPack::ID,DataPackMgr::ID);
    void			setData(const float*,int sz);
    void			setData(const Array2D<float>*);
    void			setDataName(const char*);

    uiHistogramDisplay*         funcDisp()        { return histgramdisp_; }
    void			setMarkValue(float,bool forx);

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


#endif

