#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    uiGenInput*		lognmfld_;
    uiGenInput*		mdrangefld_;
    uiComboBox*		zunitfld_;
    uiGenInput*		nullfld_;
    uiGenInput*		colwidthfld_;
    uiASCIIFileInput*	lasfld_;

    RefMan<Well::Data>	wd_	= nullptr;
};
