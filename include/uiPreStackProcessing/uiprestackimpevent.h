#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          May 2011
________________________________________________________________________

-*/


#include "uiprestackprocessingmod.h"
#include "uidialog.h"

class uiFileSel;
class uiIOObjSel;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }

namespace PreStack
{

mExpClass(uiPreStackProcessing) uiEventImport : public uiDialog
{ mODTextTranslationClass(uiEventImport);
public:
			uiEventImport(uiParent*);

protected:
    bool		acceptOK();

    Table::FormatDesc&	fd_;

    uiFileSel*		filefld_;
    uiTableImpDataSel*	dataselfld_;
    uiIOObjSel*		outputfld_;
};

} // namespace PreStack
