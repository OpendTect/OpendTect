#ifndef uistatsdisplay_h
#define uistatsdisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra / Bert
 Date:          Aug 2007
 RCS:           $Id: uistatsdisplay.h,v 1.1 2008-03-26 13:21:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "datapack.h"
class uiCanvas;
class uiGenInput;
class uiHistogramDisplay;
template <class T> class Array2D;
namespace Stats { template <class T> class RunCalc; }


class uiStatsDisplay : public uiGroup
{
public:
				uiStatsDisplay(uiParent*,bool withplot=true,
						bool withtext=true);

    bool                        setDataPackID(DataPack::ID,DataPackMgr::ID);
    void			setData(const float*,int sz);
    void			setData(const Array2D<float>*);
    void			setData(const Stats::RunCalc<float>&);

protected:

    uiCanvas*			canvas_;
    uiHistogramDisplay*		histogramdisplay_;
    uiGenInput*			countfld_;
    uiGenInput*			minmaxfld_;
    uiGenInput*			avgstdfld_;
    uiGenInput*			medrmsfld_;

    void                        reDraw(CallBacker*);

    void			setHistogram(const TypeSet<float>&);
    void 			setInfo(const Stats::RunCalc<float>&);
    
    uiRect                      bdrect_;
    StepInterval<float>	  	xrg_;
};


#endif
