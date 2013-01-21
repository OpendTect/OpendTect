#ifndef uiprestackprocessor_h
#define uiprestackprocessor_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiprestackprocessingmod.h"
#include "iopar.h"
#include "multiid.h"
#include "uidialog.h"
#include "uigroup.h"
#include "factory.h"

class uiListBox;
class IOObj;
class uiButton;

namespace PreStack
{

class ProcessManager;
class Processor;
/*! An editor for a PreStackManager, with save/load possibilities. */
mExpClass(uiPreStackProcessing) uiProcessorManager : public uiGroup
{
public:
				uiProcessorManager(uiParent*,ProcessManager&);

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
    void			showPropDialog(int);
    void			factoryClickCB(CallBacker*);
    void			processorClickCB(CallBacker*);
    void			processorDoubleClickCB(CallBacker*);
    void			addProcessorCB(CallBacker*);
    void			removeProcessorCB(CallBacker*);
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


}; //namespace

#endif

