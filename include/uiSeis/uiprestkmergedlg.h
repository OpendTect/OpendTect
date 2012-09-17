#ifndef uiprestkmergedlg_h
#define uiprestkmergedlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          October 2007
 RCS:           $Id: uiprestkmergedlg.h,v 1.10 2011/10/07 13:15:04 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstringset.h"

class CtxtIOObj;
class IOObj;
class MultiID;
class uiIOObjSel;
class uiPosSubSel;
class uiGenInput;
class uiListBox;
class uiToolButton;

/*! \brief: setup a dialog where the user can select a set of Pre-stack volumes and merge them into one. The priority order decides which volume to use in case of an overlap.
*/

mClass uiPreStackMergeDlg : public uiDialog
{
public:

    			uiPreStackMergeDlg(uiParent*);
    			~uiPreStackMergeDlg();

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
    uiGroup*		movebuttons_;

    uiGenInput*		stackfld_;
    uiIOObjSel*		outpfld_;
    uiPosSubSel*	subselfld_;
    
    CtxtIOObj&          inctio_;
    CtxtIOObj&          outctio_;
};


mClass uiPreStackCopyDlg : public uiDialog
{
public:

    			uiPreStackCopyDlg(uiParent*,const MultiID&);
    			~uiPreStackCopyDlg();

protected:

    void		objSel(CallBacker*);
    bool		acceptOK(CallBacker*);

    uiIOObjSel*		inpfld_;
    uiIOObjSel*		outpfld_;
    uiPosSubSel*	subselfld_;
    uiGenInput*		offsrgfld_;
    
    CtxtIOObj&          inctio_;
    CtxtIOObj&          outctio_;

};

#endif
