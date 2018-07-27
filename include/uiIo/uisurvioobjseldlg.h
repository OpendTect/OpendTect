#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno / Bert
 Date:          Dec 2010 / Oct 2016 / July 2018
________________________________________________________________________

-*/

/*!\brief select IOObj(s), possibly from an other survey. */


#include "uiiocommon.h"
#include "uidialog.h"
#include "dbkey.h"

class IOObjContext;
class uiSurvIOObjSelGroup;


mExpClass(uiIo) uiSurvIOObjSelDlg : public uiDialog
{ mODTextTranslationClass(uiSurvIOObjSelDlg);
public:

			uiSurvIOObjSelDlg(uiParent*,const IOObjContext&,
					bool selmulti=false,bool fixsurv=false);

    void		setSurvey(const SurveyDiskLocation&);
    void		setSelected(const DBKey&);
    void		setSelected(const DBKeySet&);
    void		addExclude(const SurveyDiskLocation&);
    void		excludeCurrentSurvey();
    void		setSurveySelectable(bool);

    int			nrSelected() const;
    const IOObj*	ioObj(int idx=0) const;
    DBKey		key(int idx=0) const;
    BufferString	mainFileName(int idx=0) const;
    const ObjectSet<IOObj>& objsInSurvey() const;
    SurveyDiskLocation	surveyDiskLocation() const;

    uiSurvIOObjSelGroup&	selGrp()	{ return *selgrp_; }
    const uiSurvIOObjSelGroup&	selGrp() const	{ return *selgrp_; }

protected:

    uiSurvIOObjSelGroup* selgrp_;

    void		initWin(CallBacker*);
    void		survChgCB(CallBacker*);

    bool		acceptOK();

};
