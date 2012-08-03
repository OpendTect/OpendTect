#ifndef uivolprocchain_h
#define uivolprocchain_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uivolprocchain.h,v 1.13 2012-08-03 13:01:19 cvskris Exp $
________________________________________________________________________


-*/

#include "uivolumeprocessingmod.h"
#include "iopar.h"
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


mClass(uiVolumeProcessing) uiChain : public uiDialog
{
public:

				uiChain(uiParent*,Chain&, bool withprocessnow);
				~uiChain();

    const MultiID&		storageID() const;

    static const char*		pixmapFileName()    { return "volproc"; }

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

