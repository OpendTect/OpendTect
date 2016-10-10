#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
-*/

#include "uigoogleexpdlg.h"
class SurveyInfo;
class uiSurveyManager;
class uiGenInput;
class uiSelLineStyle;


mClass(uiGoogleIO) uiGoogleExportSurvey : public uiDialog
{ mODTextTranslationClass(uiGoogleExportSurvey);
public:

			uiGoogleExportSurvey(uiSurveyManager*);
			~uiGoogleExportSurvey();

protected:

    SurveyInfo*		si_;

    uiSelLineStyle*	lsfld_;
    uiGenInput*		hghtfld_;

			mDecluiGoogleExpStd;

};
