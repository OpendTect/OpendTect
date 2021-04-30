#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
-*/

#include "uigoogleexpdlg.h"

class SurveyInfo;
class uiSurvey;
class uiGenInput;
class uiSelLineStyle;


mExpClass(uiGoogleIO) uiGISExportSurvey : public uiDialog
{ mODTextTranslationClass(uiGISExportSurvey);
public:

			uiGISExportSurvey(uiParent*,SurveyInfo*);
			~uiGISExportSurvey();

protected:

    const SurveyInfo*	si_;

    uiSelLineStyle*	lsfld_;
    uiGenInput*		hghtfld_;
    uiGISExpStdFld*	expfld_;
    bool		acceptOK(CallBacker*);
};
