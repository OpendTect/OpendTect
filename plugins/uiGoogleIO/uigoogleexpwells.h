#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2009
 * ID       : $Id$
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
