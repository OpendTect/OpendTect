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
class uiGeom2DSel;
class uiIOObjSel;
class uiTableImpDataSel;
class uiVelocityDesc;

namespace Table { class FormatDesc; }

namespace Vel
{

mExpClass(uiVelocity) uiImportVelFunc : public uiDialog
{ mODTextTranslationClass(uiImportVelFunc);
public:
			uiImportVelFunc(uiParent*,bool is2d);
			~uiImportVelFunc();

private:

    uiFileInput*	inpfld_;
    uiVelocityDesc*	typefld_;
    uiGeom2DSel*	geom2dfld_;
    uiIOObjSel*		outfld_;

    Table::FormatDesc&	fd_;
    uiTableImpDataSel*	dataselfld_;
    bool		is2d_;

    void		initDlgCB(CallBacker*);
    void		inpSelCB(CallBacker*);
    void		velTypeChangeCB(CallBacker*);
    void		formatSel(CallBacker*);

    bool		acceptOK(CallBacker*) override;

};

} // namespace Vel
