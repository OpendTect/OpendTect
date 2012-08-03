#ifndef uidatapointsetcrossplotwin_h
#define uidatapointsetcrossplotwin_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uidatapointsetcrossplotwin.h,v 1.26 2012-08-03 13:00:59 cvskris Exp $
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidatapointsetcrossplot.h"
#include "uiprogressbar.h"
#include "uimainwin.h"

class uiDPSSelGrpDlg;
class uiDataPointSetCrossPlotterPropDlg;
class uiDPSRefineSelDlg;
class uiColorTable;
class uiComboBox;
class uiToolBar;

/*!\brief Data Point Set Cross Plotter Main window */

mClass(uiIo) uiDataPointSetCrossPlotWin : public uiMainWin
{
public:

    				uiDataPointSetCrossPlotWin(uiDataPointSet&);
				~uiDataPointSetCrossPlotWin();

    uiDataPointSet&		uiPointSet()	{ return uidps_; }
    uiDataPointSetCrossPlotter&	plotter()	{ return plotter_; }
    uiToolBar&			dispTB()	{ return disptb_; }
    uiToolBar&			manipTB()	{ return maniptb_; }

    static uiDataPointSetCrossPlotter::Setup	defsetup_;

    void 			setSelComboSensitive(bool);
    void			setButtonStatus()	{ setSelectable(0); }
    void			setPercDisp(float);
    void			setGrpColors();

protected:

    uiDataPointSet&		uidps_;
    uiDataPointSetCrossPlotter&	plotter_;
    uiDataPointSetCrossPlotterPropDlg* propdlg_;
    uiDPSSelGrpDlg*		selgrpdlg_;
    uiDPSRefineSelDlg*		refineseldlg_;
    uiToolBar&			disptb_;
    uiToolBar&			seltb_;
    uiToolBar&			maniptb_;
    uiToolBar&			colortb_;
    uiSpinBox*			eachfld_;
    uiComboBox*			grpfld_;
    uiComboBox*			selfld_;
    uiColorTable*		coltabfld_;

    bool			wantnormalplot_;
    int				densityplottbid_;
    int				showy2tbid_;
    int				showselptswstbid_;
    int				selmodechgtbid_;
    int				seldeltbid_;
    int				clearseltbid_;
    int				seltabletbid_;
    int				setselecttbid_;
    int				refineseltbid_;
    int				manseltbid_;
    int				multicolcodtbid_;
    int				minptsfordensity_;
    int				overlayproptbid_;

    static const char*		sKeyMinDPPts()
    				{ return "Minimum pts for Density Plot"; }

    void			closeNotif(CallBacker*);
    void			showTableSel(CallBacker*);
    void			showY2(CallBacker*);
    void			showPtsInWorkSpace(CallBacker*);
    void			setSelectable(CallBacker*);
    void			setSelectionMode(CallBacker*);
    void			setDensityPlot(CallBacker*);
    void			removeSelections(CallBacker*);
    void			deleteSelections(CallBacker*);
    void			setSelectionDomain(CallBacker*);
    void			drawTypeChangedCB(CallBacker*);
    void			exportPDF(CallBacker*);
    void			manageSel(CallBacker*);
    void			overlayAttrCB(CallBacker*);
    void			eachChg(CallBacker*);
    void			grpChg(CallBacker*);
    void			editProps(CallBacker*);
    void			selOption(CallBacker*);
    void			colTabChanged(CallBacker*);
    void			coltabRgChangedCB(CallBacker*);
    void			setMultiColorCB(CallBacker*);
    void			changeColCB(CallBacker*);
};


#endif

