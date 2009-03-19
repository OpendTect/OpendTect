#ifndef uivolprocchain_h
#define uivolprocchain_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uivolprocchain.h,v 1.5 2009-03-19 13:27:11 cvsbert Exp $
________________________________________________________________________


-*/

#include "iopar.h"
#include "factory.h"
#include "uidialog.h"
class IOObj;
class CtxtIOObj;
class uiListBox;
class uiButton;
class uiMenuItem;
class uiGenInput;
class uiIOObjSel;


namespace VolProc
{

class Chain;
class Step;

mClass uiStepDialog : public uiDialog
{
public:
    			uiStepDialog(uiParent*,const uiDialog::Setup&,
				     Step*);

    bool		acceptOK(CallBacker*);

protected:

    uiGenInput*		namefld_;
    Step*		step_;

};


mClass uiChain : public uiDialog
{
public:

    mDefineFactory2ParamInClass( uiStepDialog, uiParent*, Step*, factory );

				uiChain(uiParent*,Chain&);
				~uiChain();

    const MultiID&		storageID() const;

protected:

    bool			acceptOK(CallBacker*);
    bool			doSave();
    bool			doSaveAs();
    void			updateList();
    void			updateButtons();
    void			updObj(const IOObj&);
    void			showPropDialog(int);

    void			readPush(CallBacker*);
    void			savePush(CallBacker*);

    void			factoryClickCB(CallBacker*);
    void			stepClickCB(CallBacker*);
    void			stepDoubleClickCB(CallBacker*);
    void			addStepPush(CallBacker*);
    void			removeStepPush(CallBacker*);
    void			moveUpCB(CallBacker*);
    void			moveDownCB(CallBacker*);
    void			propertiesCB(CallBacker*);

    IOPar			restorepar_;
    Chain&			chain_;
    CtxtIOObj&			ctio_;

    uiMenuItem*			newmenu_;
    uiMenuItem*			loadmenu_;
    uiMenuItem*			savemenu_;
    uiMenuItem*			saveasmenu_;

    uiListBox*			factorylist_;
    uiButton*			addstepbutton_;
    uiButton*			removestepbutton_;
    uiListBox*			steplist_;
    uiButton*			moveupbutton_;
    uiButton*			movedownbutton_;
    uiButton*			propertiesbutton_;
    uiIOObjSel*			objfld_;

};


}; //namespace

#endif
