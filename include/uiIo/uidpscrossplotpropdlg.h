#ifndef uidpscrossplotpropdlg_h
#define uidpscrossplotpropdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2008
 RCS:           $Id: uidpscrossplotpropdlg.h,v 1.5 2009-08-27 07:15:03 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"
class uiDataPointSetCrossPlotter;
class uiDPSCPScalingTab;
class uiDPSCPStatsTab;
class uiDPSUserDefTab;
class uiDPSCPBackdropTab;
class uiDPSDensPlotSetTab;

		     
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
    uiDPSDensPlotSetTab* densplottab_;
    uiDPSCPBackdropTab*	bdroptab_;

    bool		acceptOK(CallBacker*);

};

#endif
