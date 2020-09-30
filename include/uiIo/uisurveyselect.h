#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2016
________________________________________________________________________

-*/

#include "uiiocommon.h"

#include "uicompoundparsel.h"
#include "uigroup.h"
#include "surveydisklocation.h"

class uiListBox;
class uiDataRootSel;
class uiSeparator;
namespace File { class Monitor; }


/*!\brief selects a survey, possibly in another data root.

 The 'align' parameter in the constructor determines whether the 'data root'
 field and the 'survey directory' fields will be aligned, or just
 ensureBelow-ed, with a separator between them.
 */

mExpClass(uiIo) uiSurveySelect : public uiGroup
{ mODTextTranslationClass(uiSurveySelect);
public:

			uiSurveySelect(uiParent*,bool align=true);
			uiSurveySelect(uiParent*,const SurveyDiskLocation&,
					bool align=true);
			~uiSurveySelect();

    bool		validSelection() const;
    SurveyDiskLocation	surveyDiskLocation() const;
    void		setSurveyDiskLocation(const SurveyDiskLocation&);
    void		setSurveyDirName(const char*);
    void		addExclude(const SurveyDiskLocation&);
    BufferString	getDirName() const;
    BufferString	getFullDirPath() const;

    Notifier<uiSurveySelect>	survDirChg;
    Notifier<uiSurveySelect>	survParsChg;
    Notifier<uiSurveySelect>	survDirAccept;

protected:

    BufferString	dataroot_;
    File::Monitor*	filemonitor_;
    BufferString	defsurvdirnm_;
    ObjectSet<SurveyDiskLocation> excludes_;

    uiDataRootSel*	datarootfld_;
    uiGroup*		maingrp_;
    uiGroup*		survselgrp_;
    uiListBox*		survdirfld_;
    uiSeparator*	topsep_;

    void		updateList();
    void		startFileMonitoring();
    void		stopFileMonitoring();

    void		initGrp(CallBacker*);
    void		dataRootChgCB(CallBacker*);
    void		survDirChgCB(CallBacker*);
    void		survParFileChg(CallBacker*);
    void		survDirAcceptCB(CallBacker*);

};

/*!\brief selects a survey

If usemanager is true, a uiSurveyManagerDlg is launched. Otherwise a uiDialog
  with a uiSurveySelect group.
*/

mExpClass(uiIo) uiSurvSel : public uiCompoundParSel
{
public:
			uiSurvSel(uiParent*,bool showmanager);
			~uiSurvSel();

protected:
    void		doDlg(CallBacker*);
    uiString		getSummary() const override;

    bool		showmanager_;
};
