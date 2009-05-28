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
class WellTiePickSetMGR;
class WellTieCSCorr;


class uiGroup;
class uiToolBar;
class uiGenInput;
class uiPushButton;
class uiCheckBox;

class uiWellTieView;
class uiWellTieControlView;
class uiWellTieCorrView;
class uiWellTieWaveletView;
class uiWellTieEventStretch;
class uiWellTieLogStretch;

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
    WellTiePickSetMGR*  picksetmgr_;
    WellTieDataMGR* 	datamgr_;
    WellTieCSCorr*	cscorr_;	

    uiToolBar*          toolbar_;
    uiGroup*            vwrgrp_;
    uiPushButton*	applybut_;
    uiPushButton*	applymrkbut_;
    uiPushButton*	undobut_;
    uiCheckBox* 	cscorrfld_;
    uiCheckBox* 	csdispfld_;
    uiGenInput*		topmrkfld_;
    uiGenInput*		botmrkfld_;
    uiGenInput*		corrcoefffld_;
    uiWellTieWaveletView* wvltdraw_; 
    uiWellTieView*	datadrawer_;
    uiWellTieControlView* controlview_;
    uiWellTieLogStretch* logstretcher_;
    uiWellTieEventStretch* eventstretcher_;
    uiWellTieCorrView*  crosscorr_;

    bool		manip_;

    void		addControl();
    void 		addToolBarTools();
    void 		doWholeWork();
    void		createTaskFields(uiGroup*);
    void 		dispParPushed(CallBacker*);
    void 		drawData();
    void 		dispDataChanged(CallBacker*);
    void 		drawFields(uiGroup*);
    void 		initAll();
    void 		updateButtons();
    void 		setWinTitle(const Attrib::DescSet& ads);
    bool 		setUserDepths();

    void 		applyPushed(CallBacker*);
    void 		applyReady(CallBacker*);
    void 		applyMarkerPushed(CallBacker*);
    void 		applyShiftPushed(CallBacker*);
    void		checkShotChg(CallBacker*);
    void		checkShotDisp(CallBacker*);
    void		drawUserPick(CallBacker*);
    void 		viewDataPushed(CallBacker*);
    void 		setView(CallBacker*);
    void 		wvltChg(CallBacker*);
    bool		saveD2TPushed(CallBacker*);
    bool 		editD2TPushed(CallBacker*);
    void		userDepthsChanged(CallBacker*);
    
    bool		acceptOK(CallBacker*);
    bool		rejectOK(CallBacker*);
    bool 		undoPushed(CallBacker*);
};


#endif

