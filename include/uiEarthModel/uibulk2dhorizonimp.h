#pragma once

/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : July 2017
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

