#pragma once

/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : July 2017
-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "uigeninput.h"

class uiFileInput;
class uiTableImpDataSel;
namespace EM { class BulkHorizon2DScanner; }
class uiPushButton;

namespace Table { class FormatDesc; }

mExpClass(uiEarthModel) uiBulk2DHorizonImport : public uiDialog
{ mODTextTranslationClass(uiBul2DHorizonImport);
public:
			uiBulk2DHorizonImport(uiParent*);
			~uiBulk2DHorizonImport();

protected:

    bool			acceptOK(CallBacker*);
    void			scanPush(CallBacker*);
    void			descChg(CallBacker*);
    void			scanButState(CallBacker*);

    uiFileInput*		inpfld_;
    uiTableImpDataSel*		dataselfld_;
    uiGenInput*			udftreatfld_;
    Table::FormatDesc*		fd_;
    EM::BulkHorizon2DScanner*	scanner_ = nullptr;

    bool			getFileNames(BufferStringSet&) const;
    bool			doImport();
};

