#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          October 2007
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "bufstringset.h"

class CtxtIOObj;
class IOObj;
class uiIOObjSel;
class uiPosSubSel;
class uiGenInput;
class uiListBox;
class uiToolButton;

/*! \brief: setup a dialog where the user can select a set of Prestack volumes
    and merge them into one. The priority order decides which volume to use in
    case of an overlap.
*/

mExpClass(uiSeis) uiPreStackMergeDlg : public uiDialog
{ mODTextTranslationClass(uiPreStackMergeDlg);
public:

			uiPreStackMergeDlg(uiParent*);
			~uiPreStackMergeDlg();

    void		setInputIds(const BufferStringSet& selnms);
protected:

    void		fillListBox();
    bool		setSelectedVols();

    void                createSelectButtons(uiGroup*);
    void                createMoveButtons(uiGroup*);
    void		createFields(uiGroup*);
    void		attachFields(uiGroup*,uiGroup*,uiGroup*);
    void		stackSel(CallBacker*);
    void		selButPush(CallBacker*);
    void		moveButPush(CallBacker*);
    bool		acceptOK(CallBacker*) override;
    void		setToolButtonProperty();

    BufferStringSet     allvolsnames_;
    TypeSet<MultiID>    allvolsids_;
    ObjectSet<IOObj>    selobjs_;

    uiListBox*		volsbox_;
    uiListBox*		selvolsbox_;

    uiToolButton*	toselect_;
    uiToolButton*	fromselect_;
    uiToolButton*	moveupward_;
    uiToolButton*	movedownward_;
    uiGroup*		movebuttons_;

    uiGenInput*		stackfld_;
    uiIOObjSel*		outpfld_;
    uiPosSubSel*	subselfld_;

    CtxtIOObj&          inctio_;
    CtxtIOObj&          outctio_;
};


