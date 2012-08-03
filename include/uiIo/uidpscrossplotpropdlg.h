#ifndef uidpscrossplotpropdlg_h
#define uidpscrossplotpropdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2008
 RCS:           $Id: uidpscrossplotpropdlg.h,v 1.9 2012-08-03 13:00:59 cvskris Exp $
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidlggroup.h"
class uiDataPointSetCrossPlotter;
class uiDPSCPScalingTab;
class uiDPSCPStatsTab;
class uiDPSUserDefTab;
class uiDPSCPBackdropTab;
class uiDPSCPDisplayPropTab;
class uiDPSDensPlotSetTab;

		     
mClass(uiIo) uiDataPointSetCrossPlotterPropDlg : public uiTabStackDlg
{
public:
					uiDataPointSetCrossPlotterPropDlg(
					    uiDataPointSetCrossPlotter*);
    uiDataPointSetCrossPlotter&		plotter()	{ return plotter_; }

protected:

    uiDataPointSetCrossPlotter&		plotter_;
    uiDPSCPScalingTab*			scaletab_;
    uiDPSCPStatsTab*			statstab_;
    uiDPSUserDefTab*			userdeftab_;
    uiDPSCPDisplayPropTab* 		dispproptab_;
    uiDPSDensPlotSetTab* 		densplottab_;
    uiDPSCPBackdropTab*			bdroptab_;

    void				doApply(CallBacker*);
    bool				acceptOK(CallBacker*);

};

#endif

