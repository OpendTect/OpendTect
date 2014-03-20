#ifndef uisynthgendlg_h
#define uisynthgendlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uidialog.h"

class StratSynth;

class uiComboBox;
class uiGenInput;
class uiFlatViewer;
class uiRayTracerSel;
class uiListBox;
class uiLabeledComboBox;
class uiPushButton;
class uiSynthGenDlg;
class uiSeisWaveletSel;

class uiSynthCorrectionsGrp;

mExpClass(uiWellAttrib) uiSynthGenDlg : public uiDialog
{
public:
				uiSynthGenDlg(uiParent*,StratSynth&);

    bool			getFromScreen();
    void			putToScreen();
    void			updateSynthNames();
    void			updateWaveletName();
    bool			isCurSynthChanged() const;

    Notifier<uiSynthGenDlg>	genNewReq;
    CNotifier<uiSynthGenDlg,BufferString> synthRemoved;
    CNotifier<uiSynthGenDlg,BufferString> synthChanged;

protected:
    void			updateFieldSensitivity();

    uiSeisWaveletSel*		wvltfld_;
    uiComboBox*			typefld_;
    uiLabeledComboBox*		psselfld_;
    uiGenInput*  		angleinpfld_;
    uiGenInput*  		namefld_;
    uiRayTracerSel*		rtsel_;
    uiPushButton*		gennewbut_;
    uiPushButton*		applybut_;
    uiPushButton*		revertbut_;
    uiPushButton*		savebut_;
    uiListBox*			synthnmlb_;
    StratSynth&			stratsynth_;
    uiSynthCorrectionsGrp*	uisynthcorrgrp_;


    void			getPSNames(BufferStringSet&);
    void			typeChg(CallBacker*);
    bool			genNewCB(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			removeSyntheticsCB(CallBacker*);
    void			changeSyntheticsCB(CallBacker*);
    void			parsChanged(CallBacker*);
    void			nameChanged(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			finaliseDone(CallBacker*);
};


class uiSynthCorrAdvancedDlg;

mExpClass(uiWellAttrib) uiSynthCorrectionsGrp : public uiGroup
{
public:
				uiSynthCorrectionsGrp(uiParent*);

    bool			wantNMOCorr() const;
    float			getStrechtMutePerc() const;
    float			getMuteLength() const;
    void			setValues(bool,float mutelen,float stretchlim);

    Notifier<uiSynthCorrectionsGrp> nmoparsChanged_;

protected:
    uiGenInput* 		nmofld_;
    uiPushButton*		advbut_;
    uiSynthCorrAdvancedDlg*	uiscadvdlg_;

    void			getAdvancedPush(CallBacker*);
    void			parsChanged(CallBacker*);
};

#endif

