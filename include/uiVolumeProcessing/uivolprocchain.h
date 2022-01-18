#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
________________________________________________________________________


-*/

#include "uivolumeprocessingmod.h"
#include "iopar.h"
#include "uivolprocstepdlg.h"

class IOObj;
class uiButton;
class uiIOObjSel;
class uiListBox;
class uiToolBar;


namespace VolProc
{

class Chain;
class Step;


mExpClass(uiVolumeProcessing) uiChain : public uiDialog
{ mODTextTranslationClass(uiChain);
public:

				uiChain(uiParent*,Chain&,
					bool withprocessnow,bool is2d=false);
				~uiChain();

    void			setChain(Chain&);
    const Chain&		getChain() const	{ return chain_; }
    const MultiID&		storageID() const;
    void			addStep(const char* steptype);
    void			emptyChain();

    static const char*		pixmapFileName()    { return "volproc"; }

protected:

    static const char*		sKeySettingKey();

    bool			acceptOK(CallBacker*);
    bool			doSave();
    bool			doSaveAs();
    void			updateList();
    void			updateButtons();
    void			updObj(const IOObj&);
    void			updWriteStatus(const IOObj*);
    bool			showPropDialog(int);

    void			readPush(CallBacker*);
    void			savePush(CallBacker*);
    void			saveAsPush(CallBacker*);

    void			factoryClickCB(CallBacker*);
    void			stepClickCB(CallBacker*);
    void			stepDoubleClickCB(CallBacker*);
    void			addStepPush(CallBacker*);
    void			removeStepPush(CallBacker*);
    void			moveUpCB(CallBacker*);
    void			moveDownCB(CallBacker*);
    void			propertiesCB(CallBacker*);

    static uiString		getPossibleInitialStepNames(bool);

    IOPar			restorepar_;
    Chain&			chain_;
    bool			is2d_;
    BufferStringSet		factorysteptypes_;

    uiListBox*			factorylist_;
    uiButton*			addstepbutton_;
    uiButton*			removestepbutton_;
    uiListBox*			steplist_;
    uiButton*			moveupbutton_;
    uiButton*			movedownbutton_;
    uiButton*			propertiesbutton_;
    uiIOObjSel*			objfld_;

private:
    uiToolBar*			tb_;
    int				savebuttonid_;

};

} // namespace VolProc
