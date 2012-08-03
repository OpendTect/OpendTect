#ifndef uiattribtransdlg_h
#define uiattribtransdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          March 2006
 RCS:           $Id: uiattribtransdlg.h,v 1.4 2012-08-03 13:01:17 cvskris Exp $
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"

namespace visSurvey { class SurveyObject; }
class uiSliderExtra;


mClass(uiVis) uiAttribTransDlg : public uiDialog
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

