#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Dec 2010
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
    void		setDirToOtherSurvey();

    void		getIOObjFullUserExpression(BufferString& exp) const
			{ exp = fulluserexpression_; }

protected:

    uiSurveySelect*	selfld_;
    CtxtIOObj&		ctio_;
    BufferString	fulluserexpression_;
    BufferString	othersurveyrootdir_;

    bool		acceptOK(CallBacker*) override;
};

