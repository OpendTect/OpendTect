#pragma once
/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
RCS:           $Id$
________________________________________________________________________

-*/


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


mExpClass(uiWell) uiWellLogToolWin : public uiMainWin
{ mODTextTranslationClass(uiWellLogToolWin)
public:

    mStruct(uiWell) LogData
    {
				LogData(const Well::LogSet&,
					const Well::D2TModel*,
					const Well::Track*);
				~LogData();

	MultiID			wellid_;
	BufferString		wellname_;
	Interval<float>		dahrg_;

	int			setSelectedLogs(BufferStringSet&);
	void			getOutputLogs(Well::LogSet& ls) const;

    protected:

	Well::LogSet&		logs_;

	ObjectSet<const Well::Log> inplogs_;
	ObjectSet<Well::Log>	outplogs_;

	const Well::D2TModel*	d2t_;
	const Well::Track*	track_;

	friend class		uiWellLogToolWin;
    };

				uiWellLogToolWin(uiParent*,ObjectSet<LogData>&,
						 bool withedit=true);
				~uiWellLogToolWin();

    bool			needSave() const	{ return needsave_; }

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

    ObjectSet<LogData>		logdatas_;
    ObjectSet<uiWellLogDisplay> logdisps_;
    bool			needsave_;
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
    bool			closeOK();
};


mExpClass(uiWell) uiWellLogToolWinMgr : public uiDialog
{ mODTextTranslationClass(uiWellLogToolWinMgr)
public:
			uiWellLogToolWinMgr(uiParent*,
					const BufferStringSet* welllnms=nullptr,
					const BufferStringSet* lognms=nullptr);
protected:

    uiMultiWellLogSel*	welllogselfld_;

    bool		acceptOK(CallBacker*);
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
    bool		acceptOK(CallBacker*);
    void		rowDelCB(CallBacker*);
    void		rowInsertCB(CallBacker*);

    Well::Log&		log_;
    uiTable*		table_;

    bool		changed_;

};

