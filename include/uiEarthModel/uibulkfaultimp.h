#ifndef uibulkfaultimp_h
#define uibulkfaultimp_h
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

mExpClass(uiEarthModel) uiBulkFaultImport : public uiDialog
{ mODTextTranslationClass(uiBulkFaultImport);
public:
			uiBulkFaultImport(uiParent*);
			~uiBulkFaultImport();

protected:

    bool		acceptOK(CallBacker*);

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;
    Table::FormatDesc*	fd_;
};

#endif
