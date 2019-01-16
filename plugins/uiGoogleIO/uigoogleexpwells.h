#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2009
-*/

#include "uigoogleexpdlg.h"
class uiMultiWellSel;


mClass(uiGoogleIO) uiGISExportWells : public uiDialog
{ mODTextTranslationClass(uiGISExportWells);
public:

			uiGISExportWells(uiParent*);

protected:

    uiMultiWellSel*	selfld_;
    uiGISExpStdFld*	expfld_;
    bool		acceptOK();

};
