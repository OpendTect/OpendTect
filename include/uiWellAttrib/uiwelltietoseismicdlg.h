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
#include "bufstringset.h"

class WellTieDataMGR;
class WellTieParams;
class WellTieToSeismic;
class WellTiePickSetManager;

class uiGroup;
class uiToolBar;
class uiGenInput;
class uiPushButton;
class uiCheckBox;

class uiWellTieStretch;
class uiWellTieView;
class uiWellTieControlView;
class uiWellTieWavelet;

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


    const WellTieSetup&	Setup()		{ return setup_; }    
	
protected:

    BufferStringSet     markernames_;
    Well::Data*		wd_;
    WellTieSetup	setup_;
    WellTieParams*	params_;
    WellTieToSeismic*   dataplayer_;
    WellTieDataMGR* 	datamgr_;

    uiToolBar*          toolbar_;
    uiGroup*            vwrgrp_;
    uiPushButton*	applybut_;
    uiPushButton*	undobut_;
    uiCheckBox* 	cscorrfld_;
    uiGenInput*		corrtopmrkfld_;
    uiGenInput*		corrbotmrkfld_;
    uiGenInput*		corrcoefffld_;
    uiWellTieWavelet*	wvltdraw_; 
    uiWellTieView*	datadrawer_;
    uiWellTieControlView* controlview_;
    uiWellTieStretch* 	stretcher_;

    void		addControl();
    void 		addToolBarTools();
    void 		doWholeWork();
    void 		doLogWork(bool);
    void		createTaskFields(uiGroup*);
    void 		dispParPushed(CallBacker*);
    void 		drawData();
    void 		dispDataChanged(CallBacker*);
    void 		dispDataChanging(CallBacker*);
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

