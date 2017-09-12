#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : July 2017
-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uibutton.h"


class uiFileSel;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }

mExpClass(uiEarthModel) uiBulk2DHorizonImport : public uiDialog
{ mODTextTranslationClass(uiBul2DHorizonImport);
public:
			uiBulk2DHorizonImport(uiParent*, bool isbulk);
			~uiBulk2DHorizonImport();

protected:

    bool		    acceptOK();

    uiFileSel*		    inpfld_;
    uiTableImpDataSel*	    dataselfld_;
    uiGenInput*		    udftreatfld_;
    Table::FormatDesc*	    fd_;
    const bool		    isbulk_;

    bool		    getFileNames(BufferStringSet&) const;

};
