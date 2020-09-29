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

class IOObjContext;
class uiSurveySelect;
class uiListBox;
class uiListBoxChoiceIO;
class uiIOObjSelGrp;

/* allows selection of IOObj in any survey. */

mExpClass(uiIo) uiSurvIOObjSelGroup : public uiGroup
{
public:

			uiSurvIOObjSelGroup(uiParent*,const IOObjContext&,
					    bool selmulti=false,
					    bool fixsurv=false,
					    bool withinserters=false );
    virtual		~uiSurvIOObjSelGroup();

    void		setSurvey(const SurveyDiskLocation&);
    void		addExclude(const SurveyDiskLocation&);
    void		excludeCurrentSurvey();
    void		setSurveySelectable(bool);

    int			indexOf(const DBKey&);

    void		setSelected(const DBKey&, bool add=false);
    void		setSelected(const DBKeySet&);

    virtual bool	evaluateInput();

    // Available after evaluateInput():
    SurveyDiskLocation	surveyDiskLocation() const;
    int			nrSelected() const	{ return chosennms_.size(); }
    const IOObj*	ioObj(int iselected=0) const;
    DBKey		key(int iselected=0) const;
    BufferString	mainFileName(int iselected=0) const;

    const IOObjContext&	ioobjContext()		 { return ctxt_; }
    const ObjectSet<IOObj>& objsInSurvey() const { return ioobjs_; }

    Notifier<uiSurvIOObjSelGroup>	dClicked;
    Notifier<uiSurvIOObjSelGroup>	survChange;
    Notifier<uiSurvIOObjSelGroup>	selChange;

protected:

    IOObjContext&	ctxt_;
    ObjectSet<IOObj>	ioobjs_;
    BufferStringSet	chosennms_;
    DBKeySet		seldbkys_;
    const bool		ismultisel_;
    SurveyDiskLocation	survloc_; //!< only used when survey fixed

    uiSurveySelect*	survsel_;
    uiIOObjSelGrp*	objfld_;

    void		initGrp(CallBacker*);
    void		survSelCB(CallBacker*);
    void		dClickCB(CallBacker*);
    void		selChgCB(CallBacker*);
    void		itemChgCB(CallBacker*);

    void		updGrp(bool withsurvsel);
    void		selSurvFromSelection();
    void		updateObjs();
    void		setSelection();

public:

    void		insert(uiObject&);
    virtual void	refresh()		{ updGrp(false); }

};
