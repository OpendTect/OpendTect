#ifndef uigoogleexpsurv_h
#define uigoogleexpsurv_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
 * ID       : $Id: uigoogleexpsurv.h,v 1.3 2009-07-22 16:01:28 cvsbert Exp $
-*/

#include "uidialog.h"
class uiSurvey;
class SurveyInfo;
class uiFileInput;
class uiColorInput;
class uiGenInput;


class uiGoogleExportSurvey : public uiDialog
{
public:

			uiGoogleExportSurvey(uiSurvey*);
			~uiGoogleExportSurvey();

protected:

    uiColorInput*	colfld_;
    uiGenInput*		hghtfld_;
    uiFileInput*	fnmfld_;
    SurveyInfo*		si_;

    bool		acceptOK(CallBacker*);

};


#endif
