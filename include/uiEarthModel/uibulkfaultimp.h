#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : May 2012
-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

class uiFileSel;
class uiGenInput;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }

mExpClass(uiEarthModel) uiBulkFaultImport : public uiDialog
{ mODTextTranslationClass(uiBulkFaultImport)
public:
			uiBulkFaultImport(uiParent*);
			~uiBulkFaultImport();

protected:

    bool		acceptOK();

    uiFileSel*		inpfld_;
    uiGenInput*		sortsticksfld_;
    uiTableImpDataSel*	dataselfld_;
    Table::FormatDesc*	fd_;

};
