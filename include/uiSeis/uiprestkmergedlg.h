#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
