#ifndef uiprestackprocessor_h
#define uiprestackprocessor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uiprestackprocessor.h,v 1.7 2009-01-08 08:56:15 cvsranojay Exp $
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

class ProcessManager;
class Processor;

mClass uiProcessorManager : public uiGroup
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
    void			loadCB(CallBacker*);
    void			saveAsCB(CallBacker*);

    IOPar			restorepar_;

    ProcessManager&		manager_;

    uiListBox*			factorylist_;
    uiButton*			addprocessorbutton_;
    uiButton*			removeprocessorbutton_;
    uiListBox*			processorlist_;
    uiButton*			moveupbutton_;
    uiButton*			movedownbutton_;
    uiButton*			propertiesbutton_;
    uiButton*			saveasbutton_;
    uiButton*			loadbutton_;
};


mDefineFactory2Param( uiDialog, uiParent*, Processor*, uiPSPD );


}; //namespace

#endif
