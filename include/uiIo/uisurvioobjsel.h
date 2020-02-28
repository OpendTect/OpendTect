#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno / Bert
 Date:          Dec 2010 / Oct 2016
________________________________________________________________________

-*/

/*!\brief select IOObj(s), possibly from an other survey. */


#include "uiiocommon.h"
#include "uigroup.h"
#include "dbkey.h"
#include "surveydisklocation.h"

class IOObjContext;
class uiButton;
class uiComboBox;
class uiLabel;


/*\brief selector for objects in (possibly) other surveys.
  Can also be used for compact IOObj selector in tables. */

mExpClass(uiIo) uiSurvIOObjSel : public uiGroup
{
public:
			uiSurvIOObjSel(uiParent*,const IOObjContext&,
				       const uiString& lbltxt=uiString::empty(),
				       bool surveyfixed=false);
    virtual		~uiSurvIOObjSel();

    void		setSurvey(const SurveyDiskLocation&);
    void		addExclude(const SurveyDiskLocation&);
    void		excludeCurrentSurvey()
			{ addExclude(SurveyDiskLocation::currentSurvey()); }

    void		setSelected(const DBKey&);
    void		setSelected(const char* nm,bool mostsimilar=true);
    void		setSurveySelectable(bool);
			//!< only possible if you did not specify surveyfixed
    void		setLblText(const uiString&);
			//!< only possible if you already passed a lbltxt
    void		refresh();

    SurveyDiskLocation	surveyDiskLocation() const  { return survloc_; }
    const IOObj*	ioObj() const;
    DBKey		key() const;
    BufferString	ioObjName() const;
    BufferString	mainFileName() const;

    const ObjectSet<IOObj>& objsInSurvey() const { return ioobjs_; }

    Notifier<uiSurvIOObjSel>	selChange;
    Notifier<uiSurvIOObjSel>	survChange;

protected:

    SurveyDiskLocation	survloc_;
    IOObjContext&	ctxt_;
    ObjectSet<IOObj>	ioobjs_;
    BufferStringSet	ioobjnames_;
    TypeSet<SurveyDiskLocation>	excludes_;

    uiComboBox*		objfld_;
    uiLabel*		lbl_		= 0;
    uiButton*		survselbut_	= 0;

    void		initGrp(CallBacker*);
    void		selChgCB(CallBacker*);
    void		selSurvCB(CallBacker*);

    void		updateObjs();
    void		updateUi(const DBKey* dbky=0);

};
