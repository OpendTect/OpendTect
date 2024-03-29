#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidlggroup.h"

class uiDPSCPBackdropTab;
class uiDPSCPDisplayPropTab;
class uiDPSCPScalingTab;
class uiDPSCPStatsTab;
class uiDPSDensPlotSetTab;
class uiDPSUserDefTab;
class uiDataPointSetCrossPlotter;

/*!
\brief Crossplot properties dialog box.
*/

mExpClass(uiIo) uiDataPointSetCrossPlotterPropDlg : public uiTabStackDlg
{ mODTextTranslationClass(uiDataPointSetCrossPlotterPropDlg);
public:
				uiDataPointSetCrossPlotterPropDlg(
					    uiDataPointSetCrossPlotter*);
				~uiDataPointSetCrossPlotterPropDlg();

    uiDataPointSetCrossPlotter&	plotter()	{ return plotter_; }
    
protected:

    uiDataPointSetCrossPlotter&	plotter_;
    uiDPSCPScalingTab*		scaletab_;
    uiDPSCPStatsTab*		statstab_;
    uiDPSUserDefTab*		userdeftab_;
    uiDPSCPDisplayPropTab*	dispproptab_;
    uiDPSDensPlotSetTab*	densplottab_;
    uiDPSCPBackdropTab*		bdroptab_;

    void			doApply(CallBacker*);
    bool			acceptOK(CallBacker*) override;
};
