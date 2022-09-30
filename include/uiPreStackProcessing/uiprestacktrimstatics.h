#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"

class uiGenInput;
class uiTable;
class uiToolButton;

namespace PreStack
{

class Processor;
class TrimStatics;

mExpClass(uiPreStackProcessing) uiTrimStatics : public uiDialog
{ mODTextTranslationClass(uiTrimStatics)
public:
    static void		initClass();
			uiTrimStatics(uiParent*,TrimStatics*);
			~uiTrimStatics();

protected:
    TrimStatics*	processor_;

    bool		acceptOK(CallBacker*) override;
    static uiDialog*	create(uiParent*,Processor*);

    void		fillTable();
    void		updateButtons();
    void		changeCB(CallBacker*);
    void		rowClickCB(CallBacker*);
    void		addCB(CallBacker*);
    void		rmCB(CallBacker*);
    void		moveUpCB(CallBacker*);
    void		moveDownCB(CallBacker*);

    uiTable*		table_;
    uiGenInput*		outputfld_;
    uiToolButton*	rmbut_;
    uiToolButton*	upbut_;
    uiToolButton*	downbut_;
};

} // namespace PreStack
