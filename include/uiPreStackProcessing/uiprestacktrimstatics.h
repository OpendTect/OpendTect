#ifndef uiprestacktrimstatics_h
#define uiprestacktrimstatics_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2014
 RCS:		$Id$
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

protected:
    TrimStatics*	processor_;

    bool		acceptOK(CallBacker*);
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

#endif
