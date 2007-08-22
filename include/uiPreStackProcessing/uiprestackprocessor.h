#ifndef uiprestackprocessor_h
#define uiprestackprocessor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uiprestackprocessor.h,v 1.4 2007-08-22 06:30:21 cvskris Exp $
________________________________________________________________________


-*/

#include "iopar.h"
#include "uidialog.h"
#include "uigroup.h"
#include "factory.h"

class uiListBox;
class uiButton;

namespace PreStack
{

void initBuiltinUiClasses();

class ProcessManager;
class Processor;

class uiProcessorManager : public uiGroup
{
public:
				uiProcessorManager(uiParent*,ProcessManager&);

    Notifier<uiProcessorManager>change;
    bool			restore();

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

    IOPar			restorepar_;

    ProcessManager&		manager_;

    uiListBox*			factorylist_;
    uiButton*			addprocessorbutton_;
    uiButton*			removeprocessorbutton_;
    uiListBox*			processorlist_;
    uiButton*			moveupbutton_;
    uiButton*			movedownbutton_;
    uiButton*			propertiesbutton_;
};


mDefineFactory2Param( uiDialog, uiParent*, Processor*, uiPSPD );


}; //namespace

#endif
