#ifndef uiwelltietoseismicdlg_h
#define uiwelltietoseismicdlg_h

/*+
________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
Author:        Bruno
Date:          January 2009
RCS:           $Id: uiwellwelltietoseismicdlg.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "uidialog.h"
#include "welltiesetup.h"

template <class T> class Array1DImpl;
class DataPointSet;
class uiWellTieView;
class uiWellTieControlView;
class uiWellTieWavelet;
class uiTable;
class UserPicks;
class WellTieToSeismic;
class WellTiePickSetManager;

class uiGroup;
class uiToolBar;
class uiPushButton;
class uiCheckBox;


namespace Attrib { class DescSet; }
namespace Well
{
    class Data;
}

mClass uiWellTieToSeismicDlg : public uiDialog
{
public:

	    uiWellTieToSeismicDlg(uiParent*,const WellTieSetup&,
				const Attrib::DescSet&,bool); 
	    ~uiWellTieToSeismicDlg();


    const WellTieSetup&	tieSetup()	{ return wtsetup_; }    
	
protected:

    Well::Data*		wd_;
    WellTieSetup	wtsetup_;
    WellTieToSeismic*   wts_;

    ObjectSet< Array1DImpl<float> >  dispdata_;   
    ObjectSet< Array1DImpl<float> >  orgdispdata_;   
    DataPointSet*	dps_;
    WellTiePickSetManager* picksetmgr_;

    uiToolBar*          toolbar_;
    uiGroup*            vwrgrp_;
    uiPushButton*	applybut_;
    uiPushButton*	undobut_;
    uiCheckBox* 	cscorrfld_;
    uiWellTieWavelet*	wvltdraw_; 
    uiWellTieView*	dataviewer_;
    uiWellTieControlView* controlview_;

    void		addControl();
    void 		addToolBarTools();
    void 		doWork();
    void		createTaskFields(uiGroup*);
    void 		dispParPushed(CallBacker*);
    void 		drawData();
    void 		updateCurve(CallBacker*);
    void 		drawFields(uiGroup*);
    void 		initAll();
    void 		updateButtons();
    void 		setWinTitle(const Attrib::DescSet& ads);

    void 		applyPushed(CallBacker*);
    void 		applyShiftPushed(CallBacker*);
    void		checkShotChg(CallBacker*);
    void		drawUserPick(CallBacker*);
    void 		viewDataPushed(CallBacker*);
    void 		setView(CallBacker*);
    void 		wvltChg(CallBacker*);
    bool		saveD2TPushed(CallBacker*);
    bool 		editD2TPushed(CallBacker*);
    
    bool		acceptOK(CallBacker*);
    bool		rejectOK(CallBacker*);
    bool 		undoPushed(CallBacker*);

};


#endif






