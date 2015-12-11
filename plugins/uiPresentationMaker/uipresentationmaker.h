#ifndef uipresentationmaker_h
#define uipresentationmaker_h

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
class uiFileInput;
class uiGenInput;
class uiTable;
class uiToolButton;

mExpClass(uiPresentationMaker) uiPresentationMakerDlg : public uiDialog
{ mODTextTranslationClass(uiPresentationMakerDlg)
public:
			uiPresentationMakerDlg(uiParent*);
			~uiPresentationMakerDlg();

protected:
    bool		acceptOK(CallBacker*);

    void		templateCB(CallBacker*);
    void		imageTypeCB(CallBacker*);
    void		settingsCB(CallBacker*);
    void		addCB(CallBacker*);
    void		moveUpCB(CallBacker*);
    void		moveDownCB(CallBacker*);
    void		removeCB(CallBacker*);
    void		createCB(CallBacker*);

    void		updateWindowList();
    void		updateSceneList();

    PresentationSpec	specs_;
    BufferString	pythonpptxdir_;

    uiGenInput*		titlefld_;
    uiGenInput*		templatefld_;
    uiToolButton*	settingsbut_;
    uiButtonGroup*	typegrp_;
    uiComboBox*		windowfld_;
    uiComboBox*		scenefld_;
    uiTable*		slidestbl_;
    uiFileInput*	masterfld_;
    uiFileInput*	outputfld_;
};

#endif
