#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

/*! brief get an object from an other survey ( type is given by the CtxtIOObj ).
  will first pop-up a list to select the survey, then a list of the objects
  belonging to that survey !*/


#include "uiiomod.h"
#include "uidialog.h"

class CtxtIOObj;
class uiSurveySelect;

mExpClass(uiIo) uiSelObjFromOtherSurvey : public uiDialog
{ mODTextTranslationClass(uiSelObjFromOtherSurvey);
public:
			uiSelObjFromOtherSurvey(uiParent*,CtxtIOObj&);
			//IOobj constr.
			~uiSelObjFromOtherSurvey();

    void		setDirToCurrentSurvey();
    mDeprecated("Provide SurveyDiskLocation")
    void		setDirToOtherSurvey();
    void		setDirToOtherSurvey(const SurveyDiskLocation&);

    void		getIOObjFullUserExpression(BufferString& exp) const
			{ exp = fulluserexpression_; }

protected:

    uiSurveySelect*	selfld_;
    CtxtIOObj&		ctio_;
    BufferString	fulluserexpression_;
    BufferString	othersurveyrootdir_;

    bool		acceptOK(CallBacker*) override;
};
