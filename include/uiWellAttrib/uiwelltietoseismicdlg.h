#pragma once

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          January 2009
RCS:           $Id: uiwellwelltietoseismicdlg.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uidialog.h"
#include "uiflatviewmainwin.h"
#include "bufstringset.h"
#include "uistring.h"

class Wavelet;
class uiCheckBox;
class uiGenInput;
class uiGroup;
class uiLabel;
class uiLabeledComboBox;
class uiPushButton;
class uiSeisWaveletSel;
class uiToolBar;
class uiWellLogDisplay;

namespace Well	 { class Data; }

namespace WellTie
{

class Data;
class DispParams;
class Setup;
class Server;
class EventStretch;
class uiControlView;
class uiCrossCorrView;
class uiInfoDlg;
class uiTieView;
class uiWaveletView;


mExpClass(uiWellAttrib) uiTieWin : public uiFlatViewMainWin
{ mODTextTranslationClass(uiTieWin);
public:

				uiTieWin(uiParent*,WellTie::Server&);
				~uiTieWin();

    const WellTie::Setup&	welltieSetup() const;

    void			fillPar(IOPar&) const;
    void			usePar(const IOPar&);

    static const char*		sKeyWinPar()	{ return "Well Tie Window"; }

protected:

    Server&			server_;
    EventStretch&		stretcher_;
    DispParams&			params_;

    uiCheckBox*			markerfld_;

    mDeprecatedObs
    uiCheckBox*			zinftfld_;
    mDeprecatedObs
    uiCheckBox*			zintimefld_;

    uiGroup*			vwrgrp_;
    uiLabeledComboBox*		eventtypefld_;
    uiGenInput*			nrtrcsfld_;
    uiSeisWaveletSel*		wvltfld_;
    uiPushButton*		infobut_;
    uiPushButton*		applybut_;
    uiPushButton*		undobut_;
    uiPushButton*		clearpicksbut_;
    uiPushButton*		clearlastpicksbut_;
    uiPushButton*		matchhormrksbut_;
    uiToolBar*			toolbar_;
    uiGenInput*			polarityfld_();

    uiControlView*		controlview_;
    uiInfoDlg*			infodlg_;
    uiTieView*			drawer_;
    IOPar			par_;

    void			addControls();
    void			addToolBarTools();
    void			createViewerTaskFields(uiGroup*);
    void			drawFields();
    void			initAll();
    void			resetInfoDlg();

    void			okPushCB(CallBacker*);
    void			applyPushed(CallBacker*);
    void			applyShiftPushed(CallBacker*);
    void			checkIfPick(CallBacker*);
    void			checkShotChg(CallBacker*);
    void			checkShotDisp(CallBacker*);
    void			clearLastPick(CallBacker*);
    void			clearPicks(CallBacker*);
    void			dispParPushed(CallBacker*);
    void			dispInfoMsg(CallBacker*);
    void			displayUserMsg(CallBacker*);
    void			doWork(CallBacker*);
    void			drawUserPick(CallBacker*);
    void			editD2TPushed(CallBacker*);
    void			eventTypeChg(CallBacker*);
    void			infoPushed(CallBacker*);
    void			matchHorMrks(CallBacker*);
    void			provideWinHelp(CallBacker*);
    void			reDrawSeisViewer(CallBacker*);
    void			reDrawAuxDatas(CallBacker*);
    void			reDrawAll(CallBacker*);
    void			cancelPushCB(CallBacker*);
    void			setView(CallBacker*);
    void			saveDataPushed(CallBacker*);
    void			timeChanged(CallBacker*);
    void			undoPushed(CallBacker*);
    void			userDepthsChanged(CallBacker*);
    void			snapshotCB(CallBacker*);
    void			cleanUp(CallBacker*);
    void			nrtrcsCB(CallBacker*);
    void			wvltSelCB(CallBacker*);
    void			polarityChanged(CallBacker*);

    mDeprecatedObs
    void			getDispParams();
    mDeprecatedObs
    void			putDispParams();
    mDeprecatedObs
    void			dispPropChg(CallBacker*);
    mDeprecatedObs
    void			createDispPropFields(uiGroup*);
};



mExpClass(uiWellAttrib) uiInfoDlg : public uiDialog
{ mODTextTranslationClass(uiInfoDlg);
public:

				uiInfoDlg(uiParent*,Server&);
				~uiInfoDlg();

    Notifier<uiInfoDlg>		redrawNeeded;

    void			drawData();
    bool			getMarkerDepths(Interval<float>& zrg );
    void			dtmodelChanged(CallBacker*);
    const Wavelet&		getWavelet() const;
    bool			isInitWvltActive() const;

    void			updateInitialWavelet();

    void			fillPar(IOPar&) const;
    void			usePar(const IOPar&);

protected:

    Server&			server_;
    ObjectSet<uiGenInput>	zrangeflds_;
    ObjectSet<uiLabel>		zlabelflds_;
    uiGenInput*			choicefld_;
    uiGenInput*			estwvltlengthfld_;
    uiCrossCorrView*		crosscorr_;
    uiWaveletView*		wvltdraw_;
    uiLabel*			wvltscaler_;

    BufferStringSet		markernames_;

    Interval<float>		zrg_;
    bool			zrginft_;
    int				selidx_;
    BufferString		startmrknm_;
    BufferString		stopmrknm_;

    const Data&			data_;

    void			putToScreen();
    bool			updateZrg();
    bool			computeNewWavelet();

    void			applyMarkerPushed(CallBacker*);
    void			wvltChanged(CallBacker*);
    void			needNewEstimatedWvlt(CallBacker*);
    void			zrgChanged(CallBacker*);
    void			synthChanged(CallBacker*);
    void			crossCorrelationChanged(CallBacker*);
    void			crossAttribsParsChanged(CallBacker*);
};

} // namespace WellTie

