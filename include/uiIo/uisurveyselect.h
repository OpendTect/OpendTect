#pragma once

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
class SurveyDiskLocation;

mExpClass(uiIo) uiSurveySelectDlg : public uiDialog
{ mODTextTranslationClass(uiSurveySelectDlg);
public:
			uiSurveySelectDlg(uiParent*,const char* survnm=0,
					  const char* dataroot=0,
					  bool forread=true,
					  bool needvalidrootdir=true);
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
    bool		continueAfterErrMsg();

    uiFileInput*	datarootfld_;
    uiListBox*		surveylistfld_;
    uiGenInput*		surveyfld_;

    bool		forread_;
    bool		needvalidrootdir_;
};


mExpClass(uiIo) uiSurveySelect : public uiIOSelect
{ mODTextTranslationClass(uiSurveySelect);
public:
			uiSurveySelect(uiParent*,bool forread,
				       bool needvalidrootdir,
				       const char* label=0);
			~uiSurveySelect();

    bool		isNewSurvey() const	{ return isnewsurvey_; }
    bool		getFullSurveyPath(BufferString&) const;
    void		setSurveyPath(const char*);
    void		setSurveyDiskLocation(const SurveyDiskLocation&);
    SurveyDiskLocation	surveyDiskLocation() const;

protected:

    void		selectCB(CallBacker*);
    bool		isnewsurvey_;
    BufferString	dataroot_;
    BufferString	surveyname_;
    bool		forread_;
    bool		needvalidrootdir_;
};

