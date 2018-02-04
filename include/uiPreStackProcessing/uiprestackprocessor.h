#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
________________________________________________________________________


-*/

#include "uiprestackprocessingmod.h"

#include "uidialog.h"
#include "uigroup.h"

#include "factory.h"
#include "iopar.h"
#include "dbkey.h"

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
    mDefineFactory2ParamInClass( uiDialog, uiParent*, Processor*, factory );

				uiProcessorManager(uiParent*,ProcessManager&);

    Notifier<uiProcessorManager>change;

    bool			restore();
    const DBKey&		lastMid() const		{ return lastmid_; }
    void			setLastMid(const DBKey& mid);
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
    DBKey			lastmid_;

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


}; //namespace
