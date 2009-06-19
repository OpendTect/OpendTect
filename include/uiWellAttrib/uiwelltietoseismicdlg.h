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

#include "uiflatviewmainwin.h"
#include "uidialog.h"
#include "welltiesetup.h"
#include "welltieunitfactors.h"
#include "bufstringset.h"

class WellTieDataHolder;
class WellTieToSeismic;

class uiGroup;
class uiToolBar;
class uiGenInput;
class uiPushButton;
class uiCheckBox;
class uiWellLogDisplay;

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

mClass uiWellTieToSeismicDlg : public uiFlatViewMainWin
{
public:

	    uiWellTieToSeismicDlg(uiParent*,const WellTieSetup&,
				const Attrib::DescSet&); 
	    ~uiWellTieToSeismicDlg();


    const WellTieSetup&		Setup()		{ return setup_; }    
	
protected:

    Well::Data*			wd_;
    WellTieSetup		setup_;
    WellTieDataHolder*		dataholder_;
    WellTieParams*		params_;
    WellTieToSeismic*   	dataplayer_;
    
    uiCheckBox* 		cscorrfld_;
    uiCheckBox* 		csdispfld_;
    uiCheckBox* 		markerfld_;
    uiCheckBox* 		zinftfld_;
    uiGroup*            	vwrgrp_;
    uiPushButton*		infobut_;
    uiPushButton*		applybut_;
    uiPushButton*		undobut_;
    uiPushButton*		clearpicksbut_;
    uiToolBar*          	toolbar_;
    ObjectSet<uiWellLogDisplay> logsdisp_;

    uiWellTieControlView* 	controlview_;
    uiWellTieEventStretch* 	eventstretcher_;
    uiWellTieInfoDlg* 		infodlg_; 
    uiWellTieView*		datadrawer_;

    
    bool			manip_;

    void			addControl();
    void 			addToolBarTools();
    void			checkIfPick();
    void			createTaskFields(uiGroup*);
    void			createDispPropFields(uiGroup*);
    void 			drawData();
    void 			drawFields();
    void 			getDispParams();
    void 			initAll();
    void 			putDispParams();
    void			resetInfoDlg();
    void	 		setTitle(const Attrib::DescSet&);

    //CallBackers
    bool			acceptOK(CallBacker*);
    void 			applyPushed(CallBacker*);
    void 			applyReady(CallBacker*);
    void 			applyMarkerPushed(CallBacker*);
    void 			applyShiftPushed(CallBacker*);
    void			checkShotChg(CallBacker*);
    void			checkShotDisp(CallBacker*);
    void			clearPicksPushed(CallBacker*);
    void 			dispDataChanged(CallBacker*);
    void 			dispParPushed(CallBacker*);
    void 			dispPropChg(CallBacker*);
    void 			doWork(CallBacker*);
    void			drawUserPick(CallBacker*);
    void 			editD2TPushed(CallBacker*);
    void 			infoPushed(CallBacker*);
    bool			rejectOK(CallBacker*);
    void 			setView(CallBacker*);
    bool			saveD2TPushed(CallBacker*);
    bool 			undoPushed(CallBacker*);
    void			userDepthsChanged(CallBacker*);
};



mClass uiWellTieInfoDlg : public uiDialog
{
public:		
    		
    		uiWellTieInfoDlg(uiParent*,WellTieDataHolder*);
    		~uiWellTieInfoDlg();

    Notifier<uiWellTieInfoDlg>  applyPushed;

    bool 			setUserDepths();
    void 			setXCorrel();
    void 			setWvlts();

protected:
   
    BufferStringSet             markernames_;
    Well::Data*                 wd_;
    WellTieDataHolder*		dataholder_;
    WellTieParams::DataParams*	params_;

    uiGenInput*			botmrkfld_;
    uiGenInput*                 corrcoefffld_;
    uiGenInput*			topmrkfld_;
    uiPushButton*               applymarkerbut_;
    uiPushButton*               applymrkbut_;
    uiWellTieCorrView*          crosscorr_;
    uiWellTieWaveletView*       wvltdraw_;
    
    void 			applyMarkerPushed(CallBacker*);
    void 			userDepthsChanged(CallBacker*);
};


#endif

