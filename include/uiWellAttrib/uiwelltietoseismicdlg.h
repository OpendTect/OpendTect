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
class WellTieD2TModelMGR;
class WellTieDataHolder;
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
class uiWellTieInfoDlg;
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
				const Attrib::DescSet&); 
	    ~uiWellTieToSeismicDlg();


    const WellTieSetup&		Setup()		{ return setup_; }    
	
protected:

   // BufferStringSet     	markernames_;
    Well::Data*			wd_;
    WellTieSetup		setup_;
    WellTieDataHolder*		dataholder_;
    WellTieParams*		params_;
    WellTieToSeismic*   	dataplayer_;
    WellTieCSCorr*		cscorr_;	

    uiWellTieInfoDlg* 		infodlg_; 
    uiWellTieView*		datadrawer_;
    uiWellTieControlView* 	controlview_;
    uiWellTieLogStretch* 	logstretcher_;
    uiWellTieEventStretch* 	eventstretcher_;
   // uiWellTieCorrView*  	crosscorr_;

    uiToolBar*          	toolbar_;
    uiGroup*            	vwrgrp_;
    uiPushButton*		infobut_;
    uiPushButton*		applybut_;
    uiPushButton*		undobut_;
    uiPushButton*		clearpickbut_;
    uiPushButton*		clearallpicksbut_;
    uiCheckBox* 		cscorrfld_;
    uiCheckBox* 		csdispfld_;
  //  uiGenInput*			corrcoefffld_;

    bool			manip_;

    void			addControl();
    void 			addToolBarTools();
    void 			doWholeWork( CallBacker* );
    void			createTaskFields(uiGroup*);
    void			createCSFields(uiGroup*);
    void 			drawData();
    void 			drawFields(uiGroup*);
    void 			initAll();
    void			resetInfoDlg();
    void 			updateButtons();
    void 			setWinTitle(const Attrib::DescSet& ads);

    void 			applyPushed(CallBacker*);
    void 			applyReady(CallBacker*);
    void 			applyMarkerPushed(CallBacker*);
    void 			applyShiftPushed(CallBacker*);
    void			checkIfPick(CallBacker*);
    void			checkShotChg(CallBacker*);
    void			checkShotDisp(CallBacker*);
    void			clearPickPushed(CallBacker*);
    void			clearAllPicksPushed(CallBacker*);
    void 			dispDataChanged(CallBacker*);
    void 			dispParPushed(CallBacker*);
    void			drawUserPick(CallBacker*);
    void 			infoPushed(CallBacker*);
    void 			viewDataPushed(CallBacker*);
    void 			setView(CallBacker*);
    void 			wvltChg(CallBacker*);
    bool			saveD2TPushed(CallBacker*);
    bool 			editD2TPushed(CallBacker*);
    void			userDepthsChanged(CallBacker*);
    
    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);
    bool 			undoPushed(CallBacker*);
};



mClass uiWellTieInfoDlg : public uiDialog
{
public:		
    		
    		uiWellTieInfoDlg(uiParent*,WellTieDataHolder*,WellTieParams*);
    		~uiWellTieInfoDlg();

    Notifier<uiWellTieInfoDlg>  applyPushed;

    bool 			setUserDepths();
    void 			setXCorrel();
    void 			setWvlts();

protected:
   
    WellTieDataHolder*		dataholder_;
    WellTieParams*		params_;
    uiPushButton*               applymarkerbut_;
    uiPushButton*               applymrkbut_;
    uiGenInput*			botmrkfld_;
    uiGenInput*                 corrcoefffld_;
    uiGenInput*			topmrkfld_;
    uiWellTieCorrView*          crosscorr_;
    uiWellTieWaveletView*       wvltdraw_;
    BufferStringSet             markernames_;
    Well::Data*                 wd_;
    
    void 			applyMarkerPushed(CallBacker*);
    void 			userDepthsChanged(CallBacker*);
};


#endif

