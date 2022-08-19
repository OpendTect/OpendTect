#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
