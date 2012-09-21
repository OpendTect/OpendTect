#ifndef uisurveyselect_h
#define uisurveyselect_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Dec 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "uiiosel.h"

class uiGenInput;
class uiListBox;
class uiFileInput;

mClass(uiIo) uiSurveySelectDlg : public uiDialog
{
public:
			uiSurveySelectDlg(uiParent*,const char* survnm=0,
					  const char* dataroot=0);
			~uiSurveySelectDlg();

    void		setDataRoot(const char*);
    const char*		getDataRoot() const;
    void		setSurveyName(const char*);
    const char*		getSurveyName() const;
    const BufferString	getSurveyPath() const;

    bool		isNewSurvey() const;

protected:
    
    void		rootSelCB(CallBacker*);
    void		surveySelCB(CallBacker*);
    void		fillSurveyList();
 
    uiFileInput*	datarootfld_;
    uiListBox*		surveylistfld_;
    uiGenInput*		surveyfld_;
};


mClass(uiIo) uiSurveySelect : public uiIOSelect
{
public:
			uiSurveySelect(uiParent*,const char* label=0);
			~uiSurveySelect();

    bool		isNewSurvey() const	{ return isnewsurvey_; }
    bool		getFullSurveyPath(BufferString&) const;
    void		setSurveyPath(const char*);

protected:

    void		selectCB(CallBacker*);
    bool		isnewsurvey_;
    BufferString	dataroot_;
    BufferString	surveyname_;
};

#endif

