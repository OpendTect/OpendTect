#ifndef uidpscrossplotpropdlg_h
#define uidpscrossplotpropdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Jun 2008
 RCS:           $Id: uidpscrossplotpropdlg.h,v 1.3 2009-01-08 07:23:07 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"
class uiDataPointSetCrossPlotter;
class uiDPSCPScalingTab;
class uiDPSCPStatsTab;
class uiDPSUserDefTab;
class uiDPSCPBackdropTab;

		     
mClass uiDataPointSetCrossPlotterPropDlg : public uiTabStackDlg
{
public:
			uiDataPointSetCrossPlotterPropDlg(
					uiDataPointSetCrossPlotter*);
    uiDataPointSetCrossPlotter&	plotter()		{ return plotter_; }

protected:

    uiDataPointSetCrossPlotter&	plotter_;
    uiDPSCPScalingTab*	scaletab_;
    uiDPSCPStatsTab*	statstab_;
    uiDPSUserDefTab*	userdeftab_;
    uiDPSCPBackdropTab*	bdroptab_;

    bool		acceptOK(CallBacker*);

};

#endif
