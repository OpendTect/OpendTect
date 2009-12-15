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
    
    const char*		    getDataRoot() const ;
    const BufferString	    getSurveyName() const;

protected:
    
    void		    surveySelectCB(CallBacker*);
    void		    fillSurveyList();
 
    BufferString	    basedirnm_;
    uiListBox*		    surveylistfld_;
    uiFileInput*	    datarootfld_;
};


mClass uiSurveySelect : public uiIOSelect
{
public:
			uiSurveySelect(uiParent*);
			~uiSurveySelect();
protected:
    void		selectCB(CallBacker*);
};

#endif
