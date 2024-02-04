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
namespace ZDomain { class Info; }

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
    void		stickSelCB(CallBacker*);
    void		zDomainCB(CallBacker*);

    const ZDomain::Info& zDomain() const;
    bool		isASCIIFileInTime() const;

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;
    uiGenInput*		zdomselfld_;
    uiGenInput*		sortsticksfld_ = nullptr;
    uiIOObjSel*		fltsettimefld_ = nullptr;
    uiIOObjSel*		fltsetdepthfld_ = nullptr;
    uiGenInput*		stickselfld_ = nullptr;
    uiGenInput*		thresholdfld_ = nullptr;

    Table::FormatDesc*	fd_;
    bool		isfss_;
    bool		is2dfss_;
    bool		isfltset_;
};
