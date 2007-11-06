#ifndef uigoogleexpsurv_h
#define uigoogleexpsurv_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Nov 2007
 * ID       : $Id: uigoogleexpsurv.h,v 1.1 2007-11-06 16:48:59 cvsbert Exp $
-*/

#include "uidialog.h"
class uiSurvey;
class SurveyInfo;
class uiFileInput;


class uiGoogleExportSurvey : public uiDialog
{
public:

			uiGoogleExportSurvey(uiSurvey*);
			~uiGoogleExportSurvey();

protected:

    uiFileInput*	fnmfld_;
    SurveyInfo*		si_;

    bool		acceptOK(CallBacker*);

};


#endif
