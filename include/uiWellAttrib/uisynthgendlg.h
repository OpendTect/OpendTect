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
class SynthGenParams;

class uiComboBox;
class uiFullSynthSeisSel;
class uiGenInput;
class uiListBox;
class uiLabeledComboBox;
class uiLabeledListBox;
class uiPushButton;

mExpClass(uiWellAttrib) uiSynthParsGrp : public uiGroup
{ mODTextTranslationClass(uiSynthParsGrp);
public:
			uiSynthParsGrp(uiParent*,StratSynth&);
			~uiSynthParsGrp();

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    CNotifier<uiSynthParsGrp,BufferString> synthAdded;
    CNotifier<uiSynthParsGrp,BufferStringSet> synthChanged;
    CNotifier<uiSynthParsGrp,BufferStringSet> synthRenamed;
    CNotifier<uiSynthParsGrp,BufferString> synthRemoved;
    CNotifier<uiSynthParsGrp,BufferString> synthDisabled;
    Notifier<uiSynthParsGrp> elPropSel;

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
    void			typeChgCB(CallBacker*);
    void			parsChangedCB(CallBacker*);
    void			nameChangedCB(CallBacker*);

    bool			getFromScreen(SynthGenParams&);
    void			putToScreen();

    bool			confirmSave();
    bool			doSave(const char* fnm);
    void			getPSNames(BufferStringSet&);
    void			getInpNames(BufferStringSet&);
    void			forwardInputNames();
    bool			prepareSyntheticToBeRemoved();
    bool			doAddSynthetic(const SynthGenParams&,
					       bool isupdate=false);
    bool			checkSyntheticName(const char* nm,
						   bool isupdate=false);

    StratSynth&			stratsynth_;
    uiListBox*			synthnmlb_;
    uiPushButton*		updatefld_;
    uiPushButton*		removefld_;

    uiFullSynthSeisSel*		synthselgrp_;
    uiPushButton*		addnewfld_;

    BufferString		lastsavedfnm_;
    bool			parschanged_ = false;
    bool			namechanged_ = false;

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
