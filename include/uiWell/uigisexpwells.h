#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"

#include "uidialog.h"

class uiColorInput;
class uiGenInput;
class uiGISExpStdFld;
class uiMultiWellSel;


mExpClass(uiWell) uiGISExportWells : public uiDialog
{
mODTextTranslationClass(uiGISExportWells);
public:

			uiGISExportWells(uiParent*,
					 const TypeSet<MultiID>* =nullptr);
			~uiGISExportWells();

    void		setSelected(const MultiID&);
    void		setSelected(const TypeSet<MultiID>&);

private:

    bool		acceptOK(CallBacker*) override;

    uiMultiWellSel*	selfld_;
    uiColorInput*	colinput_	= nullptr;
    uiGenInput*		prefixfld_	= nullptr;
    uiGISExpStdFld*	expfld_;
};
