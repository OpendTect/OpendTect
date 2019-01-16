#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2009
-*/

#include "uigoogleexpdlg.h"
class uiGenInput;
class uiSelLineStyle;
namespace ODGoogle { class XMLWriter; }


mClass(uiGoogleIO) uiGoogleExportRandomLine : public uiDialog
{ mODTextTranslationClass(uiGoogleExportRandomLine);
public:

			uiGoogleExportRandomLine(uiParent*,
					const TypeSet<Coord>&,const uiString&);

protected:

    const TypeSet<Coord>& crds_;

    uiGenInput*		putlnmfld_;
    uiGenInput*		lnmfld_;
    uiSelLineStyle*	lsfld_;
    uiGISExpStdFld*	expfld_;

    void		putSel(CallBacker*);
    bool		acceptOK();
};
