#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Khushnood Qadir
 Date:		May 2020
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uidialog.h"
#include "multiid.h"
#include "welllogmerge.h"

class uiCheckBox;
class uiCheckList;
class uiComboBox;
class uiGenInput;
class uiListBox;
class uiToolButton;
class uiUnitSel;
class uiWellLogDisplay;
namespace Well { class Log; class LogMerger; class LogSet; }

mExpClass(uiWell) uiWellLogMerger : public uiDialog
{ mODTextTranslationClass(uiWellLogMerger);
public:

		uiWellLogMerger(uiParent*,
				const TypeSet<MultiID>&,
				const BufferStringSet* chosenlognms=nullptr);
		~uiWellLogMerger();

    bool			haveNewLogs() const	{ return havenew_; }

    void			setOutputLogName(const char* nm);
    const char*			getOutputLogName() const;

private:

    uiGroup*			mergeparamgrp_;
    uiGroup*			mergedlogdispgrp_;
    uiGenInput*			logfld_;
    uiGenInput*			overlapfld_;
    uiGenInput*			extrapolatefld_;
    uiGenInput*			intrapolatefld_;
    uiListBox*			loglistfld_;
    uiListBox*			selectedlogsfld_;
    uiGroup*			selbuttons_;
    uiGroup*			movebuttons_;
    uiGroup*			changelogdispbut_;
    uiToolButton*		toselect_;
    uiToolButton*		fromselect_;
    uiToolButton*		moveupward_;
    uiToolButton*		movedownward_;
    uiToolButton*		nextlog_;
    uiToolButton*		prevlog_;
    uiGenInput*			srfld_;
    uiGenInput*			nmfld_;
    uiUnitSel*			outunfld_;
    uiWellLogDisplay*		logdisp_;

    Well::LogMerger::OverlapAction	action_ = Well::LogMerger::UseAverage;

    const TypeSet<MultiID>	wellids_;
    ObjectSet<Well::Log>	outlgs_;
    BufferStringSet		chosenlognms_;
    float			zsampling_;
    bool			havenew_;
    bool			needselectlogfld_ = false;

    void			createMergeParaGrp();
    void			createMergedLogDispGrp();

    void			updateLogViewer();
    void			merge();
    bool			writeNewLog();
    void			convertMergedLog();

    void			logSetCB(CallBacker*);
    void			overlapCB(CallBacker*);
    void			selButPush(CallBacker*);
    void			moveButPush(CallBacker*);
    void			changeLogDispButPush(CallBacker*);

    bool			acceptOK(CallBacker*);
    void			applyCB(CallBacker*);
};
