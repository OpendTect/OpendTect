#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2016
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uigroup.h"
class uiListBox;
class uiDataRootSel;
class uiSeparator;
namespace File { class Monitor; }


mExpClass(uiIo) uiSurveySelect : public uiGroup
{ mODTextTranslationClass(uiSurveySelect);
public:

			uiSurveySelect(uiParent*,bool align=true,
				const char* survnm=0,const char* dataroot=0);
			~uiSurveySelect();

    bool		validSelection() const;
    BufferString	getDirName() const;
    BufferString	getFullDirPath() const;
    void		setSurveyDirName(const char*);

    Notifier<uiSurveySelect>	survDirChg;
    Notifier<uiSurveySelect>	survParsChg;
    Notifier<uiSurveySelect>	survDirAccept;

protected:

    BufferString	dataroot_;
    File::Monitor*	filemonitor_;

    uiDataRootSel*	datarootfld_;
    uiGroup*		maingrp_;
    uiGroup*		survselgrp_;
    uiListBox*		survdirfld_;
    uiSeparator*	topsep_;

    void		updateList();
    void		startFileMonitoring();
    void		stopFileMonitoring();
    void		dataRootChgCB(CallBacker*);
    void		survDirChgCB(CallBacker*);
    void		survParFileChg(CallBacker*);
    void		survDirAcceptCB(CallBacker*);

};
