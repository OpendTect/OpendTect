#ifndef uiattribtransdlg_h
#define uiattribtransdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          March 2006
 RCS:           $Id: uiattribtransdlg.h,v 1.3 2009/07/22 16:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

namespace visSurvey { class SurveyObject; }
class uiSliderExtra;


mClass uiAttribTransDlg : public uiDialog
{
public:
    				uiAttribTransDlg( uiParent*,
						  visSurvey::SurveyObject&,
				       		  int attrib	);
protected:
    bool			rejectOK(CallBacker*);
    void			changeCB(CallBacker*);

    unsigned char		initaltrans_;
    int				attrib_;
    visSurvey::SurveyObject&	so_;
    uiSliderExtra*		slider_;
};


#endif
