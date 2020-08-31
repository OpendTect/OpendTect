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
				    const BufferStringSet* lognms=nullptr);
				~uiWellLogMerger();

    bool			haveNewLogs() const	{ return havenew_; }

    void			setOutputLogName(const char*);
    const char*			getOutputLogName() const;

private:

    uiGenInput*			logtypefld_ = nullptr;
    uiGenInput*			overlapfld_;
    uiCheckBox*			extrapolatefld_;
    uiCheckBox*			intrapolatefld_;
    uiListBox*			loglistfld_ = nullptr;
    uiListBox*			selectedlogsfld_;
    uiGroup*			movebuttons_;
    uiToolButton*		toselect_ = nullptr;
    uiToolButton*		fromselect_ = nullptr;
    uiToolButton*		moveupward_;
    uiToolButton*		movedownward_;
    uiToolButton*		nextlog_ = nullptr;
    uiToolButton*		prevlog_ = nullptr;
    uiGenInput*			srfld_;
    uiGenInput*			nmfld_ = nullptr;
    uiUnitSel*			outunfld_;
    uiWellLogDisplay*		logdisp_;

    Well::LogMerger::OverlapAction	action_ = Well::LogMerger::UseAverage;

    const TypeSet<MultiID>	wellids_;
    ObjectSet<Well::Log>	outlogs_;
    BufferStringSet		chosenlognms_;
    float			zsampling_;
    bool			havenew_;

    void			finalizeCB(CallBacker*);
    uiGroup*			createParGrp(bool);
    uiGroup*			createLogDispGrp();

    void			updateLogViewer();
    bool			merge();
    bool			write();
    bool			convert();

    void			logSetCB(CallBacker*);
    void			overlapCB(CallBacker*);
    void			selButPush(CallBacker*);
    void			moveButPush(CallBacker*);
    void			changeLogDispButPush(CallBacker*);

    bool			acceptOK(CallBacker*);
    void			applyCB(CallBacker*);
};
