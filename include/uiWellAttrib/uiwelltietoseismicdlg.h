#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    void			fillPar(IOPar&) const override;
    void			usePar(const IOPar&) override;

    static const char*		sKeyWinPar()	{ return "Well Tie Window"; }

protected:

    Server&			server_;
    EventStretch&		stretcher_;
    DispParams&			params_;

    uiCheckBox*			markerfld_;
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
    uiGenInput*			polarityfld_;

    uiControlView*		controlview_ = nullptr;
    uiInfoDlg*			infodlg_ = nullptr;
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
    uiCrossCorrView*		crosscorr_ = nullptr;
    uiWaveletView*		wvltdraw_ = nullptr;
    uiLabel*			wvltscaler_;

    BufferStringSet		markernames_;

    Interval<float>		zrg_;
    bool			zrginft_;
    int				selidx_ = 0;
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
