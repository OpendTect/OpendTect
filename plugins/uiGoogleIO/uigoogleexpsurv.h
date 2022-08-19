#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
