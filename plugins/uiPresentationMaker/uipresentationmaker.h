#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id: $
________________________________________________________________________

-*/


#include "uipresentationmakermod.h"
#include "uidialog.h"

#include "presentationspec.h"

class uiButtonGroup;
class uiComboBox;
class uiFileSel;
class uiGenInput;
class uiTable;
class uiToolButton;
class Timer;

mExpClass(uiPresentationMaker) uiPresentationMakerDlg : public uiDialog
{ mODTextTranslationClass(uiPresentationMakerDlg)
public:
			uiPresentationMakerDlg(uiParent*);
			~uiPresentationMakerDlg();

protected:
    bool		acceptOK();
    bool		checkInstallation();
    void		checkCB(CallBacker*);

    void		templateCB(CallBacker*);
    void		imageTypeCB(CallBacker*);
    void		settingsCB(CallBacker*);
    void		addCB(CallBacker*);
    void		moveUpCB(CallBacker*);
    void		moveDownCB(CallBacker*);
    void		removeCB(CallBacker*);
    void		createCB(CallBacker*);
    void		showLogCB(CallBacker*);

    void		updateWindowList();
    void		updateSceneList();
    void		updateScreenList();

    PresentationSpec	specs_;
    BufferString	pythonpptxdir_;
    BufferString	logfilenm_;
    Timer*		checktimer_;

    uiGenInput*		titlefld_;
    uiGenInput*		templatefld_;
    uiToolButton*	settingsbut_;
    uiButtonGroup*	typegrp_;
    uiComboBox*		windowfld_;
    uiComboBox*		scenefld_;
    uiComboBox*		screenfld_;
    uiTable*		slidestbl_;
    uiFileSel*		pptxfld_;
    uiFileSel*		outputfld_;
};
