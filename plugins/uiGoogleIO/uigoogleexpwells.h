#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2009
-*/

#include "uigoogleexpdlg.h"
class uiMultiWellSel;


mClass(uiGoogleIO) uiGoogleExportWells : public uiDialog
{ mODTextTranslationClass(uiGoogleExportWells);
public:

			uiGoogleExportWells(uiParent*);

protected:

    uiMultiWellSel*	selfld_;

			mDecluiGoogleExpStd;
};
