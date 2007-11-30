#ifndef uiprestkmergedlg_h
#define uiprestkmergedlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          October 2007
 RCS:           $Id: uiprestkmergedlg.h,v 1.2 2007-11-30 07:01:18 cvsraman Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstringset.h"

class CtxtIOObj;
class IOObj;
class MultiID;
class uiIOObjSel;
class uiBinIDSubSel;
class uiGenInput;
class uiListBox;
class uiToolButton;

/*! \brief: setup a dialog where the user can select a set of Pre-stack volumes and merge them into one. The priority order decides which volume to use in case of an overlap.
*/

class uiPreStackMergeDlg : public uiDialog
{
public:
    			uiPreStackMergeDlg(uiParent*);

protected:
    void		fillListBox();
    bool		setSelectedVols();
    
    void                createSelectButtons(uiGroup*);
    void                createMoveButtons(uiGroup*);
    void		createFields(uiGroup*);
    void		attachFields(uiGroup*,uiGroup*,uiGroup*);
    void		selButPush(CallBacker*);
    void		moveButPush(CallBacker*);
    bool		acceptOK(CallBacker*);

    BufferStringSet     allvolsnames_;
    TypeSet<MultiID>    allvolsids_;
    ObjectSet<IOObj>    selobjs_;

    uiListBox*		volsbox_;
    uiListBox*		selvolsbox_;

    uiToolButton*	toselect_;
    uiToolButton*	fromselect_;
    uiToolButton*	moveupward_;
    uiToolButton*	movedownward_;

    uiIOObjSel*		outpfld_;
    uiBinIDSubSel*	subselfld_;
    
    CtxtIOObj&          inctio_;
    CtxtIOObj&          outctio_;
};

#endif
