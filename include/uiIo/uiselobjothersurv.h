#ifndef uiselobjfromothersurv_h
#define uiselobjfromothersurv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Dec 2010
 RCS:           $Id: uiselobjothersurv.h,v 1.1 2010-12-14 08:50:32 cvsbruno Exp $
________________________________________________________________________

-*/

/*! brief get an object from an other survey ( type is given by the CtxtIOObj ).
  will first pop-up a list to select the survey, then a list of the objects 
  belonging to that survey !*/


#include "uiselsimple.h"

class CtxtIOObj;

mClass uiSelObjFromOtherSurvey : public uiSelectFromList
{
public:
    			uiSelObjFromOtherSurvey(uiParent*,CtxtIOObj&);
    			~uiSelObjFromOtherSurvey();
protected:

    CtxtIOObj&		ctio_;

    bool		acceptOK(CallBacker*);
};

#endif
