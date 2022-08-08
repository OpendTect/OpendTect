#pragma once
/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:	       Jan 2011
________________________________________________________________________

-*/


#include "uiwellmod.h"
#include "uiwellmod.h"
#include "uidialog.h"
#include "bufstringset.h"
#include "multiid.h"
#include "uistring.h"

class uiCheckBox;
class uiComboBox;
class uiFreqFilterSelFreq;
class uiGenInput;
class uiMultiWellLogSel;
class uiLabel;
class uiLabeledComboBox;
class uiLabeledSpinBox;
class uiButton;
class uiSpinBox;
class uiTable;
class uiWellLogDisplay;


namespace Well
{ class Data; class Log; class LogSet; class D2TModel; class Track; }

class uiWellLogToolWinGrp;

mExpClass(uiWell) uiWellLogToolWin : public uiMainWin
{ mODTextTranslationClass(uiWellLogToolWin)
public:

    mStruct(uiWell) LogData
    {
				LogData(Well::Data&);
				~LogData();

	MultiID			wellid_;
	BufferString		wellname_;
	Interval<float>		dahrg_;

	int			setSelectedLogs(BufferStringSet&);
	Well::LogSet&		logs() const		{ return logs_; }
	const Well::D2TModel*	d2t();
	const Well::Track*	track();

	const ObjectSet<const Well::Log> inpLogs() const  { return inplogs_; }

    protected:

	RefMan<Well::Data>		wd_;
	Well::LogSet&			logs_;
	ObjectSet<const Well::Log>	inplogs_;
    };

				uiWellLogToolWin(uiParent*,ObjectSet<LogData>&,
						 bool withedit=true);
				~uiWellLogToolWin();

    void			getLogDatas(ObjectSet<LogData>& lds) const
				{ lds = logdatas_; }

protected:

    uiComboBox*			actionfld_;
    uiGenInput*			savefld_;
    uiGenInput*			extfld_;
    uiSpinBox*			gatefld_;
    uiLabel*			gatelbl_;
    uiLabeledSpinBox*		thresholdfld_;
    uiLabeledComboBox*		replacespikefld_;
    uiGenInput*			replacespikevalfld_;
    uiFreqFilterSelFreq*	freqfld_;
    uiButton*			applybut_;
    uiButton*			okbut_;
    uiButton*			cancelbut_;
    Interval<float>		zdisplayrg_;
    uiWellLogToolWinGrp*	logdisp_;

    ObjectSet<LogData>		logdatas_;
    bool			closeok_ = true;

    uiGroup*			createEditGroup();
    void			displayLogs();
    bool			saveLogs();

    void			saveCB(CallBacker*);
    void			actionSelCB(CallBacker*);
    void			handleSpikeSelCB(CallBacker*);
    void			applyPushedCB(CallBacker*);
    void			acceptOK(CallBacker*);
    void			rejectOK(CallBacker*);
    bool			closeOK() override;
};


mExpClass(uiWell) uiWellLogToolWinMgr : public uiDialog
{ mODTextTranslationClass(uiWellLogToolWinMgr)
public:
			uiWellLogToolWinMgr(uiParent*,
					const BufferStringSet* welllnms=nullptr,
					const BufferStringSet* lognms=nullptr);
protected:

    uiMultiWellLogSel*	welllogselfld_;

    bool		acceptOK(CallBacker*) override;
    void		winClosed(CallBacker*);
    int			checkMaxLogsToDisplay();
};


mExpClass(uiWell) uiWellLogEditor : public uiDialog
{ mODTextTranslationClass(uiWellLogEditor)
public:
			uiWellLogEditor(uiParent*,Well::Log&);
			~uiWellLogEditor();

    void		selectMD(float md);
    bool		isLogChanged() const	{ return changed_; }

    Notifier<uiWellLogEditor>	valueChanged;

protected:
    void		fillTable();
    void		valChgCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;
    void		rowDelCB(CallBacker*);
    void		rowInsertCB(CallBacker*);

    Well::Log&		log_;
    uiTable*		table_;

    bool		changed_;

};

