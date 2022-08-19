#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "uiiosel.h"

class uiGenInput;
class uiListBox;
class uiFileInput;
class SurveyDiskLocation;

mGlobal(uiIo) bool doSurveySelection(SurveyDiskLocation& newsdl,
				    uiParent*,const SurveyDiskLocation* initsdl,
				    uiDialog::DoneResult*);

mExpClass(uiIo) uiSurveySelectDlg : public uiDialog
{ mODTextTranslationClass(uiSurveySelectDlg);
public:
			uiSurveySelectDlg(uiParent*,const char* survnm=nullptr,
					  const char* dataroot=nullptr,
					  bool forread=true,
					  bool needvalidrootdir=true,
					  bool selmultiplesurveys=false);
			~uiSurveySelectDlg();

    void		setSurveyDiskLocation(const SurveyDiskLocation&);
    void		setDataRoot(const char*);
    void		setSurveyName(const char*);

    bool		isNewSurvey() const;
    SurveyDiskLocation	surveyDiskLocation() const;
    BufferString	getDataRoot() const;
    BufferString	getSurveyName() const;
    BufferString	getSurveyPath() const;

    void		getSurveyLocations(TypeSet<SurveyDiskLocation>&) const;
    void		getSurveyNames(BufferStringSet&) const;
    void		getSurveyPaths(BufferStringSet&) const;

private:

    void		rootSelCB(CallBacker*);
    void		surveySelCB(CallBacker*);
    void		fillSurveyList();
    bool		continueAfterErrMsg();

    uiFileInput*	datarootfld_;
    uiListBox*		surveylistfld_;
    uiGenInput*		surveyfld_ = nullptr;

    bool		forread_;
    bool		needvalidrootdir_;
};


mExpClass(uiIo) uiSurveySelect : public uiIOSelect
{ mODTextTranslationClass(uiSurveySelect);
public:
			uiSurveySelect(uiParent*,bool forread,
				       bool needvalidrootdir,
				       const char* label=nullptr);
			~uiSurveySelect();

    bool		isNewSurvey() const	{ return isnewsurvey_; }
    bool		getFullSurveyPath(BufferString&) const;
    void		setSurveyPath(const char*);
    void		setSurveyDiskLocation(const SurveyDiskLocation&);
    SurveyDiskLocation	surveyDiskLocation() const;

private:

    void		selectCB(CallBacker*);
    void		updateList();
    bool		isnewsurvey_;
    BufferString	dataroot_;
    BufferString	surveyname_;
    bool		forread_;
    bool		needvalidrootdir_;
};
