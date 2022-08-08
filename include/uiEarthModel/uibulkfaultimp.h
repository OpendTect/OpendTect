#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : May 2012
-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }

mExpClass(uiEarthModel) uiBulkFaultImport : public uiDialog
{ mODTextTranslationClass(uiBulkFaultImport)
public:
			uiBulkFaultImport(uiParent*);
			uiBulkFaultImport(uiParent*,const char* type,bool is2d);
			~uiBulkFaultImport();

protected:

    void		init();
    bool		acceptOK(CallBacker*) override;

    void		inpChangedCB(CallBacker*);

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;
    uiGenInput* sortsticksfld_ = nullptr;
    uiIOObjSel* fltsetnmfld_ = nullptr;

    Table::FormatDesc*	fd_;
    bool		isfss_;
    bool        is2dfss_;
    bool        isfltset_;
};

