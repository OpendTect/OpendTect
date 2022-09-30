#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"

class uiFileInput;
class uiIOObjSel;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }

namespace PreStack
{

mExpClass(uiPreStackProcessing) uiEventImport : public uiDialog
{ mODTextTranslationClass(uiEventImport);
public:
    			uiEventImport(uiParent*);
			~uiEventImport();

protected:
    bool		acceptOK(CallBacker*) override;

    Table::FormatDesc&	fd_;

    uiFileInput*	filefld_;
    uiTableImpDataSel*	dataselfld_;
    uiIOObjSel*		outputfld_;
};

} // namespace PreStack
