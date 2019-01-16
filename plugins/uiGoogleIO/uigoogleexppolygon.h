#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2009
-*/

#include "uigoogleexpdlg.h"
class uiGenInput;
class uiSelLineStyle;
namespace Pick { class Set; }


mClass(uiGoogleIO) uiGISExportPolygon : public uiDialog
{ mODTextTranslationClass(uiGISExportPolygon);
public:

			uiGISExportPolygon(uiParent*,const Pick::Set&);

protected:

    const Pick::Set&	ps_;

    uiSelLineStyle*	lsfld_;
    uiGenInput*		hghtfld_;
    uiGISExpStdFld*	expfld_;

    bool		acceptOK();
};
