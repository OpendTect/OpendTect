#ifndef uidatapointsetcrossplotwin_h
#define uidatapointsetcrossplotwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uidatapointsetcrossplotwin.h,v 1.1 2008-04-03 08:28:30 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidatapointsetcrossplot.h"
#include "uimainwin.h"
class uiToolBar;

/*!\brief Data Point Set Cross Plotter Main window */

class uiDataPointSetCrossPlotWin : public uiMainWin
{
public:

    				uiDataPointSetCrossPlotWin(uiDataPointSet&);

    uiDataPointSet&		uiPointSet()	{ return uidps_; }
    uiDataPointSetCrossPlotter&	plotter()	{ return plotter_; }
    uiToolBar&			dispTB()	{ return disptb_; }
    uiToolBar&			manipTB()	{ return maniptb_; }

    static uiDataPointSetCrossPlotter::Setup	defsetup_;

protected:

    uiDataPointSet&		uidps_;
    uiDataPointSetCrossPlotter&	plotter_;
    uiToolBar&			disptb_;
    uiToolBar&			maniptb_;
    uiSpinBox*			eachfld_;

    bool			rbissel_;

    int				showy2tbid_;
    int				showbdtbid_;
    int				selzoomtbid_;

    void			closeNotif(CallBacker*);
    void			showY2(CallBacker*);
    void			showBD(CallBacker*);
    void			eachChg(CallBacker*);
    void			selZoom(CallBacker*);
    void			delSel(CallBacker*);
    void			unZoom(CallBacker*);
};


#endif
