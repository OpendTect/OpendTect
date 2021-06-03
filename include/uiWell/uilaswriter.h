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
class uiComboBox;
class uiGenInput;
class uiListBox;
class uiWellSel;

namespace Well { class Data; }

mExpClass(uiWell) uiLASWriter : public uiDialog
{ mODTextTranslationClass(uiLASWriter)
public:
			uiLASWriter(uiParent*);
			~uiLASWriter();

protected:
    void		finalizeCB(CallBacker*);
    void		wellSelCB(CallBacker*);
    void		logSelCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    uiWellSel*		wellfld_;
    uiListBox*		logsfld_;
    uiGenInput*		mdrangefld_;
    uiComboBox*		zunitfld_;
    uiGenInput*		nullfld_;
    uiGenInput*		colwidthfld_;
    uiASCIIFileInput*	lasfld_;

    RefMan<Well::Data>	wd_	= nullptr;
};
