#ifndef uistatsdisplay_h
#define uistatsdisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra / Bert
 Date:          Aug 2007
 RCS:           $Id: uistatsdisplay.h,v 1.4 2008-04-01 09:27:04 cvsbert Exp $
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
				~uiStatsDisplay();

    bool                        setDataPackID(DataPack::ID,DataPackMgr::ID);
    void			setData(const float*,int sz);
    void			setData(const Array2D<float>*);
    void			setData(const Stats::RunCalc<float>&);

    uiCanvas*			canvas()	  { return canvas_; }
    uiAxisHandler*		getAxis( bool x ) { return x ? xax_ : yax_; }

    void			setMarkValue(float);

protected:

    uiCanvas*			canvas_;
    uiGenInput*			countfld_;
    uiGenInput*			minmaxfld_;
    uiGenInput*			avgstdfld_;
    uiGenInput*			medrmsfld_;

    uiAxisHandler*		xax_;
    uiAxisHandler*		yax_;
    TypeSet<int>		histdata_;

    const Setup			setup_;
    int				histmaxidx_;
    int				histcount_;
    float			markval_;

    void                        reDraw(CallBacker*);
    void			updateHistogram(const Stats::RunCalc<float>&);
};


#endif
