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
class uiIOObjSel;

namespace Table { class FormatDesc; }

mExpClass(uiEarthModel) uiBulkFaultImport : public uiDialog
{ mODTextTranslationClass(uiBulkFaultImport)
public:
			uiBulkFaultImport(uiParent*,const char*typ=0,
							    bool is2d=false);
			~uiBulkFaultImport();

protected:

    bool		acceptOK();

    uiFileSel*		inpfld_;
    uiGenInput*		sortsticksfld_;
    uiIOObjSel*		fltsetnmfld_;
    uiTableImpDataSel*	dataselfld_;

    Table::FormatDesc*	fd_;

    bool		isfss_;
    bool		is2d_;
    bool		isfltset_;

    void		inpChangedCB(CallBacker*);
};
