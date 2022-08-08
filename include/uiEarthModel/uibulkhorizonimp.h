#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : May 2012
-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

class uiFileInput;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }

mExpClass(uiEarthModel) uiBulkHorizonImport : public uiDialog
{ mODTextTranslationClass(uiBulkHorizonImport);
public:
			uiBulkHorizonImport(uiParent*);
			~uiBulkHorizonImport();

protected:

    bool		acceptOK(CallBacker*) override;

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;
    Table::FormatDesc*	fd_;
};

