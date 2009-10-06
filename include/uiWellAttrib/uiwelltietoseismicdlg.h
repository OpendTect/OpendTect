#ifndef uiwelltietoseismicdlg_h
#define uiwelltietoseismicdlg_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
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

class uiGroup;
class uiToolBar;
class uiGenInput;
class uiPushButton;
class uiLabel;
class uiLabeledComboBox;
class uiCheckBox;
class uiWellLogDisplay;

namespace Attrib { class DescSet; }
namespace Well	 { class Data; }

namespace WellTie
{

class DataHolder;
class DataPlayer;
class uiTieView;
class uiControlView;
class uiCorrView;
class uiInfoDlg;
class uiWaveletView;
class uiEventStretch;

mClass uiTieWin : public uiFlatViewMainWin
{
public:

				uiTieWin(uiParent*,const WellTie::Setup&,
					  const Attrib::DescSet&); 
				~uiTieWin();


    const WellTie::Setup&		Setup()		{ return setup_; }
	
protected:

    Well::Data*			wd_;
    WellTie::Setup		setup_;
    WellTie::DataHolder*	dataholder_;
    WellTie::Params*		params_;
    WellTie::DataPlayer*   	dataplayer_;
    
    uiCheckBox* 		cscorrfld_;
    uiCheckBox* 		csdispfld_;
    uiCheckBox* 		markerfld_;
    uiCheckBox* 		zinftfld_;
    uiCheckBox* 		zintimefld_;
    uiGroup*            	vwrgrp_;
    uiLabeledComboBox*		eventtypefld_;
    uiPushButton*		infobut_;
    uiPushButton*		applybut_;
    uiPushButton*		undobut_;
    uiPushButton*		clearpicksbut_;
    uiPushButton*		clearlastpicksbut_;
    uiToolBar*          	toolbar_;
    ObjectSet<uiWellLogDisplay> logsdisp_;

    WellTie::uiControlView* 	controlview_;
    WellTie::uiInfoDlg* 	infodlg_; 
    WellTie::uiTieView*		datadrawer_;
    WellTie::uiEventStretch* 	stretcher_;
    
    void			addControl();
    void 			addToolBarTools();
    void			createViewerTaskFields(uiGroup*);
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
    void 			applyShiftPushed(CallBacker*);
    bool			compute(CallBacker*);
    void			checkIfPick(CallBacker*);
    void			checkShotChg(CallBacker*);
    void			checkShotDisp(CallBacker*);
    void 			csCorrChanged(CallBacker*);
    void			clearLastPick(CallBacker*);
    void			clearPicks(CallBacker*);
    void 			timeChanged(CallBacker*);
    void 			dispParPushed(CallBacker*);
    void 			dispPropChg(CallBacker*);
    void 			displayUserMsg(CallBacker*);
    bool 			doWork(CallBacker*);
    void			drawUserPick(CallBacker*);
    void 			editD2TPushed(CallBacker*);
    void 			infoPushed(CallBacker*);
    void 			provideWinHelp(CallBacker*);
    bool			rejectOK(CallBacker*);
    void 			setView(CallBacker*);
    void 			saveDataPushed(CallBacker*);
    void			eventTypeChg(CallBacker*);
    bool 			undoPushed(CallBacker*);
    void			userDepthsChanged(CallBacker*);

};



mClass uiInfoDlg : public uiDialog
{
public:		
    		
    		uiInfoDlg(uiParent*,WellTie::DataHolder*,WellTie::DataPlayer*);
    		~uiInfoDlg();

    Notifier<uiInfoDlg>  redrawNeeded;

    void 			drawData();
    bool 			getMarkerDepths(Interval<float>& zrg );

protected:
   
    BufferStringSet             markernames_;
    Well::Data*                 wd_;
    WellTie::DataHolder*	dataholder_;
    WellTie::Params::DataParams* params_;

    ObjectSet<uiGenInput>	zrangeflds_;
    ObjectSet<uiLabel>		zlabelflds_;
    uiGenInput*                 choicefld_;
    uiGenInput*                 estwvltlengthfld_;
    uiPushButton*               savewvltestbut_;
    WellTie::uiCorrView*      	crosscorr_;
    WellTie::uiWaveletView*     wvltdraw_;
    WellTie::DataPlayer*   	dataplayer_;
    
    void 			applyMarkerPushed(CallBacker*);
    void 			propChanged(CallBacker*);
    void 			wvltChanged(CallBacker*);
};

}; //namespace WellTie

#endif

