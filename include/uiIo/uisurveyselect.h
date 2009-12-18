#ifndef uisurveyselect_h
#define uisurveyselect_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Dec 2009
 RCS:           $Id: uisurveyselect.h
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uiiosel.h"

class uiGenInput;
class uiListBox;
class uiFileInput;

mClass uiSurveySelectDlg : public uiDialog
{
public:
			uiSurveySelectDlg(uiParent*);
			~uiSurveySelectDlg();
    
    const char*		    getDataRoot() const;
    const BufferString	    getSurveyName() const;
    bool		    isNewSurvey();

protected:
    
    void		    rootSelectCB(CallBacker*);
    void		    surveyListCB(CallBacker*);
    void		    fillSurveyList();
 
    uiFileInput*	    datarootfld_;
    uiListBox*		    surveylistfld_;
    uiGenInput*		    newsurveyfld_;
};


mClass uiSurveySelect : public uiIOSelect
{
public:
			uiSurveySelect(uiParent*);
			~uiSurveySelect(); 
    bool		isNewSurvey();
protected:
    bool		isnewsurvey_;
    void		selectCB(CallBacker*);
};

#endif
