#ifndef uigoogleexpsurv_h
#define uigoogleexpsurv_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
 * ID       : $Id: uigoogleexpsurv.h,v 1.4 2009-11-16 13:56:10 cvsbert Exp $
-*/

#include "uigoogleexpdlg.h"
class uiSurvey;
class SurveyInfo;
class uiGenInput;
class uiColorInput;


class uiGoogleExportSurvey : public uiDialog
{
public:

			uiGoogleExportSurvey(uiSurvey*);
			~uiGoogleExportSurvey();

protected:

    SurveyInfo*		si_;

    uiColorInput*	colfld_;
    uiGenInput*		hghtfld_;

			mDecluiGoogleExpStd;

};


#endif
