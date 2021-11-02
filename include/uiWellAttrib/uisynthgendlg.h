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

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    void		updateWaveletName();

    CNotifier<uiSynthParsGrp,BufferString> synthAdded;
    CNotifier<uiSynthParsGrp,BufferString> synthChanged;
    CNotifier<uiSynthParsGrp,BufferString> synthRemoved;
    CNotifier<uiSynthParsGrp,BufferString> synthDisabled;
    Notifier<uiSynthParsGrp>	elPropSel;

protected:

    void			initGrp(CallBacker*);
    void			addSyntheticsCB(CallBacker*);
    void			newSynthSelCB(CallBacker*);
    void			updateSyntheticsCB(CallBacker*);
    void			removeSyntheticsCB(CallBacker*);
    void			elPropSelCB(CallBacker*);
    void			newCB(CallBacker*);
    void			openCB(CallBacker*);
    void			saveCB(CallBacker*);
    void			saveAsCB(CallBacker*);
    void			typeChg(CallBacker*);
    void			parsChanged(CallBacker*);
    void			nameChanged(CallBacker*);

    bool			getFromScreen();
    void			putToScreen();

    bool			confirmSave();
    bool			doSave(const char* fnm);
    void			updateFieldDisplay();
    void			getPSNames(BufferStringSet&);
    bool			prepareSyntheticToBeChanged(bool toberemoved);
    bool			doAddSynthetic(bool isupdate=false);

    StratSynth&			stratsynth_;
    uiListBox*			synthnmlb_;
    uiPushButton*		updatefld_;
    uiPushButton*		removefld_;

    uiComboBox*			typefld_;
    uiLabeledComboBox*		psselfld_;
    uiGenInput*			angleinpfld_;
    uiSynthSeisGrp*		synthseis_;
    uiGenInput*			namefld_;
    uiPushButton*		addnewfld_;

    BufferString		lastsavedfnm_;

};


mExpClass(uiWellAttrib) uiSynthGenDlg : public uiDialog
{ mODTextTranslationClass(uiSynthGenDlg);
public:
				uiSynthGenDlg(uiParent*,StratSynth&);
				~uiSynthGenDlg();

    uiSynthParsGrp*		grp()		{ return uisynthparsgrp_; }
    const uiSynthParsGrp*	grp() const	{ return uisynthparsgrp_; }

private:

    uiSynthParsGrp*		uisynthparsgrp_;
};
