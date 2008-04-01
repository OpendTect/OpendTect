#ifndef uistatsdisplay_h
#define uistatsdisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra / Bert
 Date:          Aug 2007
 RCS:           $Id: uistatsdisplay.h,v 1.5 2008-04-01 13:22:53 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "datapack.h"
class uiFunctionDisplay;
class uiGenInput;
template <class T> class Array2D;
namespace Stats { template <class T> class RunCalc; }


class uiStatsDisplay : public uiGroup
{
public:

    struct Setup
    {
				Setup()
				    : withplot_(true)
				    , withtext_(true)
				    , countinplot_(true)	{}

	mDefSetupMemb(bool,withplot)
	mDefSetupMemb(bool,withtext)
	mDefSetupMemb(bool,countinplot)
    };
				uiStatsDisplay(uiParent*,const Setup&);

    bool                        setDataPackID(DataPack::ID,DataPackMgr::ID);
    void			setData(const float*,int sz);
    void			setData(const Array2D<float>*);
    void			setData(const Stats::RunCalc<float>&);

    uiFunctionDisplay*		funcDisp()	  { return funcdisp_; }
    void			setMarkValue(float);

protected:

    uiFunctionDisplay*		funcdisp_;
    uiGenInput*			countfld_;
    uiGenInput*			minmaxfld_;
    uiGenInput*			avgstdfld_;
    uiGenInput*			medrmsfld_;

    const Setup			setup_;
    int				histcount_;

    void			updateHistogram(const Stats::RunCalc<float>&);
    void			putN(CallBacker*);
};


#endif
