#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
 * ID       : $Id$
-*/

#include "uigoogleexpdlg.h"
class SurveyInfo;
class uiSurvey;
class uiGenInput;
class uiSelLineStyle;


mClass(uiGoogleIO) uiGoogleExportSurvey : public uiDialog
{ mODTextTranslationClass(uiGoogleExportSurvey);
public:

			uiGoogleExportSurvey(uiSurvey*);
			~uiGoogleExportSurvey();

protected:

    SurveyInfo*		si_;

    uiSelLineStyle*	lsfld_;
    uiGenInput*		hghtfld_;

			mDecluiGoogleExpStd;

};


