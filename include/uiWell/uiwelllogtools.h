#ifndef uiwelllogetool_h
#define uiwelllogetool_h
/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
RCS:           $Id$
________________________________________________________________________

-*/


#include "uiwellmod.h"
#include "uiwellmod.h"
#include "uidialog.h"
#include "bufstringset.h"
#include "multiid.h"

class uiCheckBox;
class uiComboBox;
class uiFreqFilterSelFreq;
class uiGenInput;
class uiMultiWellLogSel;
class uiLabel;
class uiLabeledComboBox;
class uiLabeledSpinBox;
class uiListBox;
class uiPushButton;
class uiSpinBox;
class uiWellLogDisplay;


namespace Well { class Data; class Log; class LogSet; class D2TModel; }


mClass(uiWell) uiWellLogToolWin : public uiMainWin
{
public:	

    mStruct(uiWell) LogData
    {
				LogData(const Well::LogSet&,
					const Well::D2TModel*);
				~LogData();

	MultiID			wellid_;
	const char*		wellname_;
	Interval<float>		dahrg_;

	int			setSelectedLogs(BufferStringSet&);
	void			getOutputLogs(Well::LogSet& ls) const;

    protected:

	Well::LogSet&		logs_;

	ObjectSet<const Well::Log> inplogs_;
	ObjectSet<Well::Log>	outplogs_;

	const Well::D2TModel* 	d2t_;

	friend class		uiWellLogToolWin;
    };

				uiWellLogToolWin(uiParent*,ObjectSet<LogData>&);
				~uiWellLogToolWin();

    bool                	needSave() const        { return needsave_; }

    void			getLogDatas(ObjectSet<LogData>& lds) const
				{ lds = logdatas_; }

protected:

    uiComboBox*			actionfld_;
    uiCheckBox*			overwritefld_;
    uiGenInput*			savefld_;
    uiSpinBox*			gatefld_;
    uiLabel*			gatelbl_;
    uiLabeledSpinBox*		thresholdfld_;
    uiLabeledComboBox*		replacespikefld_;
    uiGenInput*			replacespikevalfld_;
    uiFreqFilterSelFreq*	freqfld_;
    uiPushButton*		applybut_;
    uiPushButton*               okbut_;
    uiPushButton*               cancelbut_;
    Interval<float>		zdisplayrg_;

    ObjectSet<LogData>		logdatas_;
    ObjectSet<uiWellLogDisplay> logdisps_;
    bool			needsave_;

    void			displayLogs();

    void			actionSelCB(CallBacker*);
    void			handleSpikeSelCB(CallBacker*);
    void			overWriteCB(CallBacker*);
    void			applyPushedCB(CallBacker*);
    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);
};


mClass(uiWell) uiWellLogToolWinMgr : public uiDialog
{
public:
			uiWellLogToolWinMgr(uiParent*);
protected:

    uiMultiWellLogSel*	welllogselfld_;

    bool		acceptOK(CallBacker*);
    void		winClosed(CallBacker*);
};



#endif


