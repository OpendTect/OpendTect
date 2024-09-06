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
namespace ZDomain { class Info; }

mExpClass(uiEarthModel) uiBulk2DHorizonImport : public uiDialog
{ mODTextTranslationClass(uiBul2DHorizonImport);
public:
			uiBulk2DHorizonImport(uiParent*);
			~uiBulk2DHorizonImport();

protected:

    bool		acceptOK(CallBacker*) override;
    void		zDomainCB(CallBacker*);

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;
    uiGenInput*		zdomselfld_;
    uiGenInput*		udftreatfld_;
    uiGenInput*		overwritefld_;
    Table::FormatDesc*	fd_;

};
