#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno / Bert
 Date:          Dec 2010 / Oct 2016
________________________________________________________________________

-*/

/*!\brief get an IOObj from an other survey. Will first pop-up a list to select
  the survey, then a list of the objects belonging to that survey */


#include "uiiocommon.h"
#include "uidialog.h"

class IOObj;
class CtxtIOObj;
class IOObjContext;
class uiSurveySelect;
class uiListBox;


mExpClass(uiIo) uiSelObjFromOtherSurvey : public uiDialog
{ mODTextTranslationClass(uiSelObjFromOtherSurvey);
public:

			uiSelObjFromOtherSurvey(uiParent*,const IOObjContext&);
			~uiSelObjFromOtherSurvey();

    BufferString	fullUserExpression() const { return usrexpr_; }
    const IOObj*	ioObj() const;

    BufferString	sourceSurveyDirectory() const;
    const ObjectSet<IOObj> objsInSurvey() const	    { return ioobjs_; }

protected:

    IOObjContext&	ctxt_;
    ObjectSet<IOObj>	ioobjs_;
    BufferString	usrexpr_;
    int			selidx_;

    uiSurveySelect*	survsel_;
    uiListBox*		objfld_;

    void		survSelCB(CallBacker*);

    bool		acceptOK();

};
