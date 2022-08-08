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

protected:
    bool		acceptOK(CallBacker*) override;

    Table::FormatDesc&	fd_;

    uiFileInput*	filefld_;
    uiTableImpDataSel*	dataselfld_;
    uiIOObjSel*		outputfld_;
};


}; //namespace

