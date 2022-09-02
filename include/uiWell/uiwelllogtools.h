#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uidialog.h"

#include "bufstringset.h"
#include "uistring.h"
#include "wellselection.h"

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
class uiWellLogToolWinGrp;

namespace Well
{
    class D2TModel;
    class Data;
    class Log;
    class LogSet;
    class Track;
}


mExpClass(uiWell) WellLogToolData : public Well::SubSelData
{
public:
			WellLogToolData(const Well::SelInfo&);
			~WellLogToolData();

    const ObjectSet<const Well::Log> inpLogs() const  { return inplogs_; }

protected:
    void			init();

    ObjectSet<const Well::Log>	inplogs_;
};


mExpClass(uiWell) uiWellLogToolWin : public uiMainWin
{ mODTextTranslationClass(uiWellLogToolWin)
public:
				uiWellLogToolWin(uiParent*,
						ObjectSet<WellLogToolData>&,
						bool withedit=true);
				~uiWellLogToolWin();

    void			getLogDatas(
					ObjectSet<WellLogToolData>& lds) const
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

    ObjectSet<WellLogToolData>		logdatas_;
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
