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

class uiButtonGroup;
class uiComboBox;
class uiFileInput;
class uiGenInput;
class uiTable;
class PresentationSpec;

mExpClass(uiPresentationMaker) uiPresentationMakerDlg : public uiDialog
{ mODTextTranslationClass(uiPresentationMakerDlg)
public:
			uiPresentationMakerDlg(uiParent*);
			~uiPresentationMakerDlg();

protected:
    bool		acceptOK(CallBacker*);

    void		typeCB(CallBacker*);
    void		addCB(CallBacker*);
    void		moveUpCB(CallBacker*);
    void		moveDownCB(CallBacker*);
    void		removeCB(CallBacker*);
    void		createCB(CallBacker*);

    void		updateWindowList();
    void		updateSceneList();

    PresentationSpec&	specs_;

    uiGenInput*		titlefld_;
    uiButtonGroup*	typegrp_;
    uiComboBox*		windowfld_;
    uiComboBox*		scenefld_;
    uiTable*		slidestbl_;
    uiFileInput*	masterfld_;
    uiFileInput*	outputfld_;
    uiFileInput*	imagestorfld_;
};

#endif

