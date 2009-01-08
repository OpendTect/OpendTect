#ifndef uiattribtransdlg_h
#define uiattribtransdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          March 2006
 RCS:           $Id: uiattribtransdlg.h,v 1.2 2009-01-08 10:37:54 cvsranojay Exp $
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
