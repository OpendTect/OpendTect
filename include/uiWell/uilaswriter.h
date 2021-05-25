#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2021
________________________________________________________________________

-*/

#include "uiwellmod.h"

#include "uidialog.h"

class uiASCIIFileInput;
class uiListBox;
class uiWellSel;

mExpClass(uiWell) uiLASWriter : public uiDialog
{ mODTextTranslationClass(uiLASWriter)
public:
			uiLASWriter(uiParent*);
			~uiLASWriter();

protected:
    bool		acceptOK(CallBacker*) override;
    void		wellSelCB(CallBacker*);

    uiWellSel*		wellfld_;
    uiListBox*		logsfld_;
    uiASCIIFileInput*	lasfld_;
};
