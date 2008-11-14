#ifndef uidatapointsetcrossplotwin_h
#define uidatapointsetcrossplotwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uidatapointsetcrossplotwin.h,v 1.6 2008-11-14 05:36:19 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidatapointsetcrossplot.h"
#include "uimainwin.h"
class uiToolBar;
class uiComboBox;

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

    Notifier<uiDataPointSetCrossPlotWin>	showSelPts;

protected:

    uiDataPointSet&		uidps_;
    uiDataPointSetCrossPlotter&	plotter_;
    uiToolBar&			disptb_;
    uiToolBar&			maniptb_;
    uiSpinBox*			eachfld_;
    uiComboBox*			grpfld_;
    uiComboBox*			selfld_;

    int				showy2tbid_;
    int				showselptswsbid_;
    int				setselecttbid_;

    void			closeNotif(CallBacker*);
    void			showY2(CallBacker*);
    void			showPtsInWorkSpace(CallBacker*);
    void			setSelectable(CallBacker*);
    void			eachChg(CallBacker*);
    void			grpChg(CallBacker*);
    void			delSel(CallBacker*);
    void			editProps(CallBacker*);
    void			selOption(CallBacker*);
};


#endif
