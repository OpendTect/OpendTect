#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"

#include "uidialog.h"
#include "uigroup.h"

#include "factory.h"
#include "iopar.h"
#include "multiid.h"

class uiButton;
class uiListBox;
class IOObj;

namespace PreStack
{

class ProcessManager;
class Processor;
/*! An editor for a PreStackManager, with save/load possibilities. */
mExpClass(uiPreStackProcessing) uiProcessorManager : public uiGroup
{ mODTextTranslationClass(uiProcessorManager);
public:
				uiProcessorManager(uiParent*,ProcessManager&);
				~uiProcessorManager();

    Notifier<uiProcessorManager>change;

    bool			restore();
    const MultiID&		lastMid() const		{ return lastmid_; }
    void 			setLastMid(const MultiID& mid);
    bool			isChanged() const	{ return changed_; }
    				/*!<Returns if processmanager is changed since
				    last save or load. */

    bool			save();

protected:
    void			updateList();
    void			updateButtons();
    bool			hasPropDialog(int) const;
    bool			showPropDialog(int);
    bool			showPropDialog(Processor&);
    void			factoryClickCB(CallBacker*);
    void			factoryDoubleClickCB(CallBacker*);
    void			processorClickCB(CallBacker*);
    void			processorDoubleClickCB(CallBacker*);
    void			addCB(CallBacker*);
    void			removeCB(CallBacker*);
    void			moveUpCB(CallBacker*);
    void			moveDownCB(CallBacker*);
    void			propertiesCB(CallBacker*);
    void			loadCB(CallBacker*);
    void			saveAsCB(CallBacker*);
    void			saveCB(CallBacker*);
    bool			doSave(const IOObj&);
    bool			doSaveAs();


    IOPar			restorepar_;

    ProcessManager&		manager_;
    MultiID			lastmid_;

    uiListBox*			factorylist_;
    uiButton*			addprocessorbutton_;
    uiButton*			removeprocessorbutton_;
    uiListBox*			processorlist_;
    uiButton*			moveupbutton_;
    uiButton*			movedownbutton_;
    uiButton*			propertiesbutton_;
    uiButton*			savebutton_;
    uiButton*			saveasbutton_;
    uiButton*			loadbutton_;

    bool			changed_;
};


mDefineFactory2Param( uiPreStackProcessing, uiDialog, uiParent*, Processor*,
		      uiPSPD );

} // namespace PreStack
