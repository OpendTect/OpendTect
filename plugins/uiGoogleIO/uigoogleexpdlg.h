#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
-*/

#include "uidialog.h"
#include "uigeninput.h"
#include "giswriter.h"
#include "geojsonwriter.h"
#include "uigroup.h"
#include "uicoordsystem.h"

class uiFileSel;
class SurveyInfo;
class uiSurveyManager;
uiString sKMLFileUiString();
uiString sOutFileName();
uiString sExportTypLbl();

mClass(uiGoogleIO) uiGISExpStdFld : public uiGroup
{ mODTextTranslationClass(uiGISExpStdFld)
public:
			    uiGISExpStdFld(uiParent*,BufferString typnm);
			    ~uiGISExpStdFld() {}

    GISWriter*			createWriter();
    uiFileSel*			fnmfld_;
    uiGenInput*			exptyp_;
    Coords::uiCoordSystemSel*	coordsysselfld_;

protected:

    void		expTypChng(CallBacker*);
};
