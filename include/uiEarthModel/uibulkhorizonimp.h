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
class uiTableImpDataSel;

namespace Table { class FormatDesc; }
namespace ZDomain { class Info; }

mExpClass(uiEarthModel) uiBulkHorizonImport : public uiDialog
{ mODTextTranslationClass(uiBulkHorizonImport);
public:
			uiBulkHorizonImport(uiParent*);
			~uiBulkHorizonImport();

protected:

    bool		acceptOK(CallBacker*) override;
    void		zDomainCB(CallBacker*);

    uiFileInput*	    inpfld_;
    uiGenInput*		    zdomselfld_;
    uiTableImpDataSel*	    dataselfld_;
    Table::FormatDesc*	    fd_;
};
