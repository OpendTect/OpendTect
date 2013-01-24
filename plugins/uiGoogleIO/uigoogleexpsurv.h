#ifndef uigoogleexpsurv_h
#define uigoogleexpsurv_h
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
{
public:

			uiGoogleExportSurvey(uiSurvey*);
			~uiGoogleExportSurvey();

protected:

    SurveyInfo*		si_;

    uiSelLineStyle*	lsfld_;
    uiGenInput*		hghtfld_;

			mDecluiGoogleExpStd;

};


#endif
