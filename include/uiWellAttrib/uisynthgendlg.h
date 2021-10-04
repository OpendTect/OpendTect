#pragma once
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
#include "uistring.h"

class StratSynth;

class uiComboBox;
class uiGenInput;
class uiFlatViewer;
class uiListBox;
class uiLabeledComboBox;
class uiPushButton;
class uiSynthSeisGrp;

mExpClass(uiWellAttrib) uiSynthGenDlg : public uiDialog
{ mODTextTranslationClass(uiSynthGenDlg);
public:
				uiSynthGenDlg(uiParent*,StratSynth&);

    bool			getFromScreen();
    void			putToScreen();
    void			updateSynthNames();
    void			updateWaveletName();
    bool			isCurSynthChanged() const;

    Notifier<uiSynthGenDlg>	genNewReq;
    CNotifier<uiSynthGenDlg,BufferString> synthRemoved;
    CNotifier<uiSynthGenDlg,BufferString> synthDisabled;
    CNotifier<uiSynthGenDlg,BufferString> synthChanged;

protected:
    void			updateFieldDisplay();

    uiComboBox*			typefld_;
    uiLabeledComboBox*		psselfld_;
    uiGenInput*			angleinpfld_;
    uiGenInput*			namefld_;
    uiPushButton*		gennewbut_;
    uiPushButton*		applybut_;
    uiPushButton*		revertbut_;
    uiPushButton*		savebut_;
    uiListBox*			synthnmlb_;
    StratSynth&			stratsynth_;
    uiSynthSeisGrp*		synthseis_;

    void			getPSNames(BufferStringSet&);
    bool			prepareSyntheticToBeChanged(bool toberemoved);
    void			typeChg(CallBacker*);
    void			genNewCB(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			removeSyntheticsCB(CallBacker*);
    void			changeSyntheticsCB(CallBacker*);
    void			parsChanged(CallBacker*);
    void			angleInpChanged(CallBacker*);
    void			nameChanged(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			finaliseDone(CallBacker*);
};
