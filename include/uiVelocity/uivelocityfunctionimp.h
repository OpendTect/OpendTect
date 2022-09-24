#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivelocitymod.h"
#include "uidialog.h"

class uiFileInput;
class uiIOObjSel;
class uiTableImpDataSel;
class uiVelocityDesc;

namespace Table { class FormatDesc; }

namespace Vel
{

mExpClass(uiVelocity) uiImportVelFunc : public uiDialog
{ mODTextTranslationClass(uiImportVelFunc);
public:
			uiImportVelFunc(uiParent*);
			~uiImportVelFunc();

protected:

    uiFileInput*	inpfld_;
    uiVelocityDesc*	typefld_;
    uiIOObjSel*		outfld_;

    Table::FormatDesc&	fd_;
    uiTableImpDataSel*	dataselfld_;

    void		velTypeChangeCB(CallBacker*);
    void		formatSel(CallBacker*);

    bool		acceptOK(CallBacker*) override;

};

} // namespace Vel
