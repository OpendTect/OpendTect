#ifndef uivolprocchain_h
#define uivolprocchain_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uivolprocchain.h,v 1.8 2009-05-14 21:18:16 cvskris Exp $
________________________________________________________________________


-*/

#include "iopar.h"
#include "factory.h"
#include "uivolprocstepdlg.h"

class IOObj;
class uiListBox;
class uiButton;
class uiMenuItem;
class uiGenInput;
class uiIOObjSel;


namespace VolProc
{

class Chain;
class Step;


mClass uiChain : public uiDialog
{
public:

    mDefineFactory2ParamInClass(uiStepDialog,uiParent*,Step*,factory);

				uiChain(uiParent*,Chain&, bool withprocessnow);
				~uiChain();

    const MultiID&		storageID() const;

    static const ioPixmap&	getPixmap();

protected:

    static const char*		sKeySettingKey();

    bool			acceptOK(CallBacker*);
    bool			doSave();
    bool			doSaveAs();
    void			updateList();
    void			updateButtons();
    void			updObj(const IOObj&);
    bool			showPropDialog(int);

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
