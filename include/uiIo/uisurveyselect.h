#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Dec 2009
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uidialog.h"

class uiGenInput;
class uiListBox;
class uiFileInput;


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
    void		fillSurveyList(bool initial);
    bool		continueAfterErrMsg();

    uiFileInput*	datarootfld_;
    uiListBox*		surveylistfld_;
    uiGenInput*		surveyfld_;

    bool		forread_;
    bool		needvalidrootdir_;
    BufferString	dataroot_;
};
