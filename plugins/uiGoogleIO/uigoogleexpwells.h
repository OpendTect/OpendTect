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
			~uiGISExportWells();

protected:

    uiMultiWellSel*	selfld_		= nullptr;
    uiColorInput*	colinput_	= nullptr;
    uiGenInput*		putnmfld_	= nullptr;
    uiGenInput*		lnmfld_		= nullptr;

    // might require icon name or color
    uiGISExpStdFld*	expfld_;
    bool		acceptOK(CallBacker*);
    void		putSel(CallBacker*);

    MultiID		multiid_;
    bool		ismultisel_;
};
