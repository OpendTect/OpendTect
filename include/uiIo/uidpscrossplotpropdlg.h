#ifndef uidpscrossplotpropdlg_h
#define uidpscrossplotpropdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2008
 RCS:           $Id: uidpscrossplotpropdlg.h,v 1.4 2009-07-22 16:01:21 cvsbert Exp $
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
