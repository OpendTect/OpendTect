#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "uigeninput.h"

class uiFileInput;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }

mExpClass(uiEarthModel) uiBulk2DHorizonImport : public uiDialog
{ mODTextTranslationClass(uiBul2DHorizonImport);
public:
			uiBulk2DHorizonImport(uiParent*);
			~uiBulk2DHorizonImport();

protected:

    bool		acceptOK(CallBacker*) override;

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;
    uiGenInput*		udftreatfld_;
    Table::FormatDesc*	fd_;

};
