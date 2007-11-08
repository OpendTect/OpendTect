#ifndef uigoogleexpsurv_h
#define uigoogleexpsurv_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Nov 2007
 * ID       : $Id: uigoogleexpsurv.h,v 1.2 2007-11-08 13:04:17 cvsbert Exp $
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
