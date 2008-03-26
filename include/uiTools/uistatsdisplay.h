#ifndef uistatsdisplay_h
#define uistatsdisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra / Bert
 Date:          Aug 2007
 RCS:           $Id: uistatsdisplay.h,v 1.2 2008-03-26 16:46:08 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "datapack.h"
#include "samplingdata.h"
class uiCanvas;
class uiGenInput;
class uiAxisHandler;
template <class T> class Array2D;
namespace Stats { template <class T> class RunCalc; }


class uiStatsDisplay : public uiGroup
{
public:
				uiStatsDisplay(uiParent*,bool withplot=true,
						bool withtext=true);
				~uiStatsDisplay();

    bool                        setDataPackID(DataPack::ID,DataPackMgr::ID);
    void			setData(const float*,int sz);
    void			setData(const Array2D<float>*);
    void			setData(const Stats::RunCalc<float>&);

    uiCanvas*			canvas()	  { return canvas_; }
    uiAxisHandler*		getAxis( bool x ) { return x ? xax_ : yax_; }

protected:

    uiCanvas*			canvas_;
    uiGenInput*			countfld_;
    uiGenInput*			minmaxfld_;
    uiGenInput*			avgstdfld_;
    uiGenInput*			medrmsfld_;

    TypeSet<float>		histdata_;
    uiAxisHandler*		xax_;
    uiAxisHandler*		yax_;

    void                        reDraw(CallBacker*);
    void			updateHistogram(const Stats::RunCalc<float>&);
    
    SamplingData<float>	  	sd_;
};


#endif
