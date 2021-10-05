#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
________________________________________________________________________

-*/

#include "uiwellattribmod.h"

#include "uidialog.h"
#include "uigroup.h"
#include "uistring.h"

class StratSynth;

class uiComboBox;
class uiGenInput;
class uiListBox;
class uiLabeledComboBox;
class uiPushButton;
class uiSynthSeisGrp;

mExpClass(uiWellAttrib) uiSynthParsGrp : public uiGroup
{ mODTextTranslationClass(uiSynthParsGrp);
public:
				uiSynthParsGrp(uiParent*,StratSynth&);
				~uiSynthParsGrp();

    bool			getFromScreen();
    void			putToScreen();
    void			updateSynthNames();
    void			updateWaveletName();
    bool			doAccept();
    bool			isCurSynthChanged() const;

    Notifier<uiSynthParsGrp>	genNewReq;
    CNotifier<uiSynthParsGrp,BufferString> synthRemoved;
    CNotifier<uiSynthParsGrp,BufferString> synthDisabled;
    CNotifier<uiSynthParsGrp,BufferString> synthChanged;

protected:

    void			initGrp(CallBacker*);
    void			changeSyntheticsCB(CallBacker*);
    void			removeSyntheticsCB(CallBacker*);
    void			typeChg(CallBacker*);
    void			angleInpChanged(CallBacker*);
    void			parsChanged(CallBacker*);
    void			nameChanged(CallBacker*);
    void			genNewCB(CallBacker*);

    void			updateFieldDisplay();
    void			getPSNames(BufferStringSet&);
    bool			prepareSyntheticToBeChanged(bool toberemoved);

    StratSynth&			stratsynth_;
    uiListBox*			synthnmlb_;
    uiComboBox*			typefld_;
    uiLabeledComboBox*		psselfld_;
    uiGenInput*			angleinpfld_;
    uiSynthSeisGrp*		synthseis_;
    uiGenInput*			namefld_;
    uiPushButton*		gennewbut_;

};


mExpClass(uiWellAttrib) uiSynthGenDlg : public uiDialog
{ mODTextTranslationClass(uiSynthGenDlg);
public:
				uiSynthGenDlg(uiParent*,StratSynth&);
				~uiSynthGenDlg();

    uiSynthParsGrp*		grp()		{ return uisynthparsgrp_; }
    const uiSynthParsGrp*	grp() const	{ return uisynthparsgrp_; }

private:

    bool			acceptOK(CallBacker*) override;

    uiSynthParsGrp*		uisynthparsgrp_;
};
