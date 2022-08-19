#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigoogleexpdlg.h"
class uiGenInput;
class uiSelLineStyle;
namespace ODGoogle { class XMLWriter; }


mExpClass(uiGoogleIO) uiGISExportRandomLine : public uiDialog
{ mODTextTranslationClass(uiGISExportRandomLine);
public:

			uiGISExportRandomLine(uiParent*,
			    const TypeSet<Coord>* crd=nullptr,
			    const char*nm=nullptr);
			~uiGISExportRandomLine();

protected:

    const TypeSet<Coord>* crds_;

    uiGenInput*		putlnmfld_;
    uiGenInput*		lnmfld_;
    uiSelLineStyle*	lsfld_;
    uiGISExpStdFld*	expfld_;

    void		putSel(CallBacker*);
    bool		acceptOK(CallBacker*);
};
