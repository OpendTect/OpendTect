#ifndef uiselobjfromothersurv_h
#define uiselobjfromothersurv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Dec 2010
 RCS:           $Id$
________________________________________________________________________

-*/

/*! brief get an object from an other survey ( type is given by the CtxtIOObj ).
  will first pop-up a list to select the survey, then a list of the objects 
  belonging to that survey !*/


#include "uiiomod.h"
#include "uiselsimple.h"

class CtxtIOObj;

mClass(uiIo) uiSelObjFromOtherSurvey : public uiSelectFromList
{
public:
    			uiSelObjFromOtherSurvey(uiParent*,CtxtIOObj&);
    			~uiSelObjFromOtherSurvey();

    void		setDirToCurrentSurvey();
    void		setDirToOtherSurvey();

    void		getIOObjFullUserExpression(BufferString& exp) const
			{ exp = fulluserexpression_; }

protected:

    CtxtIOObj&		ctio_;
    BufferString	fulluserexpression_;
    BufferString	othersurveyrootdir_;

    bool		acceptOK(CallBacker*);
};

#endif

