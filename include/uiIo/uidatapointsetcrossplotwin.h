#ifndef uidatapointsetcrossplotwin_h
#define uidatapointsetcrossplotwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uidatapointsetcrossplotwin.h,v 1.11 2009-04-15 12:10:48 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidatapointsetcrossplot.h"
#include "uimainwin.h"
class uiToolBar;
class uiComboBox;

/*!\brief Data Point Set Cross Plotter Main window */

mClass uiDataPointSetCrossPlotWin : public uiMainWin
{
public:

    				uiDataPointSetCrossPlotWin(uiDataPointSet&);

    uiDataPointSet&		uiPointSet()	{ return uidps_; }
    uiDataPointSetCrossPlotter&	plotter()	{ return plotter_; }
    uiToolBar&			dispTB()	{ return disptb_; }
    uiToolBar&			manipTB()	{ return maniptb_; }

    static uiDataPointSetCrossPlotter::Setup	defsetup_;

    Notifier<uiDataPointSetCrossPlotWin>	showSelPts;

    void 			setSelComboSensitive(bool);

protected:

    uiDataPointSet&		uidps_;
    uiDataPointSetCrossPlotter&	plotter_;
    uiToolBar&			disptb_;
    uiToolBar&			maniptb_;
    uiSpinBox*			eachfld_;
    uiComboBox*			grpfld_;
    uiComboBox*			selfld_;

    int				showy2tbid_;
    int				showselptswstbid_;
    int				selmodechgtbid_;
    int				setselecttbid_;

    void			closeNotif(CallBacker*);
    void			shoeTableSel(CallBacker*);
    void			showY2(CallBacker*);
    void			showPtsInWorkSpace(CallBacker*);
    void			setSelectable(CallBacker*);
    void			setSelectionMode(CallBacker*);
    void			removeSelections(CallBacker*);
    void			deleteSelections(CallBacker*);
    void			setSelectionDomain(CallBacker*);
    void			eachChg(CallBacker*);
    void			grpChg(CallBacker*);
    void			editProps(CallBacker*);
    void			selOption(CallBacker*);
};


#endif
