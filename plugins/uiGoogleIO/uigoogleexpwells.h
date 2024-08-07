#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigoogleexpdlg.h"

class uiMultiWellSel;
class uiColorInput;

mExpClass(uiGoogleIO) uiGISExportWells : public uiDialog
{ mODTextTranslationClass(uiGISExportWells);
public:

			uiGISExportWells(uiParent*,
					    const MultiID& mid=MultiID::udf());
			uiGISExportWells(uiParent*,const TypeSet<MultiID>&);
			~uiGISExportWells();

protected:

    uiMultiWellSel*	selfld_		= nullptr;
    uiColorInput*	colinput_	= nullptr;
    uiGenInput*		putnmfld_	= nullptr;
    uiGenInput*		lnmfld_		= nullptr;

    // might require icon name or color
    uiGISExpStdFld*	expfld_;

    void		createFields();

    bool		acceptOK(CallBacker*) override;
    void		putSel(CallBacker*);

    TypeSet<MultiID>	wellids_;
};
