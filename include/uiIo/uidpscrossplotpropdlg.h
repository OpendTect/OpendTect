#ifndef uidpscrossplotpropdlg_h
#define uidpscrossplotpropdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Jun 2008
 RCS:           $Id: uidpscrossplotpropdlg.h,v 1.1 2008-06-20 13:39:31 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"
class uiDataPointSetCrossPlotter;
class uiDPSCPScalingTab;
class uiDPSCPStatsTab;
class uiDPSCPBackdropTab;

		     
class uiDataPointSetCrossPlotterPropDlg : public uiTabStackDlg
{
public:
			uiDataPointSetCrossPlotterPropDlg(
					uiDataPointSetCrossPlotter*);
    uiDataPointSetCrossPlotter&	plotter()		{ return plotter_; }

protected:

    uiDataPointSetCrossPlotter&	plotter_;
    uiDPSCPScalingTab*	scaletab_;
    uiDPSCPStatsTab*	statstab_;
    uiDPSCPBackdropTab*	bdroptab_;

    bool		acceptOK(CallBacker*);

};

#endif
