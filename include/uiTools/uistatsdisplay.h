#ifndef uistatsdisplay_h
#define uistatsdisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra / Bert
 Date:          Aug 2007
 RCS:           $Id: uistatsdisplay.h,v 1.11 2009-01-08 07:07:01 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "datapack.h"
class uiHistogramDisplay;
class uiGenInput;
template <class T> class Array2D;
namespace Stats { template <class T> class RunCalc; }


mClass uiStatsDisplay : public uiGroup
{
public:

    struct Setup
    {
				Setup()
				    : withplot_(true)
				    , withtext_(true)
				    , vertaxis_(true)
				    , countinplot_(false)	{}

	mDefSetupMemb(bool,withplot)
	mDefSetupMemb(bool,withtext)
	mDefSetupMemb(bool,vertaxis)
	mDefSetupMemb(bool,countinplot)
    };
				uiStatsDisplay(uiParent*,const Setup&);

    bool                        setDataPackID(DataPack::ID,DataPackMgr::ID);
    void			setData(const float*,int sz);
    void			setData(const Array2D<float>*);

    uiHistogramDisplay*         funcDisp()        { return histgramdisp_; }
    void			setMarkValue(float,bool forx);

    void			putN();

protected:

    uiHistogramDisplay*		histgramdisp_;
    uiGenInput*			countfld_;
    uiGenInput*			minmaxfld_;
    uiGenInput*			avgstdfld_;
    uiGenInput*			medrmsfld_;

    const Setup			setup_;

    void			setData(const Stats::RunCalc<float>&);
};


#endif
